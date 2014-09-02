#include "main.h"
#include "throttle.h"
#include "config.h"

#include "../common/cheat.h"
#include "../../fceu.h"
#include "../../movie.h"
#include "../../version.h"

#include "input.h"
#include "dface.h"

#include "sdl.h"
#include "sdl-video.h"

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

#if SDL_VERSION_ATLEAST(2, 0, 0)
    #error "SDL 2.0 not supported"
#endif

extern double g_fpsScale;

extern bool MaxSpeed;

int isloaded;

bool turbo = false;

int closeFinishedMovie = 0;

int eoptions=0;
int noGui = 1;

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
	
	// set pal/ntsc
	int id;
	g_config->getOption("SDL.PAL", &id);
	if(id)
		FCEUI_SetVidSystem(1);
	else
		FCEUI_SetVidSystem(0);
	
	isloaded = 1;

	return 1;
}

/**
 * Closes a game.  Frees memory, and deinitializes the drivers.
 */
int
CloseGame()
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

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

static void DoFun(int frameskip, int periodic_saves)
{
	uint8 *gfx;
	int32 *sound;
	int32 ssize;
	static int fskipc = 0;
	static int opause = 0;

#ifdef FRAMESKIP
	fskipc = (fskipc + 1) % (frameskip + 1);
#endif

	if(NoWaiting) {
		gfx = 0;
	}
	FCEUI_Emulate(&gfx, &sound, &ssize, fskipc);
	FCEUD_Update(gfx, sound, ssize);

	if(opause!=FCEUI_EmulationPaused()) {
		opause=FCEUI_EmulationPaused();
		SilenceSound(opause);
	}
}

static void EmscriptenDoFun()
{
    int reloadROM = EM_ASM_INT_V({ return Module.reloadROM||0; });
    if (reloadROM)
    {
        EM_ASM({ Module.reloadROM = 0; });
        CloseGame();
        LoadGame("src/test.nes");
    }
    if (GameInfo)
    {
	    DoFun(0, 0);
    }
}

/**
 * Initialize all of the subsystem drivers: video, audio, and joystick.
 */
static int
DriverInitialize(FCEUGI *gi)
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
static void
DriverKill()
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
void
FCEUD_Update(uint8 *XBuf,
			 int32 *Buffer,
			 int Count)
{
//	int ocount = Count;
	// apply frame scaling to Count
	Count = (int)(Count / g_fpsScale);
	if(Count) {
		int32 can=GetWriteSound();
		static int uflow=0;

// tsone: disabled throttling support for now
#ifndef EMSCRIPTEN
		// don't underflow when scaling fps
		if(can >= GetMaxSound() && g_fpsScale==1.0) uflow=1;	/* Go into massive underflow mode. */
#endif
		if(can > Count) can=Count;
		else uflow=0;

		WriteSound(Buffer,can);

// tsone: disabled throttling support for now
#ifndef EMSCRIPTEN
		//if(uflow) puts("Underflow");
		int32 tmpcan = GetWriteSound();
		// don't underflow when scaling fps
		if(g_fpsScale>1.0 || ((tmpcan < Count*0.90) && !uflow)) {
			if(XBuf && (inited&4) && !(NoWaiting & 2))
				BlitScreen(XBuf);
			Buffer+=can;
			Count-=can;
			if(Count) {
				if(NoWaiting) {
					can=GetWriteSound();
					if(Count>can) Count=can;
					WriteSound(Buffer,Count);
				} else {
// tsone: for some reason causes hang on chrome...?
#ifndef EMSCRIPTEN
					while(Count>0) {
						WriteSound(Buffer,(Count<ocount) ? Count : ocount);
						Count -= ocount;
					}
#endif
				}
			}
		} //else puts("Skipped");
		else if(!NoWaiting && (uflow || tmpcan >= (Count * 1.8))) {
			if(Count > tmpcan) Count=tmpcan;
			while(tmpcan > 0) {
				//	printf("Overwrite: %d\n", (Count <= tmpcan)?Count : tmpcan);
				WriteSound(Buffer, (Count <= tmpcan)?Count : tmpcan);
				tmpcan -= Count;
			}
		}
#ifdef EMSCRIPTEN
// tsone: not yet clear why this is needed
		else {
			if(XBuf && (inited&4)) {
				BlitScreen(XBuf);
			}
		}
#endif

	} else {
		if(!NoWaiting && (!(eoptions&EO_NOTHROTTLE) || FCEUI_EmulationPaused()))
		while (SpeedThrottle())
		{
			FCEUD_UpdateInput();
		}
		if(XBuf && (inited&4)) {
			BlitScreen(XBuf);
		}
	}
	FCEUD_UpdateInput();
	//if(!Count && !NoWaiting && !(eoptions&EO_NOTHROTTLE))
	// SpeedThrottle();
	//if(XBuf && (inited&4))
	//{
	// BlitScreen(XBuf);
	//}
	//if(Count)
	// WriteSound(Buffer,Count,NoWaiting);
	//FCEUD_UpdateInput();

#else // EMSCRIPTEN
    }

    FCEUD_UpdateInput();

    if (XBuf && (inited&4))
    {
        BlitScreen(XBuf);
    }
#endif
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

/**
 * Unimplemented.
 */
void FCEUD_DebugBreakpoint() {
	return;
}

/**
 * Unimplemented.
 */
void FCEUD_TraceInstruction() {
	return;
}


/**
 * The main loop for the SDL.
 */
int main(int argc, char *argv[])
{
	int error, frameskip;

	FCEUD_Message("Starting " FCEU_NAME_AND_VERSION "...\n");

	/* SDL_INIT_VIDEO Needed for (joystick config) event processing? */
	if(SDL_Init(SDL_INIT_VIDEO)) {
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		return(-1);
	}

	// Initialize the configuration system
	g_config = InitConfig();
		
	if(!g_config) {
		SDL_Quit();
		return -1;
	}

	// initialize the infrastructure
	error = FCEUI_Initialize();
	std::string s;

	g_config->getOption("SDL.InputCfg", &s);
	if(s.size() != 0)
	{
	InitVideo(GameInfo);
	InputCfg(s);
	}

    // update the input devices
	UpdateInput(g_config);

	// If x/y res set to 0, store current display res in SDL.LastX/YRes
	int yres, xres;
	g_config->getOption("SDL.XResolution", &xres);
	g_config->getOption("SDL.YResolution", &yres);
	const SDL_VideoInfo* vid_info = SDL_GetVideoInfo();
	if(xres == 0) 
    {
        if(vid_info != NULL)
        {
			g_config->setOption("SDL.LastXRes", vid_info->current_w);
        }
        else
        {
			g_config->setOption("SDL.LastXRes", 512);
        }
    }
	else
	{
		g_config->setOption("SDL.LastXRes", xres);
	}	
    if(yres == 0)
    {
        if(vid_info != NULL)
        {
			g_config->setOption("SDL.LastYRes", vid_info->current_h);
        }
        else
        {
			g_config->setOption("SDL.LastYRes", 448);
        }
    } 
	else
	{
		g_config->setOption("SDL.LastYRes", yres);
	}
	
	// update the emu core
	UpdateEMUCore(g_config);

	{
		int id;
		g_config->getOption("SDL.InputDisplay", &id);
		extern int input_display;
		input_display = id;
		// not exactly an id as an true/false switch; still better than creating another int for that
		g_config->getOption("SDL.SubtitleDisplay", &id); 
		extern int movieSubtitles;
		movieSubtitles = id;
	}
	
	// load the hotkeys from the config life
	setHotKeys();

// tsone: override rom with test.nes
//    ReloadGame();
//	error = LoadGame("src/test.nes");
//	g_config->setOption("SDL.LastOpenFile", "src/test.nes");
//	g_config->save();
	
	{
		int id;
		g_config->getOption("SDL.NewPPU", &id);
		if (id)
			newppu = 1;
	}

	g_config->getOption("SDL.Frameskip", &frameskip);

	// loop playing the game
    emscripten_set_main_loop(EmscriptenDoFun, 0, true);
// tsone: emscripten will never exit
/*
	CloseGame();

	// exit the infrastructure
	FCEUI_Kill();
	SDL_Quit();
*/
	return 0;
}

/**
 * Get the time in ticks.
 */
uint64
FCEUD_GetTime()
{
	return SDL_GetTicks();
}

/**
 * Get the tick frequency in Hz.
 */
uint64
FCEUD_GetTimeFreq(void)
{
	// SDL_GetTicks() is in milliseconds
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


// dummy functions

#define DUMMY(__f) \
    void __f(void) {\
        printf("%s\n", #__f);\
        FCEU_DispMessage("Not implemented.",0);\
    }
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
void FCEUD_TurboOn	(void) { NoWaiting|= 1; }
void FCEUD_TurboOff   (void) { NoWaiting&=~1; }
void FCEUD_TurboToggle(void) { NoWaiting^= 1; }
FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string fname) { return ArchiveScanRecord(); }

