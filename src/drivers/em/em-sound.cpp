/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

// NOTE: tsone: define to output test sine tone
#define TEST_SINE_AT_FILL	0
#define TEST_SINE_AT_WRITE	0

#if SDL_VERSION_ATLEAST(2, 0, 0)
typedef float buf_t;
typedef float mix_t;
#else
typedef int buf_t;
typedef int16 mix_t;
#endif

int em_sound_rate = SOUND_RATE;
int em_sound_frame_samples = SOUND_RATE / 60;

static buf_t *s_Buffer = 0;
static int s_BufferRead = 0;
static int s_BufferWrite = 0;
static int s_BufferCount = 0;

#if SDL_VERSION_ATLEAST(2, 0, 0)
static int s_initialized = 0;
#endif

// Sound is NOT muted if this is 0, otherwise it is muted.
static int s_soundvolumestore = 0;

#if TEST_SINE_AT_FILL || TEST_SINE_AT_WRITE
#include <math.h>
static double testSinePhase = 0.0;
#endif

/**
 * Callback from the SDL to get and play audio data.
 */
static void fillaudio(void *udata, uint8 *stream, int len)
{
#if TEST_SINE_AT_FILL

	// Sine wave test outputs a 440Hz tone.
	len = len / sizeof(mix_t);
	mix_t* str = (mix_t*) stream;
	for (int i = 0; i < len; ++i) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
		str[i] = 0.5 * sin(testSinePhase * (2.0*M_PI * 440.0/em_sound_rate));
#else
		str[i] = 0xFFF * sin(testSinePhase * (2.0*M_PI * 440.0/em_sound_rate));
#endif
		++testSinePhase;
	}
	s_BufferCount -= len;

#else

#if 1

// NOTE: tsone: this is an optimized version of the below filler loop
	mix_t* str = (mix_t*) stream;
	len = len / sizeof(mix_t);

	int i = -1;
	int j = s_BufferRead - 1;
	int d = (s_BufferCount > len) ? len : s_BufferCount;
	int m = SOUND_BUF_MAX - s_BufferRead;
	if (m > d) {
		m = d;
	}

	s_BufferRead = (s_BufferRead + d) & SOUND_BUF_MASK;
	s_BufferCount -= d;

	len = len - d + 1;
	d = d - m + 1;
	++m;

	while (--m) {
		str[++i] = s_Buffer[++j];
	}
	j = -1;
	while (--d) {
		str[++i] = s_Buffer[++j];
	}
	while (--len) {
		str[++i] = 0;
	}

#else
	int16* str = (int16*) stream;
	len >>= 1;
	int d = (s_BufferCount > len) ? len : s_BufferCount;
	int i = 0;
	if (s_BufferCount < len) {
		printf("Sound buffer exhausted (has %d samples, needs %d).\n", s_BufferCount, len);
	}
	for (; i < d; ++i) {
		str[i] = s_Buffer[s_BufferRead];
		s_BufferRead = (s_BufferRead + 1) & SOUND_BUF_MASK;
	}
	for (; i < len; ++i) {
		str[i] = 0;
	}
	s_BufferCount -= d;
#endif

#endif // TEST_SINE_AT_FILL
}

int InitSound()
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (s_initialized) {
		return 1;
	}
#endif

	SDL_AudioSpec spec;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_AudioSpec obtained;
#endif

	memset(&spec, 0, sizeof(spec));
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		puts(SDL_GetError());
		KillSound();
		return 0;
	}

#if SDL_VERSION_ATLEAST(2, 0, 0)
	const char *driverName = SDL_GetAudioDriver(0);
#else
	char driverName[8];
	SDL_AudioDriverName(driverName, 8);
#endif
	printf("Loading SDL audio driver %s...\n", driverName);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	spec.format = AUDIO_F32;
#else
	spec.format = AUDIO_S16SYS;
#endif
	spec.channels = 1;
	spec.freq = SOUND_RATE;
	spec.samples = SOUND_HW_BUF_MAX;
	spec.callback = fillaudio;
	spec.userdata = 0;

//	printf("Sound buffersize: %d, HW: %d)\n", SOUND_BUF_MAX, spec.samples);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	if(SDL_OpenAudio(&spec, &obtained)) {
#else
	if(SDL_OpenAudio(&spec, 0)) {
#endif
		puts(SDL_GetError());
		KillSound();
		return 0;
	}
#if SDL_VERSION_ATLEAST(2, 0, 0)
	printf("SDL open audio requested: format:%d freq:%d channels:%d samples:%d\n", spec.format,
		spec.freq, spec.channels, spec.samples);
	printf("SDL open audio obtained: format:%d freq:%d channels:%d samples:%d\n", obtained.format,
		obtained.freq, obtained.channels, obtained.samples);
#endif
	em_sound_rate = obtained.freq;
// TODO: tsone: should use 60.0988 divisor instead?
	em_sound_frame_samples = em_sound_rate / 60;

	s_Buffer = (buf_t*) FCEU_dmalloc(sizeof(buf_t) * SOUND_BUF_MAX);
	if (!s_Buffer) {
		KillSound();
		return 0;
	}
	s_BufferRead = s_BufferWrite = s_BufferCount = 0;

	SDL_PauseAudio(0);

	FCEUI_SetSoundVolume(150);
	FCEUI_SetSoundQuality(SOUND_QUALITY);
	FCEUI_Sound(em_sound_rate);
	FCEUI_SetTriangleVolume(256);
	FCEUI_SetSquare1Volume(256);
	FCEUI_SetSquare2Volume(256);
	FCEUI_SetNoiseVolume(256);
	FCEUI_SetPCMVolume(256);

#if SDL_VERSION_ATLEAST(2, 0, 0)
	s_initialized = 1;
#endif
	return 1;
}

int GetSoundBufferCount(void)
{
	return s_BufferCount;
}

void WriteSound(int32 *buf, int Count)
{
	extern int EmulationPaused;
	if (EmulationPaused != 0) {
		return;
	}
	if (Count <= 0) {
//		printf("!!!! WriteSound: Unable to write %d samples.\n", Count);
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
#if SDL_VERSION_ATLEAST(2, 0, 0)
		s_Buffer[s_BufferWrite] = 0.5 * sin(testSinePhase * (2.0*M_PI * 440.0/em_sound_rate));
#else
		s_Buffer[s_BufferWrite] = 0xFFF * sin(testSinePhase * (2.0*M_PI * 440.0/em_sound_rate));
#endif
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
#if SDL_VERSION_ATLEAST(2, 0, 0)
		s_Buffer[++i] = buf[++j] / 32768.0;
#else
		s_Buffer[++i] = buf[++j];
#endif
	}
	i = -1;
	while (--Count) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
		s_Buffer[++i] = buf[++j] / 32768.0;
#else
		s_Buffer[++i] = buf[++j];
#endif
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

void SilenceSound(int n)
{ 
	SDL_PauseAudio(n);   
}

int KillSound(void)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	// Don't close audio with SDL2.
#else
	FCEUI_Sound(0);
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	if(s_Buffer) {
		free(s_Buffer);
		s_Buffer = 0;
	}
#endif
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

//	FCEU_DispMessage("Sound volume %d.",0, soundvolume);
}

void FCEUD_SoundToggle(void)
{
	if(s_soundvolumestore) {
		FCEUI_SetSoundVolume(s_soundvolumestore);
		s_soundvolumestore = 0;
//		FCEU_DispMessage("Sound mute off.",0);
	} else {
		s_soundvolumestore = FSettings.SoundVolume;
		FCEUI_SetSoundVolume(0);
//		FCEU_DispMessage("Sound mute on.",0);
	}
}

