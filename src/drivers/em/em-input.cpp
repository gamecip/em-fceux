/* FCE Ultra - NES/Famicom Emulator - Emscripten inputs
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
#include <html5.h>


#define GAMEPAD_THRESHOLD 0.1


int NoWaiting = 1;

extern Config *g_config;
extern bool frameAdvanceLagSkip, lagCounterDisplay;
extern int gametype;

uint32 MouseData[3] = { 0, 0, 0 };


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
static int s_rapidFireFrame = 0;

// Mapping from an html5 key code to an input.
static unsigned int* s_key_map = 0;
// Input buttons state. Set to 1 if mapped keyboard key is down, otherwise 0.
static bool* s_input_state = 0;

// Frame counter for zeroing mouse buttons.
static int s_mouseReleaseFC = 0;

#if PERI
static int g_fkbEnabled = 0;
#endif //PERI


static bool IsInput(unsigned int input)
{
	return s_input_state[input];
}

static bool IsInputOnce(unsigned int input)
{
	bool result = s_input_state[input];
	s_input_state[input] = false;
	return result;
}

static void UpdateSystem()
{
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
		if (g_fkbEnabled)
		{
			return;
		}
	}
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

	if (gametype != GIT_NSF) {
		if (IsInputOnce(FCEM_SYSTEM_STATE_SAVE)) {
			FCEUI_SaveState(NULL);
		}
		if (IsInputOnce(FCEM_SYSTEM_STATE_LOAD)) {
			FCEUI_LoadState(NULL);
		}
	}

// TODO: tsone: not implemented
#ifndef EMSCRIPTEN
	if (_keyonly (SDLK_EQUALS)) {
		DecreaseEmulationSpeed ();
	}

	if (_keyonly (SDLK_MINUS))
	{
		IncreaseEmulationSpeed ();
	}
#endif

// TODO: tsone: allow input displaying?
#ifndef EMSCRIPTEN
	if (_keyonly (SDLK_COMMA))
	{
		FCEUI_ToggleInputDisplay ();
	}
#endif

	if (IsInputOnce(FCEM_SYSTEM_PAUSE)) {
		FCEUI_ToggleEmulationPause();
	}

// TODO: tsone: is this the right way to clear here?
	NoWaiting &= ~1;
	if (IsInput(FCEM_SYSTEM_THROTTLE)) {
		NoWaiting |= 1;
	}

	static bool frameAdvancing = false;
	if (IsInput(FCEM_SYSTEM_FRAME_ADVANCE)) {
		if (frameAdvancing == false) {
			FCEUI_FrameAdvance ();
			frameAdvancing = true;
		}
	} else {
		if (frameAdvancing) {
			FCEUI_FrameAdvanceEnd ();
			frameAdvancing = false;
		}
	}

	if (IsInputOnce(FCEM_SYSTEM_RESET)) {
		FCEUI_ResetNES();
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

	for (int i = 0; i < 10; ++i) {
		if (IsInputOnce(FCEM_SYSTEM_STATE_SELECT_0 + i)) {
			FCEUI_SelectState (i, 1);
		}
	}

// TODO: tsone: not implemented
#ifndef EMSCRIPTEN
	if (_keyonly (SDLK_PAGEUP)) {
		FCEUI_SelectStateNext (1);
	}

	if (_keyonly (SDLK_PAGEDOWN)) {
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

		DIPSless:
		for(int i=0; i<10;i++)
		{
			keyonly(i);
		}
	}
#endif
}

// TODO: tsone: test buttons for connected gamepad devices (in browser)
// idx 0..9 matches: a, b, select, start, up, down, left, right, turbo a, turbo b
static int FCEM_TestGamepadButton(const EmscriptenGamepadEvent *p, int idx)
{
    if (!p->connected) {
        return 0;
    }
    if (idx >= 8) {
        // Turbo A, Turbo B.
        idx = idx - 4;
        return ((idx < p->numButtons) && (p->digitalButton[idx] || (p->analogButton[idx] >= GAMEPAD_THRESHOLD)));
    } else if (idx >= 4) {
        // Up, down, left, right.
        const int positive_axis = idx & 1;
        idx = !((idx & 0x02) >> 1);
        if (idx < p->numAxes) {
            if (positive_axis) { 
                return p->axis[idx] >= GAMEPAD_THRESHOLD;
            } else { 
                return p->axis[idx] <= -GAMEPAD_THRESHOLD;
            }
        } else {
            return 0;
        }
    } else { 
        // A, B, Select, Start.
        return ((idx < p->numButtons) && (p->digitalButton[idx] || (p->analogButton[idx] >= GAMEPAD_THRESHOLD)));
    }
}

static void UpdateGamepad(void)
{
	uint32 JS = 0;
	int x;
	int i;

	s_rapidFireFrame ^= 1;

	int opposite_dirs;
	g_config->getOption("SDL.Input.EnableOppositeDirectionals", &opposite_dirs);

	// Four possibly connected joysticks/gamepads are read here, each matching a NES gamepad.
	EmscriptenGamepadEvent gamepads[4];
	for (i = 0; i < 4; ++i) {
		if (EMSCRIPTEN_RESULT_SUCCESS != emscripten_get_gamepad_status(i, &gamepads[i])) {
			// Set as disconnected if query failed.
			gamepads[i].connected = 0;
		}
	}

	for (i = 0; i < 4; ++i) {
		bool left = false;
		bool up = false;
                int inputBase = FCEM_GAMEPAD + FCEM_GAMEPAD_SIZE*i;

		for (x = 0; x < 8; ++x) {
			if (FCEM_TestGamepadButton(&gamepads[i], x) || IsInput(inputBase + x)) {
				if (opposite_dirs == 0) {
					// test for left+right and up+down
					if(x == 4) {
						up = true;
					}
					if((x == 5) && (up == true)) {
						continue;
					}
					if(x == 6) {
						left = true;
					}
					if((x == 7) && (left == true)) {
						continue;
					}
				}
				JS |= (1 << x) << (i << 3);
			}
		}

// TODO: tsone: keypad exit is not supported
#ifndef EMSCRIPTEN
		int four_button_exit;
		g_config->getOption("SDL.ABStartSelectExit", &four_button_exit);
		// if a+b+start+select is pressed, exit
		if (four_button_exit && JS == 15) {
			FCEUI_printf("all buttons pressed, exiting\n");
			CloseGame();
			FCEUI_Kill();
			exit(0);
		}
#endif

		// rapid-fire a, rapid-fire b
		if (s_rapidFireFrame) {
			for (x = 0; x < 2; ++x) {
   				if (FCEM_TestGamepadButton(&gamepads[i], 8+x) || IsInput(inputBase+8+x)) {
					JS |= (1 << x) << (i << 3);
				}
			}
		}
	}

	JSreturn = JS;
}

static void UpdateMouse()
{
	if (!s_mouseReleaseFC) {
		MouseData[2] = 0;
	} else {
		--s_mouseReleaseFC;
	}
}

static EM_BOOL FCEM_MouseDownCallback(int type, const EmscriptenMouseEvent *event, void *)
{
	MouseData[0] = event->targetX;
	MouseData[1] = event->targetY;
	MouseData[2] = event->buttons & 3;

	CanvasToNESCoords(&MouseData[0], &MouseData[1]);

	// Force buttons down for 3 frames. This is because mouse events are async.
	// We need mouse buttons remain down at least one frame -- however it seems
	// fceux works better if duration is 3 frames.
	s_mouseReleaseFC = 3;

	return 1;
}

static int getKeyMapIdx(const EmscriptenKeyboardEvent *ev)
{
	if (ev->keyCode >= 256) {
		return -1;
	}
	return ev->keyCode | (ev->ctrlKey << 8) | (ev->shiftKey << 9) | (ev->altKey << 10) | (ev->metaKey << 11);
}

static unsigned int getInput(const EmscriptenKeyboardEvent *ev)
{
	int idx = getKeyMapIdx(ev);
	if (idx == -1) {
		return FCEM_NULL;
	}
	return s_key_map[idx];
}

static EM_BOOL FCEM_KeyCallback(int eventType, const EmscriptenKeyboardEvent *event, void*)
{
	unsigned int input = getInput(event);
	s_input_state[input] = (eventType == EMSCRIPTEN_EVENT_KEYDOWN);
	s_input_state[FCEM_NULL] = false; // Disable NULL input.
	return 1;
}

void BindKey(int id, int keyIdx)
{
	if (keyIdx >= 0 && keyIdx < FCEM_KEY_MAP_SIZE && id >= FCEM_NULL && id < FCEM_INPUT_COUNT) {
		s_key_map[keyIdx] = id;
	}
}

void BindPort(int portIdx, ESI peri)
{
	uint32 *ptr = 0;

	switch (peri) {
	case SI_GAMEPAD: ptr = &JSreturn; break;
	case SI_ZAPPER:  ptr = MouseData; break;
	default: return;
	}

	FCEUI_SetInput(portIdx, peri, ptr, 0);
}

void FCEUD_UpdateInput ()
{
	UpdateSystem();
	UpdateGamepad();
	UpdateMouse();
// TODO: tsone: add other peripherals?
}

void FCEUD_SetInput(bool fourscore, bool microphone, ESI, ESI, ESIFC fcexp)
{
// TODO: tsone: with fourscore, only gamepads will work and must be forced on:
/*
	if (fourscore) {
		FCEUI_SetInput(0,SI_GAMEPAD,&JSreturn,0);
		FCEUI_SetInput(1,SI_GAMEPAD,&JSreturn,0);
	}
*/
	eoptions &= ~EO_FOURSCORE;
	if (fourscore) {
		// Four Score emulation, only support gamepads, nothing else
		eoptions |= EO_FOURSCORE;
	}

	replaceP2StartWithMicrophone = microphone;

	// Init input state array. Fills it with zeroes.
	FCEU_ARRAY_EM(s_input_state, bool, FCEM_INPUT_COUNT);

	// Init key map. Fills it with zeroes.
	FCEU_ARRAY_EM(s_key_map, unsigned int, FCEM_KEY_MAP_SIZE);

// TODO: tsone: make configurable by front-end
	FCEUI_SetInput(0, SI_GAMEPAD, &JSreturn, 0);
	FCEUI_SetInput(1, SI_ZAPPER, MouseData, 0);

// TODO: tsone: support FC expansion port?
	FCEUI_SetInputFC(fcexp, 0, 0);

// TODO: tsone: support fourscore?
//	FCEUI_SetInputFourscore((eoptions & EO_FOURSCORE) != 0);

	emscripten_set_mousedown_callback("#canvas", 0, 0, FCEM_MouseDownCallback);

	const char *elem = "#window";
	emscripten_set_keydown_callback(elem, 0, 0, FCEM_KeyCallback);
	emscripten_set_keyup_callback(elem, 0, 0, FCEM_KeyCallback);
}

void ParseGIInput (FCEUGI * gi)
{
	gametype = gi->type;
}

// NOTE: tsone: required for boards/transformer.cpp, must return array of 256 ints...
const unsigned int *GetKeyboard()
{
	return s_key_map;
}

// TODO: tsone: dummy functions
void FCEUD_MovieRecordTo() {}
void FCEUD_SaveStateAs() {}
void FCEUD_LoadStateFrom() {}

