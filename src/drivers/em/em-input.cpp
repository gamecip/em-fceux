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
#include <html5.h>


int NoWaiting = 1;

extern Config *g_config;
extern bool frameAdvanceLagSkip, lagCounterDisplay;
extern int gametype;

int MouseData[3] = { 0, 0, 0 };


void ParseGIInput (FCEUGI * gi)
{
	gametype = gi->type;
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

// Mapping from an html5 key code to an input.
static unsigned int* s_key_map = 0;
// Input buttons state. Set to 1 if mapped keyboard key is down, otherwise 0.
static bool* s_input_state = 0;

#if PERI
static int g_fkbEnabled = 0;
#endif //PERI

// TODO: tsone: dummy functions
void FCEUD_MovieRecordTo() {}
void FCEUD_SaveStateAs() {}
void FCEUD_LoadStateFrom() {}

// NOTE: tsone: required for boards/transformer.cpp, must return array of 256 ints...
unsigned int *GetKeyboard()
{
	return s_key_map;
}

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
		SDL_WM_GrabInput (g_fkbEnabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
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

static void UpdateGamepad(void)
{
	static int rapid = 0;
	uint32 JS = 0;
	int x;
	int i;

	rapid ^= 1;

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
		for (x = 0; x < 8; ++x) {
			if (FCEM_TestGamepadButton(&gamepads[i], x) || IsInput(FCEM_GAMEPAD + x)) {
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
		if (rapid) {
			for (x = 0; x < 2; ++x) {
   				if (FCEM_TestGamepadButton(&gamepads[i], 8+x) || IsInput(FCEM_GAMEPAD+8+x)) {
					JS |= (1 << x) << (i << 3);
				}
			}
		}
	}

	JSreturn = JS;
}

void FCEUD_UpdateInput ()
{
	UpdateSystem();
// TODO: tsone: add zapper etc.?
	UpdateGamepad();
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

static int getKeyMapIdx(const EmscriptenKeyboardEvent *event)
{
	if (event->keyCode >= 256) {
		return -1;
	}
	return (event->ctrlKey << 8) | (event->shiftKey << 9)
		| (event->altKey << 10) | (event->metaKey << 11)
		| event->keyCode;
}

static unsigned int getInput(const EmscriptenKeyboardEvent *event)
{
	int idx = getKeyMapIdx(event);
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

void FCEUD_SetInput(bool fourscore, bool microphone, ESI port0, ESI port1, ESIFC fcexp)
{
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

	// Default mapping for 1st gamepad inputs.
// TODO: tsone: mappings for 2nd gamepad?
	// The key codes are from:
	// https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent/keyCode
	s_key_map[0x046] = FCEM_GAMEPAD0_A;		// F key
	s_key_map[0x044] = FCEM_GAMEPAD0_B;		// D key
	s_key_map[0x053] = FCEM_GAMEPAD0_SELECT;	// S key
	s_key_map[0x00D] = FCEM_GAMEPAD0_START;		// Enter key
	s_key_map[0x026] = FCEM_GAMEPAD0_UP;		// Up arrow
	s_key_map[0x028] = FCEM_GAMEPAD0_DOWN;		// Down arrow
	s_key_map[0x025] = FCEM_GAMEPAD0_LEFT;		// Left arrow
	s_key_map[0x027] = FCEM_GAMEPAD0_RIGHT;		// Right arrow

	// Default mappings for system inputs.
	s_key_map[0x152] = FCEM_SYSTEM_RESET;		// Ctrl+R key
	s_key_map[0x009] = FCEM_SYSTEM_THROTTLE;	// Tab key
	s_key_map[0x050] = FCEM_SYSTEM_PAUSE;		// P key
	s_key_map[0x0DC] = FCEM_SYSTEM_FRAME_ADVANCE;	// Backslash key
	s_key_map[0x074] = FCEM_SYSTEM_STATE_SAVE;	// F5 key
	s_key_map[0x076] = FCEM_SYSTEM_STATE_LOAD;	// F7 key
	for (int i = 0; i < 10; ++i) {
		s_key_map[0x030 + i] = FCEM_SYSTEM_STATE_SELECT_0 + i;	// 0..9 keys
	}

// TODO: tsone: more inputs, now just gamepad at port 0
// TODO: tsone: zapper would get attrib 1
	FCEUI_SetInput(0, port0, &JSreturn, 0);
	FCEUI_SetInput(1, port1, 0, 0);

// TODO: tsone: support FC expansion port?
	FCEUI_SetInputFC(fcexp, 0, 0);

// TODO: tsone: support fourscore? really?
//	FCEUI_SetInputFourscore((eoptions & EO_FOURSCORE) != 0);

	const char *elem = "#canvas";
	emscripten_set_mousemove_callback(elem, 0, 0, FCEM_MouseCallback);
	emscripten_set_mousedown_callback(elem, 0, 0, FCEM_MouseCallback);
	emscripten_set_mouseup_callback(elem, 0, 0, FCEM_MouseCallback);
	emscripten_set_click_callback(elem, 0, 0, FCEM_MouseCallback);

	elem = "#window";
	emscripten_set_keydown_callback(elem, 0, 0, FCEM_KeyCallback);
	emscripten_set_keyup_callback(elem, 0, 0, FCEM_KeyCallback);
}

