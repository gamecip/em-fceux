/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

/// \file
/// \brief Handles sound emulation using the SDL.
#include "sdl.h"

#include "../common/configSys.h"
#include "../../utils/memory.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

// TODO: tsone: define to output test sine tone
#define TEST_SINE_AT_FILL	0
#define TEST_SINE_AT_WRITE	0

// Note: Emulated sound is mono.
#define SOUND_RATE 	22050	// Sampling rate of both FCEUX and audio interface (in Hz).
#define BUFFER_SIZE	(6*256)	// Internal sound buffer size (in samples).
#define HW_BUFFER_SIZE	256	// HW sound buffer size (in samples).
// TODO: tsone: Will it work with higher quality?
#define SOUND_QUALITY	0	// FCEUX sound quality setting, 0: lowest.

extern Config *g_config;

static int *s_Buffer = 0;
static int s_BufferMax = 0;
static int s_BufferRead = 0;
static int s_BufferWrite = 0;
static int s_BufferCount = 0;

// TODO: tsone: required for sound muting
#ifndef EMSCRIPTEN
static int s_mute = 0;
#endif

#if TEST_SINE_AT_FILL || TEST_SINE_AT_WRITE
#include <math.h>
static double testSinePhase = 0.0;
#endif

/**
 * Callback from the SDL to get and play audio data.
 */
static void
fillaudio(void *udata,
			uint8 *stream,
			int len)
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

#ifndef EMSCRIPTEN
	int16 *tmps = (int16*)stream;
	len >>= 1;
	while(len) {
		int16 sample = 0;
		if(s_BufferCount) {
			sample = s_Buffer[s_BufferRead];
			s_BufferRead = (s_BufferRead + 1) % s_BufferMax;
			s_BufferCount--;
		} else {
			sample = 0;
		}

		*tmps = sample;
		tmps++;
		len--;
	}
#elif 0
	len >>= 1;
	for (int i = 0; i < len; i++) {
		int16 sample = 0;
		if(s_BufferCount) {
			sample = s_Buffer[s_BufferRead];
			s_BufferRead = (s_BufferRead + 1) % s_BufferMax;
			s_BufferCount--;
		} else {
			sample = 0;
		}

		((int16*)stream)[i] = sample;
	}
#elif 0
	int16* str = (int16*) stream;
    len >>= 1;
//    if (s_BufferCount < len) {
//        printf("Sound buffer exhausted (has %d samples, needs %d).\n", s_BufferCount, len);
//    }
    int i = -1;
    int j = (s_BufferCount > len) ? len : s_BufferCount;
	int k = len - j;
    s_BufferCount -= j;
	++j;
	++k;
	while (--j) {
		str[++i] = s_Buffer[s_BufferRead];
		s_BufferRead = (s_BufferRead + 1) % s_BufferMax;
	}
	while (--k) {
		str[++i] = 0;
	}
#else
	int16* str = (int16*) stream;
    len >>= 1;
//    if (s_BufferCount < len) {
//        printf("Sound buffer exhausted (has %d samples, needs %d).\n", s_BufferCount, len);
//    }
    int i = 0;
    int d = (s_BufferCount > len) ? len : s_BufferCount;
	for (; i < d; ++i) {
		str[i] = s_Buffer[s_BufferRead];
		s_BufferRead = (s_BufferRead + 1) % s_BufferMax;
	}
	for (; i < len; ++i) {
		str[i] = 0;
	}
    s_BufferCount -= d;
#endif

#endif // TEST_SINE_AT_FILL
}

/**
 * Initialize the audio subsystem.
 */
int
InitSound()
{
	int soundvolume, soundtrianglevolume, soundsquare1volume, soundsquare2volume, soundnoisevolume, soundpcmvolume;
	SDL_AudioSpec spec;

	memset(&spec, 0, sizeof(spec));
	if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
		puts(SDL_GetError());
		KillSound();
		return 0;
	}
	char driverName[8];
#if SDL_VERSION_ATLEAST(2, 0, 0)
	// TODO - SDL 2
#else
	SDL_AudioDriverName(driverName, 8);
	fprintf(stderr, "Loading SDL sound with %s driver...\n", driverName);
#endif

	soundvolume = 150;
	soundtrianglevolume = 256;
	soundsquare1volume = 256;
	soundsquare2volume = 256;
	soundnoisevolume = 256;
	soundpcmvolume = 256;

	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.freq = SOUND_RATE;
	spec.samples = HW_BUFFER_SIZE;

	spec.callback = fillaudio;
	spec.userdata = 0;

	s_BufferMax = BUFFER_SIZE;

	// For safety, set a bare minimum:
	if (s_BufferMax < HW_BUFFER_SIZE * 2) {
		s_BufferMax = HW_BUFFER_SIZE * 2;
	}

	printf("Sound buffersize: %d (requested: %d, HW: %d)\n", s_BufferMax, BUFFER_SIZE, spec.samples);

	s_Buffer = (int *)FCEU_dmalloc(sizeof(int) * s_BufferMax);
	if (!s_Buffer)
		return 0;
	s_BufferRead = s_BufferWrite = s_BufferCount = 0;

	if(SDL_OpenAudio(&spec, 0))
	{
		puts(SDL_GetError());
		KillSound();
		return 0;
    }
	SDL_PauseAudio(0);

	FCEUI_SetSoundVolume(soundvolume);
	FCEUI_SetSoundQuality(SOUND_QUALITY);
	FCEUI_Sound(SOUND_RATE);
	FCEUI_SetTriangleVolume(soundtrianglevolume);
	FCEUI_SetSquare1Volume(soundsquare1volume);
	FCEUI_SetSquare2Volume(soundsquare2volume);
	FCEUI_SetNoiseVolume(soundnoisevolume);
	FCEUI_SetPCMVolume(soundpcmvolume);
	return 1;
}


/**
 * Returns the size of the audio buffer.
 */
int GetMaxSound(void)
{
	return(s_BufferMax);
}

/**
 * Returns the amount of free space in the audio buffer.
 */
int GetWriteSound(void)
{
    if (s_BufferMax < s_BufferCount)
    {
        printf("!!!! GetWriteSound: %4d < %4d\n", s_BufferMax, s_BufferCount);
    }
	return(s_BufferMax - s_BufferCount);
}

/**
 * Send a sound clip to the audio subsystem.
 */
void
WriteSound(int32 *buf,
           int Count)
{
	extern int EmulationPaused;
	if (EmulationPaused != 0) {
		return;
	}
	if (Count <= 0) {
		printf("!!!! WriteSound: Unable to write %d samples.\n", Count);
		return;
	}
	int freeCount = GetWriteSound();
	if (Count > freeCount) {
		printf("!!!! WriteSound: Tried to write %d samples while buffer has %d free.\n", Count, freeCount);
		Count = freeCount;
	}

#if TEST_SINE_AT_WRITE

	// Sine wave test outputs a 440Hz tone.
	s_BufferCount += Count;
	++Count;
	while (--Count) {
		s_Buffer[s_BufferWrite] = 0xFFF * sin(testSinePhase * (2.0*M_PI * 440.0/SOUND_RATE));
		s_BufferWrite = (s_BufferWrite + 1) % s_BufferMax;
		++testSinePhase;
	}

#else
        if (Count > s_BufferMax - s_BufferCount)
        {
            printf("Had to limit sound: %d (got: %d)\n", s_BufferMax - s_BufferCount, Count);
            Count = s_BufferMax - s_BufferCount;
        }
//        printf("Bufsize: %5d, Count: %5d\n", s_BufferMax - s_BufferCount, Count);
	for (int i = 0; i < Count; ++i) {
		s_Buffer[s_BufferWrite] = buf[i];
		s_BufferWrite = (s_BufferWrite + 1) % s_BufferMax;
	}
//		SDL_LockAudio();
	s_BufferCount += Count;
//		SDL_UnlockAudio();
#endif // TEST_SINE_AT_WRITE
}

/**
 * Pause (1) or unpause (0) the audio output.
 */
void
SilenceSound(int n)
{ 
	SDL_PauseAudio(n);   
}

/**
 * Shut down the audio subsystem.
 */
int
KillSound(void)
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


/**
 * Adjust the volume either down (-1), up (1), or to the default (0).
 * Unmutes if mute was active before.
 */
void
FCEUD_SoundVolumeAdjust(int n)
{
// TODO: tsone: set way to adjust volume?
#ifndef EMSCRIPTEN
	int soundvolume;
	g_config->getOption("SDL.SoundVolume", &soundvolume);

	switch(n) {
	case -1:
		soundvolume -= 10;
		if(soundvolume < 0) {
			soundvolume = 0;
		}
		break;
	case 0:
		soundvolume = 100;
		break;
	case 1:
		soundvolume += 10;
		if(soundvolume > 150) {
			soundvolume = 150;
		}
		break;
	}

	s_mute = 0;
	FCEUI_SetSoundVolume(soundvolume);
	g_config->setOption("SDL.SoundVolume", soundvolume);

	FCEU_DispMessage("Sound volume %d.",0, soundvolume);
#endif
}

/**
 * Toggles the sound on or off.
 */
void
FCEUD_SoundToggle(void)
{
// TODO: tsone: set way to toggle sound?
#ifndef EMSCRIPTEN
	if(s_mute) {
		int soundvolume;
		g_config->getOption("SDL.SoundVolume", &soundvolume);

		s_mute = 0;
		FCEUI_SetSoundVolume(soundvolume);
		FCEU_DispMessage("Sound mute off.",0);
	} else {
		s_mute = 1;
		FCEUI_SetSoundVolume(0);
		FCEU_DispMessage("Sound mute on.",0);
	}
#endif
}
