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
#include "../../fceu.h"
#include "../../video.h"
#include "../../utils/memory.h"
#include <emscripten.h>
#include <emscripten/html5.h>


#define NWIDTH	(256 - (s_clipSides ? 16 : 0))
#define NOFFSET	(s_clipSides ? 8 : 0)

extern Config *g_config;

static int s_curbpp;
static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited;

static double s_exs, s_eys;
static int s_clipSides;
static int s_nativeWidth = -1;
static int s_nativeHeight = -1;

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
	// return failure if the video system was not initialized
	if(s_inited == 0)
		return -1;
    
	// if the rest of the system has been initialized, shut it down
	// check for OpenGL and shut it down
	KillOpenGL();

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
	if(FSettings.PAL)
		PAL = 1;
	else
		PAL = 0;
}

// Return 0 on success, -1 on failure.
int
InitVideo(FCEUGI *gi)
{
	s_clipSides = 0; // Don't clip left side.

	// check the starting, ending, and total scan lines
	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	// Scale x to compensate the 24px overscan.
	s_exs = 4.0 * (280.0/256.0) + 0.5/256.0;
	s_eys = 4.0;
	int w = (int) (NWIDTH * s_exs);
	int h = (int) (s_tlines * s_eys);
	s_nativeWidth = w;
	s_nativeHeight = h;

	s_inited = 1;

	FCEUI_SetShowFPS(1);
    
	FCEU_printf("Initializing WebGL.\n");

	emscripten_set_canvas_size(w, h);
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.alpha = attr.antialias = attr.premultipliedAlpha = 0;
	attr.depth = attr.stencil = attr.preserveDrawingBuffer = attr.preferLowPowerToHighPerformance = attr.failIfMajorPerformanceCaveat = 0;
	attr.enableExtensionsByDefault = 0;
	attr.majorVersion = 1;
	attr.minorVersion = 0;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(0, &attr);
	emscripten_webgl_make_context_current(ctx);

	s_curbpp = 32;

	if(!InitOpenGL(NOFFSET, 256 - (s_clipSides ? 8 : 0),
				s_srendline, s_erendline + 1,
				s_exs, s_eys)) 
	{
		FCEUD_PrintError("Error initializing OpenGL.");
		KillVideo();
		return -1;
	}

	return 0;
}

// TODO: tsone: may be removed if fceux doesn't really use these..
void FCEUD_SetPalette(uint8, uint8, uint8, uint8) {}
void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	*r = *g = *b = 0;
}

/**
 *  Converts an x-y coordinate in the window manager into an x-y
 *  coordinate on FCEU's screen.
 */
void PtoV(int *x, int *y)
{
	*y /= s_eys;
	*x /= s_exs;
	if (s_clipSides) {
		*x += 8;
	}
	*y += s_srendline;
}

