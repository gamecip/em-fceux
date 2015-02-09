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

/// \file
/// \brief Handles the graphical game display for the SDL implementation.

#include "sdl.h"
#include "sdl-es2.h"
//#include "../common/vidblit.h"
#include "../../fceu.h"
#include "../../version.h"
#include "../../video.h"

#include "../../utils/memory.h"

#include "sdl-icon.h"
#include "dface.h"

#include "../common/configSys.h"
#include "sdl-video.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

// GLOBALS
extern Config *g_config;

// STATIC GLOBALS
extern SDL_Surface *s_screen;

static SDL_Surface *s_IconSurface = NULL;

static int s_curbpp;
static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited;

static double s_exs, s_eys;
static int s_clipSides;
static int noframe;
static int s_nativeWidth = -1;
static int s_nativeHeight = -1;

#define NWIDTH	(256 - (s_clipSides ? 16 : 0))
#define NOFFSET	(s_clipSides ? 8 : 0)

static int s_paletterefresh;


//draw input aids if we are fullscreen
bool FCEUD_ShouldDrawInputAids()
{
	return false;
}
 
/**
 * Attempts to destroy the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */
int
KillVideo()
{
	// if the IconSurface has been initialized, destroy it
	if(s_IconSurface) {
		SDL_FreeSurface(s_IconSurface);
		s_IconSurface=0;
	}

	// return failure if the video system was not initialized
	if(s_inited == 0)
		return -1;
    
	// if the rest of the system has been initialized, shut it down
	// check for OpenGL and shut it down
	KillOpenGL();

	// shut down the SDL video sub-system
	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	s_inited = 0;
	return 0;
}

/**
 * These functions determine an appropriate scale factor for fullscreen/
 */
inline double GetXScale(int xres)
{
	return ((double)xres) / NWIDTH;
}
inline double GetYScale(int yres)
{
	return ((double)yres) / s_tlines;
}
void FCEUD_VideoChanged()
{
	int buf;
	g_config->getOption("SDL.PAL", &buf);
	if(buf)
		PAL = 1;
	else
		PAL = 0;
}

/**
 * Attempts to initialize the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */
int
InitVideo(FCEUGI *gi)
{
	// XXX soules - const?  is this necessary?
	const SDL_VideoInfo *vinf;
	int error, flags = 0;
	int doublebuf, xstretch, ystretch, xres, yres, show_fps;

	FCEUI_printf("Initializing video...");

	// load the relevant configuration variables
	g_config->getOption("SDL.DoubleBuffering", &doublebuf);
	g_config->getOption("SDL.XStretch", &xstretch);
	g_config->getOption("SDL.YStretch", &ystretch);
	g_config->getOption("SDL.LastXRes", &xres);
	g_config->getOption("SDL.LastYRes", &yres);
	g_config->getOption("SDL.ClipSides", &s_clipSides);
	g_config->getOption("SDL.NoFrame", &noframe);
	g_config->getOption("SDL.ShowFPS", &show_fps);

	// check the starting, ending, and total scan lines
	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	// check if we should auto-set x/y resolution

    // check for OpenGL and set the global flags
	flags = SDL_OPENGL;

	// initialize the SDL video subsystem if it is not already active
	if(!SDL_WasInit(SDL_INIT_VIDEO)) {
		error = SDL_InitSubSystem(SDL_INIT_VIDEO);
		if(error) {
			FCEUD_PrintError(SDL_GetError());
			return -1;
		}
	}
	s_inited = 1;

	// determine if we can allocate the display on the video card
	vinf = SDL_GetVideoInfo();
	if(vinf->hw_available) {
		flags |= SDL_HWSURFACE;
	}
    
	// get the monitor's current resolution if we do not already have it
	if(s_nativeWidth < 0) {
		s_nativeWidth = vinf->current_w;
	}
	if(s_nativeHeight < 0) {
		s_nativeHeight = vinf->current_h;
	}

	// check to see if we are showing FPS
	FCEUI_SetShowFPS(show_fps);
    
	if(noframe) {
		flags |= SDL_NOFRAME;
	}

	// enable double buffering if requested and we have hardware support
	FCEU_printf("Initializing with OpenGL.\n");
	if(doublebuf) {
		 SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	}

	int desbpp;
	g_config->getOption("SDL.BitsPerPixel", &desbpp);

	g_config->getOption("SDL.XScale", &s_exs);
	g_config->getOption("SDL.YScale", &s_eys);

	// -Video Modes Tag-

	if(s_exs <= 0.01) {
		FCEUD_PrintError("xscale out of bounds.");
		KillVideo();
		return -1;
	}
	if(s_eys <= 0.01) {
		FCEUD_PrintError("yscale out of bounds.");
		KillVideo();
		return -1;
	}

	s_screen = SDL_SetVideoMode((int)(NWIDTH * s_exs),
							(int)(s_tlines * s_eys),
							desbpp, flags);
	if(!s_screen) {
		FCEUD_PrintError(SDL_GetError());
		return -1;
	}
	 
	s_curbpp = s_screen->format->BitsPerPixel;
	if(!s_screen) {
		FCEUD_PrintError(SDL_GetError());
		KillVideo();
		return -1;
	}

	FCEU_printf(" Video Mode: %d x %d x %d bpp %s\n",
				s_screen->w, s_screen->h, s_screen->format->BitsPerPixel,
				"");

	if(s_curbpp != 8 && s_curbpp != 16 && s_curbpp != 24 && s_curbpp != 32) {
		FCEU_printf("  Sorry, %dbpp modes are not supported by FCE Ultra.  Supported bit depths are 8bpp, 16bpp, and 32bpp.\n", s_curbpp);
		KillVideo();
		return -1;
	}

	// if the game being run has a name, set it as the window name
	if(gi)
	{
		if(gi->name) {
			SDL_WM_SetCaption((const char *)gi->name, (const char *)gi->name);
		} else {
			SDL_WM_SetCaption(FCEU_NAME_AND_VERSION,"FCE Ultra");
		}
	}

	// create the surface for displaying graphical messages
#ifdef LSB_FIRST
	s_IconSurface = SDL_CreateRGBSurfaceFrom((void *)fceu_playicon.pixel_data,
											32, 32, 24, 32 * 3,
											0xFF, 0xFF00, 0xFF0000, 0x00);
#else
	s_IconSurface = SDL_CreateRGBSurfaceFrom((void *)fceu_playicon.pixel_data,
											32, 32, 24, 32 * 3,
											0xFF0000, 0xFF00, 0xFF, 0x00);
#endif
	SDL_WM_SetIcon(s_IconSurface,0);
	s_paletterefresh = 1;

	if(s_curbpp > 8) {
		if(!InitOpenGL(NOFFSET, 256 - (s_clipSides ? 8 : 0),
					s_srendline, s_erendline + 1,
					s_exs, s_eys,
					xstretch, ystretch, s_screen)) 
		{
			FCEUD_PrintError("Error initializing OpenGL.");
			KillVideo();
			return -1;
		}
	}
	return 0;
}

static SDL_Color* s_psdl = 0;

/**
 * Sets the color for a particular index in the palette.
 */
void
FCEUD_SetPalette(uint8 index,
                 uint8 r,
                 uint8 g,
                 uint8 b)
{
    FCEU_ARRAY_EM(s_psdl, SDL_Color, 256); 

	s_psdl[index].r = r;
	s_psdl[index].g = g;
	s_psdl[index].b = b;

	s_paletterefresh = 1;
}

/**
 * Gets the color for a particular index in the palette.
 */
void
FCEUD_GetPalette(uint8 index,
				uint8 *r,
				uint8 *g,
				uint8 *b)
{
	if (s_psdl) {
		*r = s_psdl[index].r;
		*g = s_psdl[index].g;
		*b = s_psdl[index].b;
	}
}

/** 
 * Pushes the palette structure into the underlying video subsystem.
 */
static void RedoPalette()
{
	if (s_psdl) {
    		SetOpenGLPalette((uint8*)s_psdl);
	}
}

/**
 * Pushes the given buffer of bits to the screen.
 */
void BlitScreen(uint8 *XBuf)
{
	if(!s_screen) {
		return;
	}

	// refresh the palette if required
	if(s_paletterefresh) {
		RedoPalette();
		s_paletterefresh = 0;
	}

	BlitOpenGL(XBuf);
}

/**
 *  Converts an x-y coordinate in the window manager into an x-y
 *  coordinate on FCEU's screen.
 */
uint32
PtoV(uint16 x,
	uint16 y)
{
	y = (uint16)((double)y / s_eys);
	x = (uint16)((double)x / s_exs);
	if(s_clipSides) {
		x += 8;
	}
	y += s_srendline;
	return (x | (y << 16));
}

// TODO: tsone: AVI recording should be removed, these are unnecessary
bool enableHUDrecording = false;
bool FCEUI_AviEnableHUDrecording()
{
	if (enableHUDrecording)
		return true;

	return false;
}
void FCEUI_SetAviEnableHUDrecording(bool enable)
{
	enableHUDrecording = enable;
}

bool disableMovieMessages = false;
bool FCEUI_AviDisableMovieMessages()
{
	if (disableMovieMessages)
		return true;

	return false;
}
void FCEUI_SetAviDisableMovieMessages(bool disable)
{
	disableMovieMessages = disable;
}
