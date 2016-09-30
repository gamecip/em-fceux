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
#include "../../debug.h"
#include "../../fceu.h"
#include "../../ppu.h"
#include "../../version.h"
#include "../../state.h"
#include "../../palette.h"
#include "../../cart.h"
#include <emscripten.h>

extern "C" void gamecip_freeze() __attribute__((used));
extern "C" void gamecip_unfreeze() __attribute__((used));
extern "C" void gamecip_freeze() {
	FCEUSS_Save(0, false);
}
extern "C" void gamecip_unfreeze() {
	FCEUSS_Load(0, false);
}

//size 0x800
extern "C" uint8 * gamecip_ram() {
  return RAM;
}
//size 0x800
extern "C" uint8* gamecip_ntaram() {
  return NTARAM;
}
//size 0x20
extern "C" uint8* gamecip_palram() {
  return PALRAM;
}
//size 4
extern "C" uint8* gamecip_ppu() {
  return PPU;
}
//size 0x100
extern "C" uint8* gamecip_spram() {
  return SPRAM;
}
//0-7
extern "C" uint8* gamecip_VPage(int i) {
  return VPage[i];
}
// //0-3
// extern "C" uint8* gamecip_vnapage(int i) {
//   return vnapage[i];
// }
//0-32, yikes
extern "C" uint8* gamecip_chr(int i) {
  return CHRptr[i];
}
extern "C" uint32 gamecip_chrSize(int i) {
  return CHRsize[i];
}
extern "C" uint8* gamecip_prg(int i) {
  return PRGptr[i];
}
extern "C" uint32 gamecip_prgSize(int i) {
  return PRGsize[i];
}
extern uint8 *CHRRAM;
extern uint32 CHRRAMSIZE;
extern "C" uint8* gamecip_chrRAM() {
  return CHRRAM;
}
extern "C" uint32 gamecip_chrRAMSize() {
  return CHRRAMSIZE;
}

//TODO: linker is not finding FDSRAM, etc... ):
// extern uint8 *FDSRAM;
// extern uint32 FDSRAMSize;
// extern "C" uint8* gamecip_fdsRAM() {
//   return FDSRAM;
// }
// extern "C" uint32 gamecip_fdsRAMSize() {
//   return FDSRAMSize;
// }

// extern uint8 *WRAM;
// extern uint32 WRAMSize;
// extern "C" uint8* gamecip_wRAM() {
//   return WRAM;
// }
// extern "C" uint32 gamecip_wRAMSize() {
//   return WRAMSize;
// }

// //1024 for mmc5
// extern uint8 *ExRAM;
// extern "C" uint8* gamecip_exRAM() {
//   return ExRAM;
// }


// Number of frames to skip per regular frame when frameskipping.
#define TURBO_FRAMESKIPS 3
// Set to 1 to set mainloop call at higher rate than requestAnimationFrame.
// It's recommended to set this to 0, i.e. to use requestAnimationFrame.
#define SUPER_RATE 0

// For s_status.
#define STATUS_INIT (1<<0)
#define STATUS_LOAD (1<<1)

extern double g_fpsScale;
extern bool MaxSpeed;

// TODO: tsone: this is used in fceu.cpp. should be probably defined there, not here?
bool turbo;

int eoptions = 0;
Config *g_config;

static int s_status = 0;

/**
 * Loads a game, given a full path/filename.  The driver code must be
 * initialized after the game is loaded, because the emulator code
 * provides data necessary for the driver code(number of scanlines to
 * render, what virtual input devices to use, etc.).
 */
int LoadGame(const char *path)
{
	if (s_status & STATUS_LOAD) {
		CloseGame();
	}

	// Pass user's autodetect setting to LoadGame().
	int autodetect = (GetController(FCEM_VIDEO_SYSTEM) <= -1) ? 1 : 0;
	if(!FCEUI_LoadGame(path, autodetect)) {
		return 0;
	}
	gamecip_unfreeze();

	s_status |= STATUS_LOAD;
	return 1;
}

int CloseGame()
{
	if (!(s_status & STATUS_LOAD)) {
		return 0;
	}

	FCEUI_CloseGame();

//	DriverKill();
	s_status &= ~STATUS_LOAD;
	GameInfo = 0;
	return 1;
}

static void EmulateFrame(int frameskipmode)
{
	uint8 *gfx = 0;
	int32 *sound;
	int32 ssize;

	FCEUD_UpdateInput();
	FCEUI_Emulate(&gfx, &sound, &ssize, frameskipmode);
	WriteSound(sound, ssize);
}

static int DoFrame()
{
	if (NoWaiting) {
		for (int i = 0; i < TURBO_FRAMESKIPS; ++i) {
			EmulateFrame(2);
		}
	}

	// Get the number of frames to fill the audio buffer.
	int frames = (SOUND_BUF_MAX - GetSoundBufferCount()) / em_sound_frame_samples;

	// It's possible audio to go ahead of visuals. If so, skip all emulation for this frame.
// TODO: tsone: this is not a good solution as it may cause unnecessary skipping in emulation
	if (IsSoundInitialized() && frames <= 0) {
		return 0;
	}

	// Skip frames (video) to fill the audio buffer. Leave two frames free for next requestAnimationFrame in case they come too frequently.
	if (IsSoundInitialized() && (frames > 3)) {
		// Skip only even numbers of frames to correctly display flickering sprites.
		frames = (frames - 3) & (~1);
		while (frames > 0) {
			EmulateFrame(1);
			--frames;
		}
	}

	EmulateFrame(0);
	return 1;
}

static void ReloadROM(void*)
{
	char *filename = emscripten_run_script_string("Module.romName");
//	CloseGame();
	LoadGame(filename);
}

static void MainLoop()
{
	if (s_status & STATUS_INIT) {
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

// FIXME: tsone: should be probably using exported funcs
	int reload = EM_ASM_INT_V({ return Module.romReload||0; });
	if (reload) {
		emscripten_push_main_loop_blocker(ReloadROM, 0);
		EM_ASM({ Module.romReload = 0; });
	}
}

static int DriverInitialize()
{
	InitVideo();
	InitSound();
	s_status |= STATUS_INIT;

	int fourscore = 0; // Set to 1 to enable FourScore.
// TODO: tsone: this sets two controllers by default, but should be more flexible
	FCEUD_SetInput(fourscore, false, SI_GAMEPAD, SI_GAMEPAD, SIFC_NONE);

	return 1;
}

// TODO: tsone: driver is never closed, hence DriverKill() is unused
#if 0
static void DriverKill()
{
	if (s_status & STATUS_INIT) {
		KillVideo();
		KillSound();
	}
	s_status &= ~STATUS_INIT;
}
#endif

EMUFILE_FILE* FCEUD_UTF8_fstream(const char *fn, const char *m)
{
// TODO: tsone: mode variable is not even used, remove?
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
}

FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	return fopen(fn, mode);
}

static char *s_linuxCompilerString = "emscripten " __VERSION__;
const char *FCEUD_GetCompilerString()
{
	return (const char*) s_linuxCompilerString;
}

int main(int argc, char *argv[])
{
	int error;

	FCEUD_Message("Starting " FCEU_NAME_AND_VERSION "...\n");

	// Initialize the configuration system
	g_config = InitConfig();

	// initialize the infrastructure
	error = FCEUI_Initialize();
	std::string s;

	// override savegame, savestate and rom directory with IndexedDB mount at /fceux
	FCEUI_SetDirOverride(FCEUIOD_NV, "fceux/sav");
	FCEUI_SetDirOverride(FCEUIOD_STATES, "fceux/sav");
//	FCEUI_SetDirOverride(FCEUIOD_ROMS, "fceux/rom");

	UpdateEMUCore(g_config);

// TODO: tsone: do not use new PPU, it's probably not working
	int use_new_ppu = 0;
	if (use_new_ppu) {
		newppu = 1;
	}

	DriverInitialize();

#if SUPER_RATE
// NOTE: tsone: UNTESTED! higher call rate to generate frames
	emscripten_set_main_loop(MainLoop, 2 * (em_sound_rate + SOUND_HW_BUF_MAX-1) / SOUND_HW_BUF_MAX, 1);
#else
	emscripten_set_main_loop(MainLoop, 0, 1);
#endif

	return 0;
}

// TODO: tsone: is FCEUD_GetTime() necessary?
uint64 FCEUD_GetTime()
{
	return emscripten_get_now();
}

// TODO: tsone: is FCEUD_GetTimeFreq() necessary?
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


// NOTE: tsone: following are non-implemented "dummy" functions in this driver
#define DUMMY(__f) void __f() {}
DUMMY(FCEUD_DebugBreakpoint)
DUMMY(FCEUD_TraceInstruction)
DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) {}
int FCEUD_ShowStatusIcon(void) {return 0;}
bool FCEUI_AviIsRecording(void) {return false;}
void FCEUI_UseInputPreset(int preset) {}
bool FCEUD_PauseAfterPlayback() { return false; }
// These are actually fine, but will be unused and overriden by the current UI code.
void FCEUD_TurboToggle(void) { NoWaiting^= 1; }
FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string fname) { return ArchiveScanRecord(); }
