#include "main.h"
#include "throttle.h"
#include "config.h"

#include "../common/cheat.h"
#include "../../fceu.h"
#include "../../movie.h"
#include "../../version.h"

#include "input.h"
#include "dface.h"

#include "em.h"
#include "em-video.h"
#include <SDL.h>

#include "../common/configSys.h"
#include "../../oldmovie.h"
#include "../../types.h"

#include <emscripten.h>

#include <unistd.h>
#include <csignal>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <climits>
#include <cmath>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>


#define TURBO_FRAMESKIPS 3

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

extern double g_fpsScale;

extern bool MaxSpeed;

int isloaded;

int eoptions=0;

static int inited = 0;

static void DriverKill(void);
static int DriverInitialize(FCEUGI *gi);
uint64 FCEUD_GetTime();
int gametype = 0;
static int noconfig;

// global configuration object
Config *g_config;

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

	ParseGIInput(GameInfo);
	RefreshThrottleFPS();

	if(!DriverInitialize(GameInfo)) {
		return(0);
	}
	
// TODO: tsone: support for PAL?
        // NTSC=0, PAL=1
	FCEUI_SetVidSystem(0);
	
	isloaded = 1;

	return 1;
}

extern "C" {
// Write savegame and synchronize IDBFS contents to IndexedDB. Must be a C-function.
void FCEM_onSaveGameInterval()
{
    if (GameInterface) {
        GameInterface(GI_SAVE);
    }
    EM_ASM({
      FS.syncfs(FCEM.onSyncToIDB);
    });
}
}

/**
 * Closes a game.  Frees memory, and deinitializes the drivers.
 */
int CloseGame()
{
	if(!isloaded) {
		return(0);
	}

	FCEUI_CloseGame();

	DriverKill();
	isloaded = 0;
	GameInfo = 0;

	InputUserActiveFix();
	return(1);
}

static void DoFrame()
{
	uint8 *gfx = 0;
	int32 *sound;
	int32 ssize;
	static int opause = 0;

	if (NoWaiting) {
		for (int i = 0; i < TURBO_FRAMESKIPS; ++i) {
			FCEUI_Emulate(&gfx, &sound, &ssize, 2);
			FCEUD_Update(gfx, sound, ssize);
		}
	}

// TODO: tsone: assumes NTSC 60Hz
	if (GetSoundBufferCount() > SOUND_BUF_MAX - (SOUND_RATE/60)) {
		// Audio buffer has no capacity, i.e. update cycle is ahead of time -> skip the cycle.
		return;
	}

	// In case of lag, try to fill audio buffer to critical minimum by skipping frames.
// TODO: tsone: assumes NTSC 60Hz
	int frameskips = (2*SOUND_HW_BUF_MAX - GetSoundBufferCount()) / (SOUND_RATE/60);
	while (frameskips > 0) {
		FCEUI_Emulate(&gfx, &sound, &ssize, 1);
		FCEUD_Update(gfx, sound, ssize);
		--frameskips;
	}

	FCEUI_Emulate(&gfx, &sound, &ssize, 0);
	FCEUD_Update(gfx, sound, ssize);
	
	FCEUD_UpdateInput();

	if (gfx && (inited & 4)) {
		BlitScreen(gfx);
	}

	if(opause!=FCEUI_EmulationPaused()) {
		opause=FCEUI_EmulationPaused();
		SilenceSound(opause);
	}
}

static void ReloadROM(void*)
{
    char *filename = emscripten_run_script_string("Module.romName");
    CloseGame();
    LoadGame(filename);
}

static void MainLoop()
{
// NOTE: tsone: simple way to communicate with mainloop without "exporting" functions
    int reload = EM_ASM_INT_V({ return Module.romReload||0; });
    if (reload)
    {
        emscripten_push_main_loop_blocker(ReloadROM, 0);
        EM_ASM({ Module.romReload = 0; });
    }
    else if (GameInfo)
    {
	DoFrame();
    }
}

/**
 * Initialize all of the subsystem drivers: video, audio, and joystick.
 */
static int DriverInitialize(FCEUGI *gi)
{
	if(InitVideo(gi) < 0) return 0;
	inited|=4;

	if(InitSound())
		inited|=1;

	if(InitJoysticks())
		inited|=2;

	int fourscore=0;
	g_config->getOption("SDL.FourScore", &fourscore);
	eoptions &= ~EO_FOURSCORE;
	if(fourscore)
		eoptions |= EO_FOURSCORE;

	InitInputInterface();
	return 1;
}

/**
 * Shut down all of the subsystem drivers: video, audio, and joystick.
 */
static void DriverKill()
{
	if (!noconfig)
		g_config->save();

	if(inited&2)
		KillJoysticks();
	if(inited&4)
		KillVideo();
	if(inited&1)
		KillSound();
	inited=0;
}

/**
 * Update the video, audio, and input subsystems with the provided
 * video (XBuf) and audio (Buffer) information.
 */
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{
	WriteSound(Buffer, Count);
}

/**
 * Opens a file to be read a byte at a time.
 */
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

/**
 * Opens a file, C++ style, to be read a byte at a time.
 */
FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	return(fopen(fn,mode));
}

static char *s_linuxCompilerString = "g++ " __VERSION__;
/**
 * Returns the compiler string.
 */
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

	g_config->getOption("SDL.InputCfg", &s);
	if(s.size() != 0)
	{
	InitVideo(GameInfo);
	InputCfg(s);
	}

    // update the input devices
	UpdateInput(g_config);

	// update the emu core
	UpdateEMUCore(g_config);

	{
		int id;
		g_config->getOption("SDL.NewPPU", &id);
		if (id)
			newppu = 1;
	}

// NOTE: tsone: set higher frame rate to ensure emulation stays ahead
	emscripten_set_main_loop(MainLoop, 100, true);

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

/**
* Prints a textual message without adding a newline at the end.
*
* @param text The text of the message.
*
* TODO: This function should have a better name.
**/
void FCEUD_Message(const char *text)
{
	fputs(text, stdout);
}

/**
* Shows an error message in a message box.
* (For now: prints to stderr.)
* 
* If running in GTK mode, display a dialog message box of the error.
*
* @param errormsg Text of the error message.
**/
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

