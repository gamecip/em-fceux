/* FCE Ultra - NES/Famicom Emulator - Emscripten video/window
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
#include "es2.h"
#include "../../fceu.h"
#include "../../video.h"
#include "../../utils/memory.h"
#include <emscripten.h>
#include <emscripten/html5.h>


extern uint8 *XBuf;
extern uint8 deempScan[240];
extern uint8 PALRAM[0x20];
extern Config *g_config;

static int s_curbpp;
static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited;

static int s_width, s_height;
// Aspect is adjusted with CRT TV pixel aspect to get proper look.
// In-depth details can be found here:
//   http://wiki.nesdev.com/w/index.php/Overscan
// While 8:7 pixel aspect is probably correct, this uses 292:256 as it
// looks better on LCD. This assumes square pixel aspect in output.
static const double s_targetAspect = (256.0/224.0) * (292.0/256.0);


// Functions only needed for linking.
void SetOpenGLPalette(uint8*) {}
void FCEUD_SetPalette(uint8, uint8, uint8, uint8) {}
void FCEUD_GetPalette(uint8 index, uint8 *r, uint8 *g, uint8 *b)
{
	*r = *g = *b = 0;
}

bool FCEUD_ShouldDrawInputAids()
{
	return false;
}

// Returns 0 on success, -1 on failure.
int KillVideo()
{
// TODO: tsone: never cleanup video?
#if 0
	// return failure if the video system was not initialized
	if(s_inited == 0)
		return -1;

	// if the rest of the system has been initialized, shut it down
	// check for OpenGL and shut it down
	es2Deinit();

	s_inited = 0;
#endif
	return 0;
}

void FCEUD_VideoChanged()
{
	PAL = FSettings.PAL ? 1 : 0;

	if (!PAL) {
// TODO: tsone: should use 60.0988 divisor instead?
		em_sound_frame_samples = em_sound_rate / 60;
	} else {
// TODO: tsone: what's the proper pal divisor?
		em_sound_frame_samples = em_sound_rate / 50;
	}
}

static void Resize(int width, int height)
{
	double aspect = width / (double) height;

	if (aspect >= s_targetAspect) {
		s_width = height * s_targetAspect;
		s_height = height;
	} else {
		s_width = width;
		s_height = width / s_targetAspect;
	}

//	printf("!!!! resize: (%dx%d) '(%dx%d) asp:%f\n", width, height, s_width, s_height, aspect);

	// HACK: emscripten_set_canvas_size() forces canvas size by setting css style
	// width and height with "!important" flag. Workaround is to set size manually
	// and remove the style attribute. See Emscripten's updateCanvasDimensions()
	// in library_browser.js for the faulty code.
	EM_ASM_INT({
		var canvas = Module.canvas;
		canvas.width = canvas.widthNative = $0 |0;
		canvas.height = canvas.heightNative = $1 |0;
		canvas.style.setProperty( "width", ($0 |0) + "px", "important");
		canvas.style.setProperty("height", ($1 |0) + "px", "important");
	}, s_width, s_height);

	es2SetViewport(s_width, s_height);
}

static EM_BOOL FCEM_ResizeCallback(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
	Resize(uiEvent->windowInnerWidth, uiEvent->windowInnerHeight);
	return 1;
}


void RenderVideo(int draw_splash)
{
	if (draw_splash) {
		DrawSplash();
	}
	es2Render(XBuf, deempScan, PALRAM[0]);
}

// Return 0 on success, -1 on failure.
int InitVideo()
{
	if (s_inited) {
		return 0;
	}

	// check the starting, ending, and total scan lines
	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	// HACK: Manually resize to cover the window inner size.
	// Apparently there's no way to do this with Emscripten...?
	s_width = EM_ASM_INT_V({ return window.innerWidth; });
	s_height = EM_ASM_INT_V({ return window.innerHeight; });
	Resize(s_width, s_height);

//	FCEUI_SetShowFPS(1);

	FCEU_printf("Initializing WebGL.\n");

	emscripten_set_resize_callback(0, 0, 0, FCEM_ResizeCallback);

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

	es2Init(s_targetAspect);

	s_inited = 1;
	return 0;
}

void CanvasToNESCoords(uint32 *x, uint32 *y)
{
	*x = 256 * (*x) / s_width;
	*y = 224 * (*y) / s_height;
	*y += s_srendline;
}

