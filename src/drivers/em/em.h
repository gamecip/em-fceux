#ifndef __FCEU_SDL_H
#define __FCEU_SDL_H

#include "main.h"
#include "dface.h"
#include "input.h"

// NOTE: tsone: both SOUND_BUF_MAX and SOUND_HW_BUF_MAX must be power of two!
#if 1
#define SOUND_RATE		22050
#define SOUND_BUF_MAX		4096
#define SOUND_HW_BUF_MAX	512
#define SOUND_QUALITY		0
#else
#define SOUND_RATE		44100
#define SOUND_BUF_MAX		8192
#define SOUND_HW_BUF_MAX	1024
#define SOUND_QUALITY		1
#endif
#define SOUND_BUF_MASK		(SOUND_BUF_MAX-1)

const int INVALID_STATE = 99;

extern int isloaded;

int LoadGame(const char *path);
int CloseGame(void);
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);
uint64 FCEUD_GetTime();

#endif
