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
#ifndef _EM_INPUT_H_
#define _EM_INPUT_H_
#include "../common/configSys.h"
#include <SDL.h>


#define MAXBUTTCONFIG   4
typedef struct {
	uint8  ButtType[MAXBUTTCONFIG];
	uint8  DeviceNum[MAXBUTTCONFIG];
#if SDL_VERSION_ATLEAST(2, 0, 0)
	uint32 ButtonNum[MAXBUTTCONFIG];
#else
	uint16 ButtonNum[MAXBUTTCONFIG];
#endif
	uint32 NumC;
	//uint64 DeviceID[MAXBUTTCONFIG];	/* TODO */
} ButtConfig;


extern int NoWaiting;
extern CFGSTRUCT InputConfig[];
extern ARGPSTRUCT InputArgs[];
void ParseGIInput(FCEUGI *GI);
int ButtonConfigBegin();
void ButtonConfigEnd();
void ConfigButton(char *text, ButtConfig *bc);
int DWaitButton(const uint8 *text, ButtConfig *bc, int wb);

#define BUTTC_KEYBOARD          0x00
#define BUTTC_JOYSTICK          0x01
#define BUTTC_MOUSE             0x02

#define FCFGD_GAMEPAD   1
#define FCFGD_POWERPAD  2
#define FCFGD_HYPERSHOT 3
#define FCFGD_QUIZKING  4

void InitInputInterface(void);
void InputUserActiveFix(void);

extern bool replaceP2StartWithMicrophone;
extern ButtConfig GamePadConfig[4][10];
//extern ButtConfig powerpadsc[2][12];
//extern ButtConfig QuizKingButtons[6];
//extern ButtConfig FTrainerButtons[12];

#define DTestButtonJoy(a_) 0

void FCEUD_UpdateInput(void);

void UpdateInput(Config *config);
void InputCfg(const std::string &);

#endif // _EM_INPUT_H_
