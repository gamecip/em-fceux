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
#include "main.h"
#include "input.h"
#include "config.h"
#include <SDL.h>
#include "em-video.h"
#include "em.h"
#include "throttle.h"
#include "../common/cheat.h"
#include "../../movie.h"
#include "../../fceu.h"
#include "../../driver.h"
#include "../../utils/xstring.h"
#include <html5.h>
#include <cstring>
#include <cstdio>


int NoWaiting = 1;
extern Config *g_config;
extern bool frameAdvanceLagSkip, lagCounterDisplay;


/* UsrInputType[] is user-specified.  CurInputType[] is current
        (game loading can override user settings)
*/
static int UsrInputType[NUM_INPUT_DEVICES];
static int CurInputType[NUM_INPUT_DEVICES];
static int cspec = 0;

extern int gametype;

int MouseData[3] = { 0, 0, 0 };

/**
 * Necessary for proper GUI functioning (configuring when a game isn't loaded).
 */
void
InputUserActiveFix ()
{
	int x;
	for (x = 0; x < 3; x++)
	{
		CurInputType[x] = UsrInputType[x];
	}
}

/**
 * Parse game information and configure the input devices accordingly.
 */
void
ParseGIInput (FCEUGI * gi)
{
	gametype = gi->type;

	CurInputType[0] = UsrInputType[0];
	CurInputType[1] = UsrInputType[1];
	CurInputType[2] = UsrInputType[2];

	if (gi->input[0] >= 0)
	{
		CurInputType[0] = gi->input[0];
	}
	if (gi->input[1] >= 0)
	{
		CurInputType[1] = gi->input[1];
	}
	if (gi->inputfc >= 0)
	{
		CurInputType[2] = gi->inputfc;
	}
	cspec = gi->cspecial;
}

#if PERI
static uint8 QuizKingData = 0;
static uint8 HyperShotData = 0;
static uint32 MahjongData = 0;
static uint32 FTrainerData = 0;
static uint8 TopRiderData = 0;
static uint8 BWorldData[1 + 13 + 1];

static void UpdateFKB (void);
#endif
static void UpdateGamepad (void);
#if PERI
static void UpdateQuizKing (void);
static void UpdateHyperShot (void);
static void UpdateMahjong (void);
static void UpdateFTrainer (void);
static void UpdateTopRider (void);
#endif

static uint32 JSreturn = 0;

#include "keyscan.h"
static uint8 *g_keyState = 0;
static int DIPS = 0;

static uint8 keyonce[MKK_COUNT];
#define KEY(__a) g_keyState[MKK(__a)]

static int
_keyonly (int a)
{
	// check for valid key
	if (a > SDLK_LAST + 1 || a < 0)
		return 0;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	if (g_keyState[SDL_GetScancodeFromKey (a)])
#else
	if (g_keyState[a])
#endif
	{
		if (!keyonce[a])
		{
			keyonce[a] = 1;
			return 1;
		}
	} 
	else {
		keyonce[a] = 0;
	}
	return 0;
}

#define keyonly(__a) _keyonly(MKK(__a))

#if PERI
static int g_fkbEnabled = 0;
#endif //PERI

// TODO: tsone: dummy functions
void FCEUD_MovieRecordTo() {}

void FCEUD_SaveStateAs() {}

void FCEUD_LoadStateFrom() {}

/**
* Hook for transformer board
*/
unsigned int *GetKeyboard(void)                                                     
{
	int size = 256;
#if SDL_VERSION_ATLEAST(1, 3, 0)
	Uint8* keystate = (Uint8*)SDL_GetKeyboardState(&size);
#else
	Uint8* keystate = SDL_GetKeyState(&size);
#endif
	return (unsigned int*)(keystate);
}

/**
 * Parse keyboard commands and execute accordingly.
 */
static void KeyboardCommands ()
{
	// get the keyboard input
	g_keyState = (Uint8*)SDL_GetKeyboardState (NULL);

// TODO: tsone: family keyboard toggle not working
#ifndef EMSCRIPTEN
	// check if the family keyboard is enabled
	if (CurInputType[2] == SIFC_FKB)
	{
		if (keyonly (SCROLLLOCK))
			{
				g_fkbEnabled ^= 1;
				FCEUI_DispMessage ("Family Keyboard %sabled.", 0,
				g_fkbEnabled ? "en" : "dis");
			}
		SDL_WM_GrabInput (g_fkbEnabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
		if (g_fkbEnabled)
		{
			return;
		}
	}
#endif

// TODO: tsone: shift and alt are not working
#ifndef EMSCRIPTEN
	int is_shift, is_alt;

	if (g_keyState[SDLK_LSHIFT] || g_keyState[SDLK_RSHIFT])
	is_shift = 1;
  else
	is_shift = 0;
	if (g_keyState[SDLK_LALT] || g_keyState[SDLK_RALT])
	{
		is_alt = 1;
	}
	else
		is_alt = 0;
#endif

// TODO: tsone: not yet implemented
#ifndef EMSCRIPTEN
	if (_keyonly (Hotkeys[HK_TOGGLE_BG]))
	{
		if (is_shift)
		{
			FCEUI_SetRenderPlanes (true, false);
		}
		else
		{
			FCEUI_SetRenderPlanes (true, true);
		}
	}
#endif

// TODO: tsone: not yet implemented
#ifndef EMSCRIPTEN
		// Famicom disk-system games
	if (gametype == GIT_FDS)
	{
		if (_keyonly (Hotkeys[HK_FDS_SELECT]))
		{
			FCEUI_FDSSelect ();
		}
		if (_keyonly (Hotkeys[HK_FDS_EJECT]))
		{
			FCEUI_FDSInsert ();
		}
	}
#endif

	// if not NES Sound Format
	if (gametype != GIT_NSF)
	{
		if (_keyonly (SDLK_F5))
		{
			FCEUI_SaveState(NULL);
		}
		if (_keyonly (SDLK_F7))
		{
			FCEUI_LoadState(NULL);
		}
	}


	if (_keyonly (SDLK_EQUALS))
	{
		DecreaseEmulationSpeed ();
	}

	if (_keyonly (SDLK_MINUS))
	{
		IncreaseEmulationSpeed ();
	}

// TODO: tsone: allow input displaying?
#ifndef EMSCRIPTEN
	if (_keyonly (SDLK_COMMA))
	{
		FCEUI_ToggleInputDisplay ();
	}
#endif

	if (_keyonly (SDLK_p))
	{
		//FCEUI_ToggleEmulationPause(); 
		// use the wrapper function instead of the fceui function directly
		// so we can handle cursor grabbage
	FCEUI_ToggleEmulationPause ();
	}

	// Toggle throttling
	NoWaiting &= ~1;
	if (g_keyState[SDLK_TAB])
	{
		NoWaiting |= 1;
	}

	static bool frameAdvancing = false;
	if (g_keyState[SDLK_BACKSLASH])
	{
		if (frameAdvancing == false)
		{
			FCEUI_FrameAdvance ();
			frameAdvancing = true;
		}
	}
	else
	{
		if (frameAdvancing)
		{
			FCEUI_FrameAdvanceEnd ();
			frameAdvancing = false;
		}
	}

	if (_keyonly (SDLK_r))
	{
		FCEUI_ResetNES ();
	}

// TODO: tsone: power off by some key or UI?
#ifndef EMSCRIPTEN
	//if(_keyonly(Hotkeys[HK_POWER])) {
	//    FCEUI_PowerNES();
	//}
	if (_keyonly (Hotkeys[HK_QUIT]))
	{
		CloseGame ();
	}
	else
#endif

	const int statekeys[] = {
		SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
		SDLK_6, SDLK_7, SDLK_8, SDLK_9
        };
	for (int i = 0; i < 10; i++)
		if (_keyonly (statekeys[i]))
		{
			FCEUI_SelectState (i, 1);
		}

	if (_keyonly (SDLK_PAGEUP))
	{
		FCEUI_SelectStateNext (1);
	}

	if (_keyonly (SDLK_PAGEDOWN))
	{
		FCEUI_SelectStateNext (-1);
	}

	if (_keyonly (SDLK_DELETE))
	{
		frameAdvanceLagSkip ^= 1;
		FCEUI_DispMessage ("Skipping lag in Frame Advance %sabled.", 0,
		frameAdvanceLagSkip ? "en" : "dis");
	}

	if (_keyonly (SDLK_SLASH))
	{
		lagCounterDisplay ^= 1;
	}

	// VS Unisystem games
	if (gametype == GIT_VSUNI)
	{
		// insert coin
		if (_keyonly (SDLK_F6))
			FCEUI_VSUniCoin ();

		// toggle dipswitch display
		if (_keyonly (SDLK_F8))
		{
			DIPS ^= 1;
			FCEUI_VSUniToggleDIPView ();
		}
		if (!(DIPS & 1))
			goto DIPSless;

		// toggle the various dipswitches
		for(int i=1; i<=8;i++)
		{
			if(keyonly(i))
				FCEUI_VSUniToggleDIP(i-1);
		}
	}
	else
	{
		if (_keyonly (SDLK_EQUALS))
			FCEUI_NTSCDEC ();
		if (_keyonly (SDLK_MINUS))
			FCEUI_NTSCINC ();

// TODO: tsone: disabled barcode
#ifdef EMSCRIPTEN
		DIPSless:
		for(int i=0; i<10;i++)
		{
			keyonly(i);
		}
#else  // !EMSCRIPTEN
		static uint8 bbuf[32];
		static int bbuft;
		static int barcoder = 0;

		if ((CurInputType[2] == SIFC_BWORLD) || (cspec == SIS_DATACH))
		{
			if (keyonly (F8))
			{
				barcoder ^= 1;
				if (!barcoder)
				{
					if (CurInputType[2] == SIFC_BWORLD)
					{
						strcpy ((char *) &BWorldData[1], (char *) bbuf);
						BWorldData[0] = 1;
					}
					else
					{
						FCEUI_DatachSet (bbuf);
					}
					FCEUI_DispMessage ("Barcode Entered", 0);
				}
				else
				{
					bbuft = 0;
					FCEUI_DispMessage ("Enter Barcode", 0);
				}
			}
		}
		else
		{
			barcoder = 0;
		}

		DIPSless:
		for(int i=0; i<10;i++)
		{
			if (keyonly (i)) {
				if(barcoder) {
					if(bbuft < 13) {
						bbuf[bbuft++] = '0' + i;
						bbuf[bbuft] = 0;
					}
					FCEUI_DispMessage("Barcode: %s",0, bbuf);
				}
			}
		}

#endif // EMSCRIPTEN
	}
}

/**
 * Handles outstanding SDL events.
 */
static void
UpdatePhysicalInput ()
{
	SDL_Event event;

	// loop, handling all pending events
	while (SDL_PollEvent (&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
			  CloseGame ();
			  puts ("Quit");
			  break;
			default:
				break;
		}
	}
	//SDL_PumpEvents();
}

/**
 * Tests to see if a specified button is currently pressed.
 */
static int
DTestButton (ButtConfig * bc)
{
	int x;

	for (x = 0; x < bc->NumC; x++)
	{
		if (bc->ButtType[x] == BUTTC_KEYBOARD)
		{
#if SDL_VERSION_ATLEAST(2, 0, 0)
			if (g_keyState[SDL_GetScancodeFromKey (bc->ButtonNum[x])])
			{
#else
			if (g_keyState[bc->ButtonNum[x]])
			{
#endif
				return 1;
			}
		}
		else if (bc->ButtType[x] == BUTTC_JOYSTICK)
		{
			if (DTestButtonJoy (bc))
			{
				return 1;
			}
		}
    }
  return 0;
}


#define MK(x)       {{BUTTC_KEYBOARD},{0},{MKK(x)},1}
#define MK2(x1,x2)  {{BUTTC_KEYBOARD},{0},{MKK(x1),MKK(x2)},2}
#define MKZ()       {{0},{0},{0},0}
#define GPZ()       {MKZ(), MKZ(), MKZ(), MKZ()}

ButtConfig GamePadConfig[4][10] = {
#if SDL_VERSION_ATLEAST(2, 0, 0)
/* Gamepad 1 */
	{MK (KP_3), MK (KP_2), MK (SLASH), MK (ENTER),
	MK (W), MK (Z), MK (A), MK (S), MKZ (), MKZ ()},
#else
	/* Gamepad 1 */
	{MK (KP3), MK (KP2), MK (SLASH), MK (ENTER),
	MK (W), MK (Z), MK (A), MK (S), MKZ (), MKZ ()},
#endif

	/* Gamepad 2 */
	GPZ (),

	/* Gamepad 3 */
	GPZ (),

	/* Gamepad 4 */
	GPZ ()
};

// TODO: tsone: this was added as new way to test buttons
// idx 0..9 matches: a, b, select, start, up, down, left, right, turbo a, turbo b
static int EmscriptenTestButton(const EmscriptenGamepadEvent *p, int idx)
{
    if (!p->connected) {
        return 0;
    }
    if (idx >= 8) {
        // Turbo A, Turbo B.
        idx = idx - 4;
        return ((idx < p->numButtons) && (p->digitalButton[idx] || (p->analogButton[idx] > 0.5)));
    } else if (idx >= 4) {
        // Up, down, left, right.
        const int positive_axis = idx & 1;
        idx = !((idx & 0x02) >> 1);
        if (idx < p->numAxes) {
            if (positive_axis) { 
                return p->axis[idx] > 0.5;
            } else { 
                return p->axis[idx] < -0.5;
            }
        } else {
            return 0;
        }
    } else { 
        // A, B, Select, Start.
        return ((idx < p->numButtons) && (p->digitalButton[idx] || (p->analogButton[idx] > 0.5)));
    }
}

/**
 * Update the status of the gamepad input devices.
 */
static void
UpdateGamepad(void)
{
	static int rapid = 0;
	uint32 JS = 0;
	int x;
	int wg;

	rapid ^= 1;

	int opposite_dirs;
	g_config->getOption("SDL.Input.EnableOppositeDirectionals", &opposite_dirs);

// TODO: tsone: Read gamepads.
    // Four possibly connected joysticks/gamepads are read here, each matching a NES gamepad.
    EmscriptenGamepadEvent gamepads[4];
    for (int i = 0; i < 4; i++) {
        if (EMSCRIPTEN_RESULT_SUCCESS != emscripten_get_gamepad_status(i, &gamepads[i])) {
            // Set disconnected if query failed.
            gamepads[i].connected = 0;
        }
    }

	// go through each of the four game pads
	for (wg = 0; wg < 4; wg++)
	{
		bool left = false;
		bool up = false;
		// a, b, select, start, up, down, left, right
		for (x = 0; x < 8; x++)
		{
			if (DTestButton(&GamePadConfig[wg][x]) || EmscriptenTestButton(&gamepads[wg], x)) {
				if(opposite_dirs == 0)
				{
					// test for left+right and up+down
					if(x == 4){
						up = true;
                    }
					if((x == 5) && (up == true)){
						continue;
                    }
					if(x == 6){
						left = true;
                    }
					if((x == 7) && (left == true)){
						continue;
                    }
				}
				JS |= (1 << x) << (wg << 3);
			}
		}

        int four_button_exit;
        g_config->getOption("SDL.ABStartSelectExit", &four_button_exit);
        // if a+b+start+select is pressed, exit
        if (four_button_exit && JS == 15) {
            FCEUI_printf("all buttons pressed, exiting\n");
            CloseGame();
            FCEUI_Kill();
            exit(0);
        }

		// rapid-fire a, rapid-fire b
		if (rapid)
		{
			for (x = 0; x < 2; x++)
			{
    			if (DTestButton(&GamePadConfig[wg][8+x]) || EmscriptenTestButton(&gamepads[wg], 8+x)) {
					JS |= (1 << x) << (wg << 3);
				}
			}
		}
	}

//  for(x=0;x<32;x+=8)      /* Now, test to see if anything weird(up+down at same time)
//                     is happening, and correct */
//  {
//   if((JS & (0xC0<<x) ) == (0xC0<<x) ) JS&=~(0xC0<<x);
//   if((JS & (0x30<<x) ) == (0x30<<x) ) JS&=~(0x30<<x);
//  }

	JSreturn = JS;
}

#if PERI
static ButtConfig powerpadsc[2][12] = {
	{
		MK (O), MK (P), MK (BRACKET_LEFT),
		MK (BRACKET_RIGHT), MK (K), MK (L), MK (SEMICOLON),
		MK (APOSTROPHE),
		MK (M), MK (COMMA), MK (PERIOD), MK (SLASH)},
	{
		MK (O), MK (P), MK (BRACKET_LEFT),
		MK (BRACKET_RIGHT), MK (K), MK (L), MK (SEMICOLON),
		MK (APOSTROPHE),
		MK (M), MK (COMMA), MK (PERIOD), MK (SLASH)}
};

static uint32 powerpadbuf[2] = { 0, 0 };

/**
 * Update the status of the power pad input device.
 */
static uint32
UpdatePPadData (int w)
{
	uint32 r = 0;
	ButtConfig *ppadtsc = powerpadsc[w];
	int x;

	// update each of the 12 buttons
	for (x = 0; x < 12; x++)
	{
		if (DTestButton (&ppadtsc[x]))
		{
			r |= 1 << x;
		}
	}

	return r;
}

static uint8 fkbkeys[0x48];
#endif //PERI

/**
 * Update all of the input devices required for the active game.
 */
void FCEUD_UpdateInput ()
{
	int x;
	int t = 0;

	UpdatePhysicalInput ();
	KeyboardCommands ();

	for (x = 0; x < 2; x++)
	{
		switch (CurInputType[x])
		{
			case SI_GAMEPAD:
				t |= 1;
				break;
			case SI_ARKANOID:
				t |= 2;
				break;
			case SI_ZAPPER:
				t |= 2;
				break;
#if PERI
			case SI_POWERPADA:
			case SI_POWERPADB:
				powerpadbuf[x] = UpdatePPadData (x);
				break;
#endif //PERI
		}
	}

	switch (CurInputType[2])
	{
		case SIFC_ARKANOID:
			t |= 2;
			break;
		case SIFC_SHADOW:
			t |= 2;
			break;
#if PERI
		case SIFC_FKB:
			if (g_fkbEnabled)
			{
				UpdateFKB ();
			}
			break;
		case SIFC_HYPERSHOT:
			UpdateHyperShot ();
			break;
		case SIFC_MAHJONG:
			UpdateMahjong ();
			break;
		case SIFC_QUIZKING:
			UpdateQuizKing ();
			break;
		case SIFC_FTRAINERB:
		case SIFC_FTRAINERA:
			UpdateFTrainer ();
			break;
		case SIFC_TOPRIDER:
			UpdateTopRider ();
			break;
#endif //PERI
		case SIFC_OEKAKIDS:
			t |= 2;
			break;
	}

	if (t & 1)
	{
		UpdateGamepad ();
	}

	if (t & 2)
	{
// TODO: tsone: not needed because mouse is updated via callback
//		GetMouseData (MouseData);
	}
}

void FCEUD_SetInput (bool fourscore, bool microphone, ESI port0, ESI port1,
		ESIFC fcexp)
{
	eoptions &= ~EO_FOURSCORE;
	if (fourscore)
	{				// Four Score emulation, only support gamepads, nothing else
		eoptions |= EO_FOURSCORE;
		CurInputType[0] = SI_GAMEPAD;	// Controllers 1 and 3
		CurInputType[1] = SI_GAMEPAD;	// Controllers 2 and 4
		CurInputType[2] = SIFC_NONE;	// No extension
	}
	else
	{	
		// no Four Core emulation, check the config/movie file for controller types
		CurInputType[0] = port0;
		CurInputType[1] = port1;
		CurInputType[2] = fcexp;
	}

	replaceP2StartWithMicrophone = microphone;

	InitInputInterface ();
}

static EM_BOOL FCEM_MouseCallback(int type, const EmscriptenMouseEvent *event, void *)
{
	// Map element coords to NES screen coords.
	MouseData[0] = event->targetX;
	MouseData[1] = event->targetY;
	PtoV(&MouseData[0], &MouseData[1]);
	// Bit 0 set if primary button down, bit 1 set if secondary down.
// TODO: tsone: to get debug (x,y) form mouse, disable from final
#if 1
	if ((event->buttons & 1) && !(MouseData[2] & 1)) {
		printf("DEBUG: mouse pos: %d,%d\n", MouseData[0], MouseData[1]);
	}
#endif
	MouseData[2] = event->buttons & 3;
	return 1;
}

/**
 * Initialize the input device interface between the emulation and the driver.
 */
void InitInputInterface ()
{
	void *InputDPtr;

	int t;
	int x;
	int attrib;

	for (t = 0, x = 0; x < 2; x++)
	{
		attrib = 0;
		InputDPtr = 0;

		switch (CurInputType[x])
		{
#if PERI
			case SI_POWERPADA:
			case SI_POWERPADB:
				InputDPtr = &powerpadbuf[x];
				break;
#endif //PERI
			case SI_GAMEPAD:
				InputDPtr = &JSreturn;
				break;
			case SI_ARKANOID:
				InputDPtr = MouseData;
				t |= 1;
				break;
			case SI_ZAPPER:
				InputDPtr = MouseData;
				t |= 1;
				attrib = 1;
				break;
		}
		FCEUI_SetInput (x, (ESI) CurInputType[x], InputDPtr, attrib);
	}

	attrib = 0;
	InputDPtr = 0;
	switch (CurInputType[2])
	{
		case SIFC_SHADOW:
			InputDPtr = MouseData;
			t |= 1;
			attrib = 1;
			break;
		case SIFC_OEKAKIDS:
			InputDPtr = MouseData;
			t |= 1;
			attrib = 1;
			break;
		case SIFC_ARKANOID:
			InputDPtr = MouseData;
			t |= 1;
			break;
#if PERI
		case SIFC_FKB:
			InputDPtr = fkbkeys;
			break;
		case SIFC_HYPERSHOT:
			InputDPtr = &HyperShotData;
			break;
		case SIFC_MAHJONG:
			InputDPtr = &MahjongData;
			break;
		case SIFC_QUIZKING:
			InputDPtr = &QuizKingData;
			break;
		case SIFC_TOPRIDER:
			InputDPtr = &TopRiderData;
			break;
		case SIFC_BWORLD:
			InputDPtr = BWorldData;
			break;
		case SIFC_FTRAINERA:
		case SIFC_FTRAINERB:
			InputDPtr = &FTrainerData;
			break;
#endif //PERI
	}

	FCEUI_SetInputFC ((ESIFC) CurInputType[2], InputDPtr, attrib);
	FCEUI_SetInputFourscore ((eoptions & EO_FOURSCORE) != 0);

	const char *elem = "#canvas";
	emscripten_set_mousemove_callback(elem, 0, 0, FCEM_MouseCallback);
	emscripten_set_mousedown_callback(elem, 0, 0, FCEM_MouseCallback);
	emscripten_set_mouseup_callback(elem, 0, 0, FCEM_MouseCallback);
	emscripten_set_click_callback(elem, 0, 0, FCEM_MouseCallback);
}


#if PERI
static ButtConfig fkbmap[0x48] = {
	MK (F1), MK (F2), MK (F3), MK (F4), MK (F5), MK (F6), MK (F7), MK (F8),
	MK (1), MK (2), MK (3), MK (4), MK (5), MK (6), MK (7), MK (8), MK (9),
	MK (0),
	MK (MINUS), MK (EQUAL), MK (BACKSLASH), MK (BACKSPACE),
	MK (ESCAPE), MK (Q), MK (W), MK (E), MK (R), MK (T), MK (Y), MK (U), MK (I),
	MK (O),
	MK (P), MK (GRAVE), MK (BRACKET_LEFT), MK (ENTER),
	MK (LEFTCONTROL), MK (A), MK (S), MK (D), MK (F), MK (G), MK (H), MK (J),
	MK (K),
	MK (L), MK (SEMICOLON), MK (APOSTROPHE), MK (BRACKET_RIGHT), MK (INSERT),
	MK (LEFTSHIFT), MK (Z), MK (X), MK (C), MK (V), MK (B), MK (N), MK (M),
	MK (COMMA),
	MK (PERIOD), MK (SLASH), MK (RIGHTALT), MK (RIGHTSHIFT), MK (LEFTALT),
	MK (SPACE),
	MK (DELETE), MK (END), MK (PAGEDOWN),
	MK (CURSORUP), MK (CURSORLEFT), MK (CURSORRIGHT), MK (CURSORDOWN)
};

/**
 * Update the status of the Family KeyBoard.
 */
static void UpdateFKB ()
{
	int x;

	for (x = 0; x < 0x48; x++)
	{
		fkbkeys[x] = 0;

		if (DTestButton (&fkbmap[x]))
		{
			fkbkeys[x] = 1;
		}
	}
}

static ButtConfig HyperShotButtons[4] = {
	MK (Q), MK (W), MK (E), MK (R)
};

/**
 * Update the status of the HyperShot input device.
 */
	static void
UpdateHyperShot ()
{
	int x;

	HyperShotData = 0;
	for (x = 0; x < 0x4; x++)
	{
		if (DTestButton (&HyperShotButtons[x]))
		{
			HyperShotData |= 1 << x;
		}
	}
}

static ButtConfig MahjongButtons[21] = {
	MK (Q), MK (W), MK (E), MK (R), MK (T),
	MK (A), MK (S), MK (D), MK (F), MK (G), MK (H), MK (J), MK (K), MK (L),
	MK (Z), MK (X), MK (C), MK (V), MK (B), MK (N), MK (M)
};

/**
 * Update the status of for the Mahjong input device.
 */
	static void
UpdateMahjong ()
{
	int x;

	MahjongData = 0;
	for (x = 0; x < 21; x++)
	{
		if (DTestButton (&MahjongButtons[x]))
		{
			MahjongData |= 1 << x;
		}
	}
}

static ButtConfig QuizKingButtons[6] = {
	MK (Q), MK (W), MK (E), MK (R), MK (T), MK (Y)
};

/**
 * Update the status of the QuizKing input device.
 */
	static void
UpdateQuizKing ()
{
	int x;

	QuizKingData = 0;

	for (x = 0; x < 6; x++)
	{
		if (DTestButton (&QuizKingButtons[x]))
		{
			QuizKingData |= 1 << x;
		}
	}
}

static ButtConfig TopRiderButtons[8] = {
	MK (Q), MK (W), MK (E), MK (R), MK (T), MK (Y), MK (U), MK (I)
};

/**
 * Update the status of the TopRider input device.
 */
	static void
UpdateTopRider ()
{
	int x;
	TopRiderData = 0;
	for (x = 0; x < 8; x++)
	{
		if (DTestButton (&TopRiderButtons[x]))
		{
			TopRiderData |= (1 << x);
		}
	}
}

static ButtConfig FTrainerButtons[12] = {
	MK (O), MK (P), MK (BRACKET_LEFT),
	MK (BRACKET_RIGHT), MK (K), MK (L), MK (SEMICOLON),
	MK (APOSTROPHE),
	MK (M), MK (COMMA), MK (PERIOD), MK (SLASH)
};

/**
 * Update the status of the FTrainer input device.
 */
	static void
UpdateFTrainer ()
{
	int x;
	FTrainerData = 0;

	for (x = 0; x < 12; x++)
	{
		if (DTestButton (&FTrainerButtons[x]))
		{
			FTrainerData |= (1 << x);
		}
	}
}
#endif //PERI

//
// TODO: tsone: revisit this input config code later?
//
#ifdef EMSCRIPTEN

void InputCfg(const std::string &) {} // dummy

#else // !EMSCRIPTEN

/**
 * Waits for a button input and returns the information as to which
 * button was pressed.  Used in button configuration.
 */
#include "../../utils/memory.h"
static int32 (*LastAx)[64] = 0;
int DWaitButton (const uint8 * text, ButtConfig * bc, int wb)
{
	SDL_Event event;
    if (!LastAx) {
        LastAx = (int32 (*)[64]) FCEU_malloc(sizeof(int32)*64*64);
    }
	int x, y;

	if (text)
	{
		std::string title = "Press a key for ";
		title += (const char *) text;
#if SDL_VERSION_ATLEAST(2,0,0)
		// TODO - SDL2
#else
		SDL_WM_SetCaption (title.c_str (), 0);
#endif
		puts ((const char *) text);
	}

	for (x = 0; x < 64; x++)
	{
		for (y = 0; y < 64; y++)
		{
			LastAx[x][y] = 0x100000;
		}
	}

	int done = 0;
	while (SDL_PollEvent (&event))
	{
		done++;
		switch (event.type)
		{
			case SDL_KEYDOWN:
				bc->ButtType[wb] = BUTTC_KEYBOARD;
				bc->DeviceNum[wb] = 0;
				bc->ButtonNum[wb] = event.key.keysym.sym;
				return (1);
			case SDL_JOYBUTTONDOWN:
				bc->ButtType[wb] = BUTTC_JOYSTICK;
				bc->DeviceNum[wb] = event.jbutton.which;
				bc->ButtonNum[wb] = event.jbutton.button;
				return (1);
			case SDL_JOYHATMOTION:
				if (event.jhat.value == SDL_HAT_CENTERED)
					done--;
				else
				{
					bc->ButtType[wb] = BUTTC_JOYSTICK;
					bc->DeviceNum[wb] = event.jhat.which;
					bc->ButtonNum[wb] =
						(0x2000 | ((event.jhat.hat & 0x1F) << 8) | event.
						 jhat.value);
					return (1);
				}
				break;
			case SDL_JOYAXISMOTION:
				if (LastAx[event.jaxis.which][event.jaxis.axis] == 0x100000)
				{
					if (abs (event.jaxis.value) < 1000)
					{
						LastAx[event.jaxis.which][event.jaxis.axis] =
							event.jaxis.value;
					}
					done--;
				}
				else
				{
					if (abs
							(LastAx[event.jaxis.which][event.jaxis.axis] -
							 event.jaxis.value) >= 8192)
					{
						bc->ButtType[wb] = BUTTC_JOYSTICK;
						bc->DeviceNum[wb] = event.jaxis.which;
						bc->ButtonNum[wb] = (0x8000 | event.jaxis.axis |
								((event.jaxis.value < 0)
								 ? 0x4000 : 0));
						return (1);
					}
					else
						done--;
				}
				break;
			default:
				done--;
		}
	}

	return (0);
}

/**
 * This function takes in button inputs until either it sees two of
 * the same button presses in a row or gets four inputs and then saves
 * the total number of button presses.  Each of the keys pressed is
 * used as input for the specified button, thus allowing up to four
 * possible settings for each input button.
 */
	void
ConfigButton (char *text, ButtConfig * bc)
{
	uint8 buf[256];
	int wc;

	for (wc = 0; wc < MAXBUTTCONFIG; wc++)
	{
		sprintf ((char *) buf, "%s (%d)", text, wc + 1);
		DWaitButton (buf, bc, wc);

		if (wc &&
				bc->ButtType[wc] == bc->ButtType[wc - 1] &&
				bc->DeviceNum[wc] == bc->DeviceNum[wc - 1] &&
				bc->ButtonNum[wc] == bc->ButtonNum[wc - 1])
		{
			break;
		}
	}
	bc->NumC = wc;
}

/**
 * Update the button configuration for a specified device.
 */
extern Config *g_config;

void ConfigDevice (int which, int arg)
{
	char buf[256];
	int x;
	std::string prefix;
	char *str[10] =
	{ "A", "B", "SELECT", "START", "UP", "DOWN", "LEFT", "RIGHT", "Rapid A",
		"Rapid B"
	};

	// XXX soules - set the configuration options so that later calls
	//              don't override these.  This is a temp hack until I
	//              can clean up this file.

	switch (which)
	{
#if PERI
		case FCFGD_QUIZKING:
			prefix = "SDL.Input.QuizKing.";
			for (x = 0; x < 6; x++)
			{
				sprintf (buf, "Quiz King Buzzer #%d", x + 1);
				ConfigButton (buf, &QuizKingButtons[x]);

				g_config->setOption (prefix + QuizKingNames[x],
						QuizKingButtons[x].ButtonNum[0]);
			}

			if (QuizKingButtons[0].ButtType[0] == BUTTC_KEYBOARD)
			{
				g_config->setOption (prefix + "DeviceType", "Keyboard");
			}
			else if (QuizKingButtons[0].ButtType[0] == BUTTC_JOYSTICK)
			{
				g_config->setOption (prefix + "DeviceType", "Joystick");
			}
			else
			{
				g_config->setOption (prefix + "DeviceType", "Unknown");
			}
			g_config->setOption (prefix + "DeviceNum",
					QuizKingButtons[0].DeviceNum[0]);
			break;
		case FCFGD_HYPERSHOT:
			prefix = "SDL.Input.HyperShot.";
			for (x = 0; x < 4; x++)
			{
				sprintf (buf, "Hyper Shot %d: %s",
						((x & 2) >> 1) + 1, (x & 1) ? "JUMP" : "RUN");
				ConfigButton (buf, &HyperShotButtons[x]);

				g_config->setOption (prefix + HyperShotNames[x],
						HyperShotButtons[x].ButtonNum[0]);
			}

			if (HyperShotButtons[0].ButtType[0] == BUTTC_KEYBOARD)
			{
				g_config->setOption (prefix + "DeviceType", "Keyboard");
			}
			else if (HyperShotButtons[0].ButtType[0] == BUTTC_JOYSTICK)
			{
				g_config->setOption (prefix + "DeviceType", "Joystick");
			}
			else
			{
				g_config->setOption (prefix + "DeviceType", "Unknown");
			}
			g_config->setOption (prefix + "DeviceNum",
					HyperShotButtons[0].DeviceNum[0]);
			break;
		case FCFGD_POWERPAD:
			snprintf (buf, 256, "SDL.Input.PowerPad.%d", (arg & 1));
			prefix = buf;
			for (x = 0; x < 12; x++)
			{
				sprintf (buf, "PowerPad %d: %d", (arg & 1) + 1, x + 11);
				ConfigButton (buf, &powerpadsc[arg & 1][x]);

				g_config->setOption (prefix + PowerPadNames[x],
						powerpadsc[arg & 1][x].ButtonNum[0]);
			}

			if (powerpadsc[arg & 1][0].ButtType[0] == BUTTC_KEYBOARD)
			{
				g_config->setOption (prefix + "DeviceType", "Keyboard");
			}
			else if (powerpadsc[arg & 1][0].ButtType[0] == BUTTC_JOYSTICK)
			{
				g_config->setOption (prefix + "DeviceType", "Joystick");
			}
			else
			{
				g_config->setOption (prefix + "DeviceType", "Unknown");
			}
			g_config->setOption (prefix + "DeviceNum",
					powerpadsc[arg & 1][0].DeviceNum[0]);
			break;
#endif //PERI

		case FCFGD_GAMEPAD:
			snprintf (buf, 256, "SDL.Input.GamePad.%d", arg);
			prefix = buf;
			for (x = 0; x < 10; x++)
			{
				sprintf (buf, "GamePad #%d: %s", arg + 1, str[x]);
				ConfigButton (buf, &GamePadConfig[arg][x]);

				g_config->setOption (prefix + GamePadNames[x],
						GamePadConfig[arg][x].ButtonNum[0]);
			}

			if (GamePadConfig[arg][0].ButtType[0] == BUTTC_KEYBOARD)
			{
				g_config->setOption (prefix + "DeviceType", "Keyboard");
			}
			else if (GamePadConfig[arg][0].ButtType[0] == BUTTC_JOYSTICK)
			{
				g_config->setOption (prefix + "DeviceType", "Joystick");
			}
			else
			{
				g_config->setOption (prefix + "DeviceType", "Unknown");
			}
			g_config->setOption (prefix + "DeviceNum",
					GamePadConfig[arg][0].DeviceNum[0]);
			break;
	}
}

/**
 * Update the button configuration for a device, specified by a text string.
 */
void InputCfg (const std::string & text)
{
	if (text.find ("gamepad") != std::string::npos)
	{
		int device = (text[strlen ("gamepad")] - '1');
		if (device < 0 || device > 3)
		{
			FCEUD_PrintError
				("Invalid gamepad device specified; must be one of gamepad1 through gamepad4");
			exit (-1);
		}
		ConfigDevice (FCFGD_GAMEPAD, device);
	}
#if PERI
	else if (text.find ("powerpad") != std::string::npos)
	{
		int device = (text[strlen ("powerpad")] - '1');
		if (device < 0 || device > 1)
		{
			FCEUD_PrintError
				("Invalid powerpad device specified; must be powerpad1 or powerpad2");
			exit (-1);
		}
		ConfigDevice (FCFGD_POWERPAD, device);
	}
	else if (text.find ("hypershot") != std::string::npos)
	{
		ConfigDevice (FCFGD_HYPERSHOT, 0);
	}
	else if (text.find ("quizking") != std::string::npos)
	{
		ConfigDevice (FCFGD_QUIZKING, 0);
	}
#endif //PERI
}
#endif // EMSCRIPTEN

/**
 * Hack to map the new configuration onto the existing button
 * configuration management.  Will probably want to change this in the
 * future - soules.
 */
	void
UpdateInput (Config * config)
{
	char buf[64];
	std::string device, prefix;

	for (unsigned int i = 0; i < 3; i++)
	{
		snprintf (buf, 64, "SDL.Input.%d", i);
		config->getOption (buf, &device);

		if (device == "None")
		{
			UsrInputType[i] = SI_NONE;
		}
		else if (device.find ("GamePad") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_GAMEPAD : (int) SIFC_NONE;
		}
		else if (device.find ("PowerPad.0") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_POWERPADA : (int) SIFC_NONE;
		}
		else if (device.find ("PowerPad.1") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_POWERPADB : (int) SIFC_NONE;
		}
		else if (device.find ("QuizKing") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_QUIZKING;
		}
		else if (device.find ("HyperShot") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_HYPERSHOT;
		}
		else if (device.find ("Mahjong") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_MAHJONG;
		}
		else if (device.find ("TopRider") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_TOPRIDER;
		}
		else if (device.find ("FTrainer") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_FTRAINERA;
		}
		else if (device.find ("FamilyKeyBoard") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_FKB;
		}
		else if (device.find ("OekaKids") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_OEKAKIDS;
		}
		else if (device.find ("Arkanoid") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_ARKANOID : (int) SIFC_ARKANOID;
		}
		else if (device.find ("Shadow") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_SHADOW;
		}
		else if (device.find ("Zapper") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_ZAPPER : (int) SIFC_NONE;
		}
		else if (device.find ("BWorld") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_BWORLD;
		}
		else if (device.find ("4Player") != std::string::npos)
		{
			UsrInputType[i] = (i < 2) ? (int) SI_NONE : (int) SIFC_4PLAYER;
		}
		else
		{
			// Unknown device
			UsrInputType[i] = SI_NONE;
		}
	}

	// update each of the devices' configuration structure
	// XXX soules - this is temporary until this file is cleaned up to
	//              simplify the interface between configuration and
	//              structure data.  This will likely include the
	//              removal of multiple input buttons for a single
	//              input device key.
	int type, devnum, button;

	// gamepad 0 - 3
	for (unsigned int i = 0; i < GAMEPAD_NUM_DEVICES; i++)
	{
		char buf[64];
		snprintf (buf, 20, "SDL.Input.GamePad.%d.", i);
		prefix = buf;

		config->getOption (prefix + "DeviceType", &device);
		if (device.find ("Keyboard") != std::string::npos)
		{
			type = BUTTC_KEYBOARD;
		}
		else if (device.find ("Joystick") != std::string::npos)
		{
			type = BUTTC_JOYSTICK;
		}
		else
		{
			type = 0;
		}

		config->getOption (prefix + "DeviceNum", &devnum);
		for (unsigned int j = 0; j < GAMEPAD_NUM_BUTTONS; j++)
		{
			config->getOption (prefix + GamePadNames[j], &button);

			GamePadConfig[i][j].ButtType[0] = type;
			GamePadConfig[i][j].DeviceNum[0] = devnum;
			GamePadConfig[i][j].ButtonNum[0] = button;
			GamePadConfig[i][j].NumC = 1;
		}
	}

#if PERI
	// PowerPad 0 - 1
	for (unsigned int i = 0; i < POWERPAD_NUM_DEVICES; i++)
	{
		char buf[64];
		snprintf (buf, 20, "SDL.Input.PowerPad.%d.", i);
		prefix = buf;

		config->getOption (prefix + "DeviceType", &device);
		if (device.find ("Keyboard") != std::string::npos)
		{
			type = BUTTC_KEYBOARD;
		}
		else if (device.find ("Joystick") != std::string::npos)
		{
			type = BUTTC_JOYSTICK;
		}
		else
		{
			type = 0;
		}

		config->getOption (prefix + "DeviceNum", &devnum);
		for (unsigned int j = 0; j < POWERPAD_NUM_BUTTONS; j++)
		{
			config->getOption (prefix + PowerPadNames[j], &button);

			powerpadsc[i][j].ButtType[0] = type;
			powerpadsc[i][j].DeviceNum[0] = devnum;
			powerpadsc[i][j].ButtonNum[0] = button;
			powerpadsc[i][j].NumC = 1;
		}
	}

	// QuizKing
	prefix = "SDL.Input.QuizKing.";
	config->getOption (prefix + "DeviceType", &device);
	if (device.find ("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find ("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption (prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < QUIZKING_NUM_BUTTONS; j++)
	{
		config->getOption (prefix + QuizKingNames[j], &button);

		QuizKingButtons[j].ButtType[0] = type;
		QuizKingButtons[j].DeviceNum[0] = devnum;
		QuizKingButtons[j].ButtonNum[0] = button;
		QuizKingButtons[j].NumC = 1;
	}

	// HyperShot
	prefix = "SDL.Input.HyperShot.";
	config->getOption (prefix + "DeviceType", &device);
	if (device.find ("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find ("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption (prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < HYPERSHOT_NUM_BUTTONS; j++)
	{
		config->getOption (prefix + HyperShotNames[j], &button);

		HyperShotButtons[j].ButtType[0] = type;
		HyperShotButtons[j].DeviceNum[0] = devnum;
		HyperShotButtons[j].ButtonNum[0] = button;
		HyperShotButtons[j].NumC = 1;
	}

	// Mahjong
	prefix = "SDL.Input.Mahjong.";
	config->getOption (prefix + "DeviceType", &device);
	if (device.find ("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find ("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption (prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < MAHJONG_NUM_BUTTONS; j++)
	{
		config->getOption (prefix + MahjongNames[j], &button);

		MahjongButtons[j].ButtType[0] = type;
		MahjongButtons[j].DeviceNum[0] = devnum;
		MahjongButtons[j].ButtonNum[0] = button;
		MahjongButtons[j].NumC = 1;
	}

	// TopRider
	prefix = "SDL.Input.TopRider.";
	config->getOption (prefix + "DeviceType", &device);
	if (device.find ("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find ("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption (prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < TOPRIDER_NUM_BUTTONS; j++)
	{
		config->getOption (prefix + TopRiderNames[j], &button);

		TopRiderButtons[j].ButtType[0] = type;
		TopRiderButtons[j].DeviceNum[0] = devnum;
		TopRiderButtons[j].ButtonNum[0] = button;
		TopRiderButtons[j].NumC = 1;
	}

	// FTrainer
	prefix = "SDL.Input.FTrainer.";
	config->getOption (prefix + "DeviceType", &device);
	if (device.find ("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find ("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption (prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < FTRAINER_NUM_BUTTONS; j++)
	{
		config->getOption (prefix + FTrainerNames[j], &button);

		FTrainerButtons[j].ButtType[0] = type;
		FTrainerButtons[j].DeviceNum[0] = devnum;
		FTrainerButtons[j].ButtonNum[0] = button;
		FTrainerButtons[j].NumC = 1;
	}

	// FamilyKeyBoard
	prefix = "SDL.Input.FamilyKeyBoard.";
	config->getOption (prefix + "DeviceType", &device);
	if (device.find ("Keyboard") != std::string::npos)
	{
		type = BUTTC_KEYBOARD;
	}
	else if (device.find ("Joystick") != std::string::npos)
	{
		type = BUTTC_JOYSTICK;
	}
	else
	{
		type = 0;
	}
	config->getOption (prefix + "DeviceNum", &devnum);
	for (unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++)
	{
		config->getOption (prefix + FamilyKeyBoardNames[j], &button);

		fkbmap[j].ButtType[0] = type;
		fkbmap[j].DeviceNum[0] = devnum;
		fkbmap[j].ButtonNum[0] = button;
		fkbmap[j].NumC = 1;
	}
#endif
}

// Definitions from main.h:
// GamePad defaults
const char *GamePadNames[GAMEPAD_NUM_BUTTONS] = { "A", "B", "Select", "Start",
	"Up", "Down", "Left", "Right", "TurboA", "TurboB"
};
const char *DefaultGamePadDevice[GAMEPAD_NUM_DEVICES] =
{ "Keyboard", "None", "None", "None" };
const int DefaultGamePad[GAMEPAD_NUM_DEVICES][GAMEPAD_NUM_BUTTONS] =
{ {SDLK_F, SDLK_D, SDLK_S, SDLK_RETURN,
	SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

#if PERI
// PowerPad defaults
const char *PowerPadNames[POWERPAD_NUM_BUTTONS] =
{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B" };
const char *DefaultPowerPadDevice[POWERPAD_NUM_DEVICES] =
{ "Keyboard", "None" };
const int DefaultPowerPad[POWERPAD_NUM_DEVICES][POWERPAD_NUM_BUTTONS] =
{ {SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
	SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
	SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH},
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

// QuizKing defaults
const char *QuizKingNames[QUIZKING_NUM_BUTTONS] =
{ "0", "1", "2", "3", "4", "5" };
const char *DefaultQuizKingDevice = "Keyboard";
const int DefaultQuizKing[QUIZKING_NUM_BUTTONS] =
{ SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y };

// HyperShot defaults
const char *HyperShotNames[HYPERSHOT_NUM_BUTTONS] = { "0", "1", "2", "3" };

const char *DefaultHyperShotDevice = "Keyboard";
const int DefaultHyperShot[HYPERSHOT_NUM_BUTTONS] =
{ SDLK_q, SDLK_w, SDLK_e, SDLK_r };

// Mahjong defaults
const char *MahjongNames[MAHJONG_NUM_BUTTONS] =
{ "00", "01", "02", "03", "04", "05", "06", "07",
	"08", "09", "10", "11", "12", "13", "14", "15",
	"16", "17", "18", "19", "20"
};

const char *DefaultMahjongDevice = "Keyboard";
const int DefaultMahjong[MAHJONG_NUM_BUTTONS] =
{ SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_a, SDLK_s, SDLK_d,
	SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_z, SDLK_x,
	SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m
};

// TopRider defaults
const char *TopRiderNames[TOPRIDER_NUM_BUTTONS] =
{ "0", "1", "2", "3", "4", "5", "6", "7" };
const char *DefaultTopRiderDevice = "Keyboard";
const int DefaultTopRider[TOPRIDER_NUM_BUTTONS] =
{ SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i };

// FTrainer defaults
const char *FTrainerNames[FTRAINER_NUM_BUTTONS] =
{ "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B" };
const char *DefaultFTrainerDevice = "Keyboard";
const int DefaultFTrainer[FTRAINER_NUM_BUTTONS] =
{ SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
	SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
	SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH
};

// FamilyKeyBoard defaults
const char *FamilyKeyBoardNames[FAMILYKEYBOARD_NUM_BUTTONS] =
{ "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
	"1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"MINUS", "EQUAL", "BACKSLASH", "BACKSPACE",
	"ESCAPE", "Q", "W", "E", "R", "T", "Y", "U", "I", "O",
	"P", "GRAVE", "BRACKET_LEFT", "ENTER",
	"LEFTCONTROL", "A", "S", "D", "F", "G", "H", "J", "K",
	"L", "SEMICOLON", "APOSTROPHE", "BRACKET_RIGHT", "INSERT",
	"LEFTSHIFT", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
	"PERIOD", "SLASH", "RIGHTALT", "RIGHTSHIFT", "LEFTALT", "SPACE",
	"DELETE", "END", "PAGEDOWN",
	"CURSORUP", "CURSORLEFT", "CURSORRIGHT", "CURSORDOWN"
};

const char *DefaultFamilyKeyBoardDevice = "Keyboard";
const int DefaultFamilyKeyBoard[FAMILYKEYBOARD_NUM_BUTTONS] =
{ SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8,
	SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
	SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0,
	SDLK_MINUS, SDLK_EQUALS, SDLK_BACKSLASH, SDLK_BACKSPACE,
	SDLK_ESCAPE, SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u,
	SDLK_i, SDLK_o, SDLK_p, SDLK_BACKQUOTE, SDLK_LEFTBRACKET, SDLK_RETURN,
	SDLK_LCTRL, SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j,
	SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_RIGHTBRACKET,
	SDLK_INSERT, SDLK_LSHIFT, SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b,
	SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RALT,
	SDLK_RSHIFT, SDLK_LALT, SDLK_SPACE, SDLK_DELETE, SDLK_END, SDLK_PAGEDOWN,
	SDLK_UP, SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN
};

#endif //PERI
