/* FCE Ultra - NES/Famicom Emulator - Emscripten audio
 *
 * Copyright notice for this file:
 *  Copyright (C) 2015 Valtteri "tsone" Heikkila
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "em.h"
#include "../../utils/memory.h"
#include "../../fceu.h"
#include <emscripten.h>

// NOTE: tsone: define to output test sine tone
#define TEST_SINE_AT_FILL	0
#define TEST_SINE_AT_WRITE	0

typedef float buf_t;
typedef float mix_t;

extern int EmulationPaused;

int em_sound_rate = SOUND_RATE;
int em_sound_frame_samples = SOUND_RATE / NTSC_FPS;

static buf_t *s_Buffer = 0;
static int s_BufferRead = 0;
static int s_BufferWrite = 0;
static int s_BufferCount = 0;

static int s_initialized = 0;
static int s_silenced = 0;

// Sound is NOT muted if this is 0, otherwise it is muted.
static int s_soundvolumestore = 0;

#if TEST_SINE_AT_FILL || TEST_SINE_AT_WRITE
#include <math.h>
static double testSinePhase = 0.0;
#endif

// Consumes samples from audio buffer. Modifies the buffer indexes and
// returns the number of available samples. Does NOT modify HW audio buffer.
static int consumeBuffer(int samples)
{
	const int available = (s_BufferCount > samples) ? samples : s_BufferCount;
	s_BufferRead = (s_BufferRead + available) & SOUND_BUF_MASK;
	s_BufferCount -= available;
	return available;
}

// Copy buffer samples to HW buffer. Fill zeros if there's not enough samples.
static void AudioCopy()
{
	int bufferRead = s_BufferRead; // consumeBuffer() modifies this.
// FIXME: tsone: hard-coded SOUND_HW_BUF_MAX
	int available = consumeBuffer(SOUND_HW_BUF_MAX);
	EM_ASM_ARGS({
		var s_Buffer = $0;
		var s_BufferRead = $1;
		var available = $2;

		var channelData = FCEM.currentOutputBuffer.getChannelData(0);

// FIXME: tsone: hard-coded SOUND_BUF_MAX
		var m = 8192 - s_BufferRead;
		if (m > available) {
			m = available;
		}

#if 1
		// Float32Array.set() version.
// FIXME: tsone: hard-coded SOUND_HW_BUF_MAX
		var samples = 2048 - available + 1;
		available = available - m;

		if (m > 0) {
			var j = s_Buffer + s_BufferRead;
			channelData.set(HEAPF32.subarray(j, j + m));
		}
		if (available > 0) {
			channelData.set(HEAPF32.subarray(s_Buffer, s_Buffer + available), m);
		}
		m += available - 1;
		while (--samples) {
			channelData[++m] = 0;
		}

#else
		// Regular version without Float32Array.set().
		var j = s_Buffer + s_BufferRead - 1;
// FIXME: tsone: hard-coded SOUND_HW_BUF_MAX
		var samples = 2048 - available + 1;
		available = available - m + 1;
		++m;

		var i = -1;
		while (--m) {
			channelData[++i] = HEAPF32[++j];
		}
		j = s_Buffer - 1;
		while (--available) {
			channelData[++i] = HEAPF32[++j];
		}
		while (--samples) {
			channelData[++i] = 0;
		}
#endif
	}, (ptrdiff_t) s_Buffer >> 2, bufferRead, available);
}

// Fill HW audio buffer with silence.
static void AudioSilence()
{
	EM_ASM({
		var channelData = FCEM.currentOutputBuffer.getChannelData(0);
// FIXME: tsone: hard-coded SOUND_HW_BUF_MAX
		for (var i = 2048 - 1; i >= 0; --i) {
			channelData[i] = 0;
		}
	});
}

// Callback for filling HW audio buffer.
static void AudioCallback()
{
	if (!EmulationPaused && !s_silenced) {
		AudioCopy();
	} else {
		AudioSilence();
	}

	if (s_silenced) {
// FIXME: tsone: hard-coded SOUND_HW_BUF_MAX
		consumeBuffer(SOUND_HW_BUF_MAX);
	}
}

void SilenceSound(int option)
{
	s_silenced = option;
}

int IsSoundInitialized()
{
	return s_initialized;
}

int CreateAudioContext()
{
	return EM_ASM_INT_V({
		if (!FCEM.audioContext) {
				if (typeof(AudioContext) !== 'undefined') {
					FCEM.audioContext = new AudioContext();
				} else if (typeof(webkitAudioContext) !== 'undefined') {
					FCEM.audioContext = new webkitAudioContext();
				} else {
					return 0;
				}
		}
		return 1;
	});
}

// Returns the sampleRate (Hz) of initialized audio context.
int InitAudioContext()
{
	if (!CreateAudioContext()) {
		return 0;
	}

	EM_ASM_ARGS({
		// Always mono (1 channel).
		FCEM.scriptProcessorNode = FCEM.audioContext.createScriptProcessor($0, 0, 1);
		FCEM.scriptProcessorNode.onaudioprocess = function(ev) {
			FCEM.currentOutputBuffer = ev.outputBuffer;
			Runtime.dynCall('v', $1);
		};
		FCEM.scriptProcessorNode.connect(FCEM.audioContext.destination);
	}, SOUND_HW_BUF_MAX, AudioCallback);

	return EM_ASM_INT_V({
		return FCEM.audioContext.sampleRate;
	});
}


int InitSound()
{
	int sampleRate;

	if (IsSoundInitialized()) {
		return 1;
	}

	printf("Initializing Web Audio.\n");

	s_Buffer = (buf_t*) FCEU_dmalloc(sizeof(buf_t) * SOUND_BUF_MAX);
	if (!s_Buffer) {
		goto error;
	}
	s_BufferRead = s_BufferWrite = s_BufferCount = 0;

	sampleRate = InitAudioContext();
	if (!sampleRate) {
		FCEU_dfree(s_Buffer);
		s_Buffer = 0;
		goto error;
	}
	em_sound_rate = sampleRate;
	em_sound_frame_samples = em_sound_rate / NTSC_FPS;

	FCEUI_SetSoundVolume(150);
	FCEUI_SetSoundQuality(SOUND_QUALITY);
	FCEUI_Sound(em_sound_rate);
	FCEUI_SetTriangleVolume(256);
	FCEUI_SetSquare1Volume(256);
	FCEUI_SetSquare2Volume(256);
	FCEUI_SetNoiseVolume(256);
	FCEUI_SetPCMVolume(256);

	s_initialized = 1;
	return 1;

error:
	FCEUD_PrintError("Failed to initialize Web Audio.");
	return 0;
}

int GetSoundBufferCount(void)
{
	return s_BufferCount;
}

void WriteSound(int32 *buf, int Count)
{
	if (EmulationPaused || (Count <= 0)) {
		return;
	}

	int freeCount = SOUND_BUF_MAX - s_BufferCount;
	if (Count > freeCount) {
//		printf("!!!! WriteSound: Tried to write %d samples while buffer has %d free.\n", Count, freeCount);
		Count = freeCount;
	}

#if TEST_SINE_AT_WRITE

	// Sine wave test outputs a 440Hz tone.
	s_BufferCount += Count;
	++Count;
	while (--Count) {
		s_Buffer[s_BufferWrite] = 0.5 * sin(testSinePhase * (2.0*M_PI * 440.0/em_sound_rate));
		s_BufferWrite = (s_BufferWrite + 1) & SOUND_BUF_MASK;
		++testSinePhase;
	}

#else

#if 1
// NOTE: tsone: optimized version of below
	int i = s_BufferWrite - 1;
	int j = -1;
	int m = SOUND_BUF_MAX - s_BufferWrite;
	if (m > Count) {
		m = Count;
	}

	s_BufferCount += Count;
	s_BufferWrite = (s_BufferWrite + Count) & SOUND_BUF_MASK;

	Count = Count - m + 1;
	++m;

	while (--m) {
		s_Buffer[++i] = buf[++j] / 32768.0;
	}
	i = -1;
	while (--Count) {
		s_Buffer[++i] = buf[++j] / 32768.0;
	}

#else

	for (int i = 0; i < Count; ++i) {
		s_Buffer[s_BufferWrite] = buf[i];
		s_BufferWrite = (s_BufferWrite + 1) & SOUND_BUF_MASK;
	}

	s_BufferCount += Count;

#endif

#endif // TEST_SINE_AT_WRITE
}

int KillSound(void)
{
	return 0;
}

void FCEUD_SoundVolumeAdjust(int n)
{
        int soundvolume = s_soundvolumestore ? s_soundvolumestore : FSettings.SoundVolume;
	s_soundvolumestore = 0;

	if (n < 0) {
		soundvolume -= 10;
	} else if (n > 0) {
		soundvolume += 10;
	} else {
		soundvolume = 100;
	}

	if (soundvolume < 0) {
		soundvolume = 0;
        } else if (soundvolume > 150) {
		soundvolume = 150;
        }

	FCEUI_SetSoundVolume(soundvolume);
}

void FCEUD_SoundToggle(void)
{
	if(s_soundvolumestore) {
		FCEUI_SetSoundVolume(s_soundvolumestore);
		s_soundvolumestore = 0;
	} else {
		s_soundvolumestore = FSettings.SoundVolume;
		FCEUI_SetSoundVolume(0);
	}
}
