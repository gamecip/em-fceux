/* FCE Ultra - NES/Famicom Emulator - Emscripten main
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
#include "em.h"
#include "../../fceu.h"
#include "../../version.h"
#include <emscripten.h>


// Number of frames to skip per regular frame when frameskipping.
#define TURBO_FRAMESKIPS 3
// Set to 1 to set mainloop call at higher rate than requestAnimationFrame.
// Recommended behavior is to use requestAnimationFrame, i.e. set to 0.
#define SUPER_RATE 0

extern double g_fpsScale;
extern bool MaxSpeed;

int eoptions = 0;
Config *g_config;

static int inited = 0;
static int isloaded = 0;

/**
 * Loads a game, given a full path/filename.  The driver code must be
 * initialized after the game is loaded, because the emulator code
 * provides data necessary for the driver code(number of scanlines to
 * render, what virtual input devices to use, etc.).
 */
int LoadGame(const char *path)
{
    if (isloaded){
        CloseGame();
    }
	if(!FCEUI_LoadGame(path, 1)) {
		return 0;
	}

// TODO: tsone: needed for some reason even though FCEUI_LoadGame() calls it. something to do with ResetNES() call there?
	FCEUI_SetVidSystem(PAL);

	isloaded = 1;
	return 1;
}

int CloseGame()
{
	if(!isloaded) {
		return(0);
	}

	FCEUI_CloseGame();

//	DriverKill();
	isloaded = 0;
	GameInfo = 0;

	return(1);
}

static int DoFrame()
{
	uint8 *gfx = 0;
	int32 *sound;
	int32 ssize;

	if (NoWaiting) {
		for (int i = 0; i < TURBO_FRAMESKIPS; ++i) {
			FCEUI_Emulate(&gfx, &sound, &ssize, 2);
			WriteSound(sound, ssize);
		}
	}

	// Get the number of frames to fill the audio buffer.
	int frames = (SOUND_BUF_MAX - GetSoundBufferCount()) / em_sound_frame_samples;

    // On some systems audio can get ahead of visuals. If so, skip emulation for this requestAnimationFrame.
// TODO: tsone: this is not a good solution as it may cause unnecessary skips of the requestAnimationFrame update
	if (IsSoundInitialized() && frames <= 0) {
		return 0;
	}

	FCEUD_UpdateInput();

	// Skip frames (video) to fill the audio buffer. Leave two frames free for next requestAnimationFrame in case they come too frequently.
	if (IsSoundInitialized()) {
		while (frames > 3) {
			FCEUI_Emulate(&gfx, &sound, &ssize, 1);
			WriteSound(sound, ssize);
			--frames;
		}
	}

	FCEUI_Emulate(&gfx, &sound, &ssize, 0);
	WriteSound(sound, ssize);

	return 1;
}

static void ReloadROM(void*)
{
    char *filename = emscripten_run_script_string("Module.romName");
//    CloseGame();
    LoadGame(filename);
}


static void MainLoop()
{
	if (inited & 4) {
		if (GameInfo) {
			if (!DoFrame()) {
				return; // Frame was not processed, skip rest of this callback.
			} else {
				RenderVideo(0);
			}
		} else {
			RenderVideo(1);
		}
	}

// FIXME: tsone: sould be probably using exports
// NOTE: tsone: simple way to communicate with mainloop without "exporting" functions
	int reload = EM_ASM_INT_V({ return Module.romReload||0; });
	if (reload) {
		emscripten_push_main_loop_blocker(ReloadROM, 0);
		EM_ASM({ Module.romReload = 0; });
	}
}

static int DriverInitialize()
{
	InitVideo();
	inited|=4;

	InitSound();
	inited|=1;

	int fourscore = 0;
	g_config->getOption("SDL.FourScore", &fourscore);
// TODO: tsone: this sets two controllers by default, but should be more flexible
	FCEUD_SetInput(fourscore, false, SI_GAMEPAD, SI_GAMEPAD, SIFC_NONE);

	return 1;
}

// TODO: tsone: not used
#if 0
static void DriverKill()
{
	if(inited&4)
		KillVideo();
	if(inited&1)
		KillSound();
	inited=0;
}
#endif

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *fn, const char *m)
{
	std::ios_base::openmode mode = std::ios_base::binary;
	if(!strcmp(m,"r") || !strcmp(m,"rb"))
		mode |= std::ios_base::in;
	else if(!strcmp(m,"w") || !strcmp(m,"wb"))
		mode |= std::ios_base::out | std::ios_base::trunc;
	else if(!strcmp(m,"a") || !strcmp(m,"ab"))
		mode |= std::ios_base::out | std::ios_base::app;
	else if(!strcmp(m,"r+") || !strcmp(m,"r+b"))
		mode |= std::ios_base::in | std::ios_base::out;
	else if(!strcmp(m,"w+") || !strcmp(m,"w+b"))
		mode |= std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
	else if(!strcmp(m,"a+") || !strcmp(m,"a+b"))
		mode |= std::ios_base::in | std::ios_base::out | std::ios_base::app;
    return new EMUFILE_FILE(fn, m);
	//return new std::fstream(fn,mode);
}

FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	return(fopen(fn,mode));
}

static char *s_linuxCompilerString = "emscripten " __VERSION__;
const char *FCEUD_GetCompilerString() {
	return (const char *)s_linuxCompilerString;
}

int main(int argc, char *argv[])
{
	int error;

	FCEUD_Message("Starting " FCEU_NAME_AND_VERSION "...\n");

// TODO: tsone: no flags, as they're not necessary for emscripten
	SDL_Init(0);

	// Initialize the configuration system
	g_config = InitConfig();
		
	// initialize the infrastructure
	error = FCEUI_Initialize();
	std::string s;

    // override savegame, savestate and rom directory with IndexedDB mount at /fceux
    FCEUI_SetDirOverride(FCEUIOD_NV, "fceux/sav");
    FCEUI_SetDirOverride(FCEUIOD_STATES, "fceux/sav");
//    FCEUI_SetDirOverride(FCEUIOD_ROMS, "fceux/rom");

    // update the input devices
//	UpdateInput(g_config);

	// update the emu core
	UpdateEMUCore(g_config);

	{
		int id;
		g_config->getOption("SDL.NewPPU", &id);
		if (id)
			newppu = 1;
	}

	DriverInitialize();

#if SUPER_RATE
// NOTE: tsone: set higher frame rate to ensure emulation stays ahead
//	emscripten_set_main_loop(MainLoop, 100, 1);
	emscripten_set_main_loop(MainLoop, 2 * (em_sound_rate + SOUND_HW_BUF_MAX-1) / SOUND_HW_BUF_MAX, 1);
#else
	emscripten_set_main_loop(MainLoop, 0, 1);
#endif

	return 0;
}

// TODO: tsone: is it necessary?
uint64 FCEUD_GetTime()
{
	return emscripten_get_now();
}

// TODO: tsone: is it necessary?
uint64 FCEUD_GetTimeFreq(void)
{
	// emscripten_get_now() returns time in milliseconds.
	return 1000;
}

void FCEUD_Message(const char *text)
{
	fputs(text, stdout);
}

void FCEUD_PrintError(const char *errormsg)
{
	fprintf(stderr, "%s\n", errormsg);
}


// NOTE: tsone: dummy functions, not implemented
#define DUMMY(__f) \
    void __f(void) {\
        printf("%s\n", #__f);\
        FCEU_DispMessage("Not implemented.",0);\
    }
DUMMY(FCEUD_DebugBreakpoint)
DUMMY(FCEUD_TraceInstruction)
DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) { }
int FCEUD_ShowStatusIcon(void) {return 0;}
bool FCEUI_AviIsRecording(void) {return false;}
void FCEUI_UseInputPreset(int preset) { }
bool FCEUD_PauseAfterPlayback() { return false; }
// These are actually fine, but will be unused and overriden by the current UI code.
void FCEUD_TurboToggle(void) { NoWaiting^= 1; }
FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string fname) { return ArchiveScanRecord(); }

