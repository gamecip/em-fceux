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

#include <cstdio>
#include <SDL.h>
#include "em.h"
#include "../../utils/memory.h"
#include "../../fceu.h"

// NOTE: tsone: define to output test sine tone
#define TEST_SINE_AT_FILL	0
#define TEST_SINE_AT_WRITE	0

static int *s_Buffer = 0;
static int s_BufferRead = 0;
static int s_BufferWrite = 0;
static int s_BufferCount = 0;

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
	len >>= 1;
	int16* str = (int16*) stream;
	for (int i = 0; i < len; ++i) {
		str[i] = 0xFFF * sin(testSinePhase * (2.0*M_PI * 440.0/SOUND_RATE));
		++testSinePhase;
	}
	s_BufferCount -= len;

#else

#if 1
// NOTE: tsone: optimized version of the below
	int16* str = (int16*) stream;
	len >>= 1;
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
	SDL_AudioSpec spec;

	memset(&spec, 0, sizeof(spec));
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		puts(SDL_GetError());
		KillSound();
		return 0;
	}
	char driverName[8];
	SDL_AudioDriverName(driverName, 8);
	printf("Loading SDL sound with %s driver...\n", driverName);

	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.freq = SOUND_RATE;
	spec.samples = SOUND_HW_BUF_MAX;
	spec.callback = fillaudio;
	spec.userdata = 0;

//	printf("Sound buffersize: %d, HW: %d)\n", SOUND_BUF_MAX, spec.samples);

	s_Buffer = (int *)FCEU_dmalloc(sizeof(int) * SOUND_BUF_MAX);
	if (!s_Buffer) {
		return 0;
	}
	s_BufferRead = s_BufferWrite = s_BufferCount = 0;

	if(SDL_OpenAudio(&spec, 0)) {
		puts(SDL_GetError());
		KillSound();
		return 0;
	}
	SDL_PauseAudio(0);

	FCEUI_SetSoundVolume(150);
	FCEUI_SetSoundQuality(SOUND_QUALITY);
	FCEUI_Sound(SOUND_RATE);
	FCEUI_SetTriangleVolume(256);
	FCEUI_SetSquare1Volume(256);
	FCEUI_SetSquare2Volume(256);
	FCEUI_SetNoiseVolume(256);
	FCEUI_SetPCMVolume(256);
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
		s_Buffer[s_BufferWrite] = 0xFFF * sin(testSinePhase * (2.0*M_PI * 440.0/SOUND_RATE));
		s_BufferWrite = (s_BufferWrite + 1) & SOUND_BUF_MASK;
		++testSinePhase;
	}

#elif 1
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
		s_Buffer[++i] = buf[++j];
	}
	i = -1;
	while (--Count) {
		s_Buffer[++i] = buf[++j];
	}

#else

	for (int i = 0; i < Count; ++i) {
		s_Buffer[s_BufferWrite] = buf[i];
		s_BufferWrite = (s_BufferWrite + 1) & SOUND_BUF_MASK;
	}

	s_BufferCount += Count;

#endif // TEST_SINE_AT_WRITE
}

void SilenceSound(int n)
{ 
	SDL_PauseAudio(n);   
}

int KillSound(void)
{
	FCEUI_Sound(0);
	SDL_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
	if(s_Buffer) {
		free((void *)s_Buffer);
		s_Buffer = 0;
	}
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

