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

int webgl_supported = 0;

static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited;
static int s_webgl = -1;

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
		em_sound_frame_samples = em_sound_rate / NTSC_FPS;
	} else {
		em_sound_frame_samples = em_sound_rate / PAL_FPS;
	}
}

static void Resize(int width, int height)
{
	int new_width, new_height;
	const double aspect = width / (double) height;

	if (aspect >= s_targetAspect) {
		new_width = height * s_targetAspect;
		new_height = height;
	} else {
		new_width = width;
		new_height = width / s_targetAspect;
	}

	if ((new_width == s_width) && (new_height == s_height)) {
		return;
	}
	s_width = new_width;
	s_height = new_height;

//	printf("!!!! resize: (%dx%d) '(%dx%d) asp:%f\n", width, height, s_width, s_height, aspect);

	// HACK: emscripten_set_canvas_size() forces canvas size by setting css style
	// width and height with "!important" flag. Workaround is to set size manually
	// and remove the style attribute. See Emscripten's updateCanvasDimensions()
	// in library_browser.js for the faulty code.
	EM_ASM_INT({
		var canvas = Module.canvas2D;
		canvas.style.setProperty( "width", $0 + "px", "important");
		canvas.style.setProperty("height", $1 + "px", "important");

		if ($2) {
			canvas = Module.canvas3D;
			canvas.width = canvas.widthNative = $0;
			canvas.height = canvas.heightNative = $1;
			canvas.style.setProperty( "width", $0 + "px", "important");
			canvas.style.setProperty("height", $1 + "px", "important");
		}

	}, s_width, s_height, webgl_supported);

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

	if (s_webgl) {
		es2Render(XBuf, deempScan, PALRAM[0]);
	} else {
		canvas2DRender(XBuf, deempScan);
	}
}

void EnableWebGL(int enable)
{
	enable = enable ? webgl_supported : 0;

	EM_ASM_ARGS({
		if ($0) {
			Module.canvas = Module.canvas3D;
			Module.ctx = Module.ctx3D;
			Module.canvas3D.style.display = 'block';
			Module.canvas2D.style.display = 'none';
		} else {
			Module.canvas = Module.canvas2D;
			Module.ctx = Module.ctx2D;
			Module.canvas3D.style.display = 'none';
			Module.canvas2D.style.display = 'block';
		}

		Module.useWebGL = $0;
	}, enable);

	s_webgl = enable;
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

//	FCEUI_SetShowFPS(1);

	emscripten_set_resize_callback(0, 0, 0, FCEM_ResizeCallback);

	FCEU_printf("Initializing canvas 2D context.\n");
	canvas2DInit();
	RegisterCallbacksForCanvas();

	EM_ASM({
		Module.canvas2D = Module.canvas;
		Module.ctx2D = Module.ctx;
		Module.canvas = Module.canvas3D;
		Module.ctx = null;
	});

	FCEU_printf("Initializing WebGL.\n");
	webgl_supported = es2Init(s_targetAspect);
	if (webgl_supported) {
		RegisterCallbacksForCanvas();
	}

	// HACK: Manually resize to cover the window inner size.
	// Apparently there's no way to do this with Emscripten...?
	s_width = EM_ASM_INT_V({ return window.innerWidth; });
	s_height = EM_ASM_INT_V({ return window.innerHeight; });
	Resize(s_width, s_height);

	s_inited = 1;
	return 0;
}

void CanvasToNESCoords(uint32 *x, uint32 *y)
{
	*x = 256 * (*x) / s_width;
	*y = 224 * (*y) / s_height;
	*y += s_srendline;
}
