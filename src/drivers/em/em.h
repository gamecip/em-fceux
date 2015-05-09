/* FCE Ultra - NES/Famicom Emulator
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
#ifndef _EM_H_
#define _EM_H_
#include "main.h"
#include "../common/args.h"
#include "../common/config.h"
#include "input.h"

// NOTE: tsone: both SOUND_BUF_MAX and SOUND_HW_BUF_MAX must be power of two!
#if 1
#if SDL_VERSION_ATLEAST(2, 0, 0)
// NOTE: tsone: for 32-bit floating-point audio (SDL2 port)
#define SOUND_RATE		48000
#define SOUND_BUF_MAX		8192
#define SOUND_HW_BUF_MAX	2048
#else
// NOTE: tsone: for SDL1.x 16-bit integer audio (SDL1)
#define SOUND_RATE		22050
#define SOUND_BUF_MAX		4096
#define SOUND_HW_BUF_MAX	512
#endif
#define SOUND_QUALITY		0
#else
#define SOUND_RATE		44100
#define SOUND_BUF_MAX		8192
#define SOUND_HW_BUF_MAX	1024
#define SOUND_QUALITY		1
#endif
#define SOUND_BUF_MASK		(SOUND_BUF_MAX-1)

const int INVALID_STATE = 99;

extern CFGSTRUCT DriverConfig[];
extern ARGPSTRUCT DriverArgs[];


extern int isloaded;
// The rate of output and emulated (internal) audio (frequency, in Hz).
extern int em_sound_rate;
// Number of audio samples per frame. Actually NTSC divisor is 60.0988, but since this is used as divisor
// to find out number of frames to skip, higher value will avoid audio buffer overflow.
extern int em_sound_frame_samples;


void DoDriverArgs(void);

int InitSound();
void WriteSound(int32 *Buffer, int Count);
int KillSound(void);
int GetSoundBufferCount(void);

// TODO: tsone: huh? remove?
void SilenceSound(int s);

#define InitJoysticks() 1
#define KillJoysticks() 0
uint32 *GetJSOr(void);

int InitVideo(FCEUGI *gi);
int KillVideo(void);
void BlitScreen(uint8 *XBuf);

int LoadGame(const char *path);

int FCEUD_NetworkConnect(void);

int LoadGame(const char *path);
int CloseGame(void);
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);
uint64 FCEUD_GetTime();

#endif // _EM_H_
