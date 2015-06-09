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
#ifndef _EM_H_
#define _EM_H_
#include <SDL.h>
#include "../../driver.h"
#include "../common/args.h"
#include "../common/config.h"
#include "../common/configSys.h"

#if !SDL_VERSION_ATLEAST(2, 0, 0)
#error "compile with emscripten -s USE_SDL=2 option"
#endif

// eoptions variable flags
#define EO_NO8LIM      1
#define EO_SUBASE      2
#define EO_CLIPSIDES   8
#define EO_SNAPNAME    16
#define EO_FOURSCORE	32
#define EO_NOTHROTTLE	64
#define EO_GAMEGENIE	128
#define EO_PAL		256
#define EO_LOWPASS	512
#define EO_AUTOHIDE	1024

// Input device options
#define NUM_INPUT_DEVICES 3
// GamePad defaults
#define GAMEPAD_NUM_DEVICES 4
#define GAMEPAD_NUM_BUTTONS 10
// PowerPad defaults
#define POWERPAD_NUM_DEVICES 2
#define POWERPAD_NUM_BUTTONS 12
// QuizKing defaults
#define QUIZKING_NUM_BUTTONS 6
// HyperShot defaults
#define HYPERSHOT_NUM_BUTTONS 4
// Mahjong defaults
#define MAHJONG_NUM_BUTTONS 21
// TopRider defaults
#define TOPRIDER_NUM_BUTTONS 8
// FTrainer defaults
#define FTRAINER_NUM_BUTTONS 12
// FamilyKeyBoard defaults
#define FAMILYKEYBOARD_NUM_BUTTONS 0x48

#define BUTTC_KEYBOARD          0x00
#define BUTTC_JOYSTICK          0x01
#define BUTTC_MOUSE             0x02

#define FCFGD_GAMEPAD   1
#define FCFGD_POWERPAD  2
#define FCFGD_HYPERSHOT 3
#define FCFGD_QUIZKING  4

// NOTE: tsone: new emscripten input options
// Key code map size (including ctrl, shift, alt, meta key bits at 8,9,10,11)
#define FCEM_KEY_MAP_SIZE (16*256)
// Bitmask for FCEM_Input type and key.
#define FCEM_INPUT_TYPE_MASK 0xF00
#define FCEM_INPUT_KEY_MASK 0x0FF

// Available inputs
enum FCEM_Input {
	FCEM_NULL = 0x000,

	// System inputs
	FCEM_SYSTEM = 0x100,
	FCEM_SYSTEM_RESET = FCEM_SYSTEM,
	FCEM_SYSTEM_THROTTLE,
	FCEM_SYSTEM_PAUSE,
	FCEM_SYSTEM_FRAME_ADVANCE,
	FCEM_SYSTEM_STATE_SAVE,
	FCEM_SYSTEM_STATE_LOAD,
	FCEM_SYSTEM_STATE_SELECT_0,
	FCEM_SYSTEM_STATE_SELECT_1,
	FCEM_SYSTEM_STATE_SELECT_2,
	FCEM_SYSTEM_STATE_SELECT_3,
	FCEM_SYSTEM_STATE_SELECT_4,
	FCEM_SYSTEM_STATE_SELECT_5,
	FCEM_SYSTEM_STATE_SELECT_6,
	FCEM_SYSTEM_STATE_SELECT_7,
	FCEM_SYSTEM_STATE_SELECT_8,
	FCEM_SYSTEM_STATE_SELECT_9,

	// Gamepad inputs 
	FCEM_GAMEPAD = 0x200,
	FCEM_GAMEPAD0 = FCEM_GAMEPAD,
	FCEM_GAMEPAD0_A = FCEM_GAMEPAD0,
	FCEM_GAMEPAD0_B,
	FCEM_GAMEPAD0_SELECT,
	FCEM_GAMEPAD0_START,
	FCEM_GAMEPAD0_UP,
	FCEM_GAMEPAD0_DOWN,
	FCEM_GAMEPAD0_LEFT,
	FCEM_GAMEPAD0_RIGHT,
	FCEM_GAMEPAD0_TURBO_A,
	FCEM_GAMEPAD0_TURBO_B,
	FCEM_GAMEPAD1,
	FCEM_GAMEPAD1_A = FCEM_GAMEPAD1,
	FCEM_GAMEPAD1_B,
	FCEM_GAMEPAD1_SELECT,
	FCEM_GAMEPAD1_START,
	FCEM_GAMEPAD1_UP,
	FCEM_GAMEPAD1_DOWN,
	FCEM_GAMEPAD1_LEFT,
	FCEM_GAMEPAD1_RIGHT,
	FCEM_GAMEPAD1_TURBO_A,
	FCEM_GAMEPAD1_TURBO_B,
	FCEM_GAMEPAD2,
	FCEM_GAMEPAD2_A = FCEM_GAMEPAD2,
	FCEM_GAMEPAD2_B,
	FCEM_GAMEPAD2_SELECT,
	FCEM_GAMEPAD2_START,
	FCEM_GAMEPAD2_UP,
	FCEM_GAMEPAD2_DOWN,
	FCEM_GAMEPAD2_LEFT,
	FCEM_GAMEPAD2_RIGHT,
	FCEM_GAMEPAD2_TURBO_A,
	FCEM_GAMEPAD2_TURBO_B,
	FCEM_GAMEPAD3,
	FCEM_GAMEPAD3_A = FCEM_GAMEPAD3,
	FCEM_GAMEPAD3_B,
	FCEM_GAMEPAD3_SELECT,
	FCEM_GAMEPAD3_START,
	FCEM_GAMEPAD3_UP,
	FCEM_GAMEPAD3_DOWN,
	FCEM_GAMEPAD3_LEFT,
	FCEM_GAMEPAD3_RIGHT,
	FCEM_GAMEPAD3_TURBO_A,
	FCEM_GAMEPAD3_TURBO_B,

	FCEM_INPUT_COUNT
};

// Audio options
// NOTE: tsone: both SOUND_BUF_MAX and SOUND_HW_BUF_MAX must be power of two!
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
#define SOUND_BUF_MASK		(SOUND_BUF_MAX-1)


// The rate of output and emulated (internal) audio (frequency, in Hz).
extern int em_sound_rate;
// Number of audio samples per frame. Actually NTSC divisor is 60.0988, but since this is used as divisor
// to find out number of frames to skip, higher value will avoid audio buffer overflow.
extern int em_sound_frame_samples;

extern int eoptions;

extern int NoWaiting;

extern bool replaceP2StartWithMicrophone;

// TODO: tsone: unsupported peripherals
#if PERI
extern const char *PowerPadNames[POWERPAD_NUM_BUTTONS];
extern const char *DefaultPowerPadDevice[POWERPAD_NUM_DEVICES];
extern const int DefaultPowerPad[POWERPAD_NUM_DEVICES][POWERPAD_NUM_BUTTONS];

extern const char *QuizKingNames[QUIZKING_NUM_BUTTONS];
extern const char *DefaultQuizKingDevice;
extern const int DefaultQuizKing[QUIZKING_NUM_BUTTONS];

extern const char *HyperShotNames[HYPERSHOT_NUM_BUTTONS];
extern const char *DefaultHyperShotDevice;
extern const int DefaultHyperShot[HYPERSHOT_NUM_BUTTONS];

extern const char *MahjongNames[MAHJONG_NUM_BUTTONS];
extern const char *DefaultMahjongDevice;
extern const int DefaultMahjong[MAHJONG_NUM_BUTTONS];

extern const char *TopRiderNames[TOPRIDER_NUM_BUTTONS];
extern const char *DefaultTopRiderDevice;
extern const int DefaultTopRider[TOPRIDER_NUM_BUTTONS];

extern const char *FTrainerNames[FTRAINER_NUM_BUTTONS];
extern const char *DefaultFTrainerDevice;
extern const int DefaultFTrainer[FTRAINER_NUM_BUTTONS];

extern const char *FamilyKeyBoardNames[FAMILYKEYBOARD_NUM_BUTTONS];
extern const char *DefaultFamilyKeyBoardDevice;
extern const int DefaultFamilyKeyBoard[FAMILYKEYBOARD_NUM_BUTTONS];
#endif //PERI

int InitSound();
int IsSoundInitialized();
void WriteSound(int32 *Buffer, int Count);
int KillSound(void);
int GetSoundBufferCount(void);
void SilenceSound(int option);

int InitVideo();
int KillVideo();
void BlitScreen();

int LoadGame(const char *path);
int CloseGame(void);
uint64 FCEUD_GetTime();

void PtoV(int *x, int *y);
bool FCEUD_ShouldDrawInputAids();
bool FCEUI_AviDisableMovieMessages();
bool FCEUI_AviEnableHUDrecording();
void FCEUI_SetAviEnableHUDrecording(bool enable);
bool FCEUI_AviDisableMovieMessages();
void FCEUI_SetAviDisableMovieMessages(bool disable);

Config *InitConfig(void);
void UpdateEMUCore(Config *);

void ParseGIInput(FCEUGI *GI);
int ButtonConfigBegin();
void ButtonConfigEnd();

void FCEUD_UpdateInput(void);


#endif // _EM_H_
