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
#if 1
	EM_ASM_INT({
		var canvas = Module.canvas;
		canvas.style.setProperty( "width", $0 + "px", "important");
		canvas.style.setProperty("height", $1 + "px", "important");
	}, s_width, s_height);

#else
	EM_ASM_INT({
		var canvas = Module.canvas;
		canvas.width = canvas.widthNative = $0;
		canvas.height = canvas.heightNative = $1;
		canvas.style.setProperty( "width", $0 + "px", "important");
		canvas.style.setProperty("height", $1 + "px", "important");
	}, s_width, s_height);

	es2SetViewport(s_width, s_height);
#endif
}

static EM_BOOL FCEM_ResizeCallback(int eventType, const EmscriptenUiEvent *uiEvent, void *userData)
{
	Resize(uiEvent->windowInnerWidth, uiEvent->windowInnerHeight);
	return 1;
}

// TODO: tsone: refactor 2dcanvas code to own module?
#define DOUBLE_SIZE 0
// Init software canvas rendering.
extern void genNTSCLookup();
extern double *yiqs;
// TODO: tsone: following must match with es2.cpp
#define NUM_COLORS	(64 * 8)
#define LOOKUP_W	64
#define INPUT_H		240
#define IDX_H		224
#define INPUT_ROW_OFFS	((INPUT_H-IDX_H) / 2)
// TODO: tsone: following must match with shaders.h
static const double c_convMat[] = {
	1.0,        1.0,        1.0,       // Y
	0.946882,   -0.274788,  -1.108545, // I
	0.623557,   -0.635691,  1.709007   // Q
};
static uint32 *lookupRGBA = 0;
static uint32 *tmpBuf = 0;

void RenderVideo(int draw_splash)
{
	if (draw_splash) {
		DrawSplash();
	}

#if 1
#if !DOUBLE_SIZE
	int k = 256 * INPUT_ROW_OFFS;
	int m = 0;
	for (int row = INPUT_ROW_OFFS; row < 224 + INPUT_ROW_OFFS; ++row) {
		int deemp = deempScan[row];
		for (int x = 256; x != 0; --x) {
			tmpBuf[m] = lookupRGBA[XBuf[k] + deemp];
			++m;
			++k;
		}
	}
#else
	int k = 256 * INPUT_ROW_OFFS;
	int m = 0;
	for (int y = 0; y < 224; ++y) {
		int deemp = deempScan[y + INPUT_ROW_OFFS];
		for (int x = 256; x != 0; --x) {
			int color = XBuf[k] + deemp;
			++k;
			tmpBuf[m] = tmpBuf[m+1] = lookupRGBA[color];
			m += 2;
		}
	}

	k = 2*256 * 224 - 1;
	m = 2*256 * 2*224 - 1;
	while (k > 0) {
		for (int x = 2*256; x != 0; --x) {
			tmpBuf[m] = tmpBuf[m - 2*256] = tmpBuf[k];
			--m;
			--k;
		}
		m -= 2 * 256;
	}
#endif

	EM_ASM_ARGS({
		var src = $0;
		var data = FCEM.image.data;
		if ((typeof CanvasPixelArray === 'undefined') || !(data instanceof CanvasPixelArray)) {
			if (FCEM.prevData !== data) {
				FCEM.data32 = new Int32Array(data.buffer);
				FCEM.prevData = data;
			}
			FCEM.data32.set(HEAP32.subarray(src, src + FCEM.data32.length));
		} else {
			// ImageData is CanvasPixelArray which doesn't have buffers.
			var dst = -1;
			var val;
			var num = data.length + 1;
			while (--num) {
				val = HEAP32[src++];
				data[++dst] = val & 0xFF;
				data[++dst] = (val >> 8) & 0xFF;
				data[++dst] = (val >> 16) & 0xFF;
				data[++dst] = 0xFF;
			}
		}

		FCEM.ctx.putImageData(FCEM.image, 0, 0);
	}, (ptrdiff_t) tmpBuf >> 2);
#else
	es2Render(XBuf, deempScan, PALRAM[0]);
#endif
}

void InitSWCanvas()
{
	genNTSCLookup();

	lookupRGBA = (uint32*) malloc(sizeof(uint32) * NUM_COLORS);
	for (int color = 0; color < NUM_COLORS; ++color) {
		const int k = 3 * (color*LOOKUP_W + LOOKUP_W-1);
		double *yiq = &yiqs[k];
		double rgb[3] = { 0, 0, 0 };
		for (int x = 0; x < 3; ++x) {
			for (int y = 0; y < 3; ++y) {
				rgb[x] += c_convMat[3*y + x] * yiq[y];
			}
			rgb[x] = 255.0*rgb[x] + 0.5;
			rgb[x] = (rgb[x] > 255) ? 255 : (rgb[x] < 0) ? 0 : rgb[x];
		}
		lookupRGBA[color] = (int) rgb[0] | ((int) rgb[1] << 8) | ((int) rgb[2] << 16) | 0xFF000000;
	}

	EM_ASM({
		var canvas = Module.canvas;
		FCEM.ctx = Module.createContext(canvas, false, true);
		FCEM.ctxCanvas = canvas;
#if !DOUBLE_SIZE
		FCEM.image = FCEM.ctx.createImageData(256, 224);
		canvas.width = canvas.widthNative = 256;
		canvas.height = canvas.heightNative = 224;
#else
		canvas.width = canvas.widthNative = 2*256;
		canvas.height = canvas.heightNative = 2*224;
		FCEM.image = FCEM.ctx.createImageData(2*256, 2*224);
#endif
		FCEM.imageCtx = FCEM.ctx;
	});

#if !DOUBLE_SIZE
	tmpBuf = (uint32*) malloc(sizeof(uint32) * 256*224);
#else
	tmpBuf = (uint32*) malloc(sizeof(uint32) * 2*256*2*224);
#endif
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

#if 1
	InitSWCanvas();
#else
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.alpha = attr.antialias = attr.premultipliedAlpha = 0;
	attr.depth = attr.stencil = attr.preserveDrawingBuffer = attr.preferLowPowerToHighPerformance = attr.failIfMajorPerformanceCaveat = 0;
	attr.enableExtensionsByDefault = 0;
	attr.majorVersion = 1;
	attr.minorVersion = 0;
	EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(0, &attr);
	emscripten_webgl_make_context_current(ctx);

	es2Init(s_targetAspect);
#endif

	s_inited = 1;
	return 0;
}

void CanvasToNESCoords(uint32 *x, uint32 *y)
{
	*x = 256 * (*x) / s_width;
	*y = 224 * (*y) / s_height;
	*y += s_srendline;
}
