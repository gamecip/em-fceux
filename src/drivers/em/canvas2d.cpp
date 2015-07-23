/* FCE Ultra - NES/Famicom Emulator - Emscripten video/window
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
#include <emscripten.h>


// Possible values: 1 or 2, corresponding to 1x and 2x.
#define CANVAS_SCALER 2
#define CANVAS_W (CANVAS_SCALER * 256)
#define CANVAS_H (CANVAS_SCALER * 224)


// Init software canvas rendering.
// TODO: tsone: following must match with es2.cpp
#define NUM_COLORS	(64 * 8)
#define LOOKUP_W	64
#define INPUT_H		240
#define IDX_H		224
#define INPUT_ROW_OFFS	((INPUT_H-IDX_H) / 2)

extern void genNTSCLookup();
extern double *yiqs;

// TODO: tsone: following must match with shaders.h
static const double c_convMat[] = {
	1.0,        1.0,        1.0,       // Y
	0.946882,   -0.274788,  -1.108545, // I
	0.623557,   -0.635691,  1.709007   // Q
};

static uint32 *lookupRGBA = 0;
static uint32 *tmpBuf = 0;


void canvas2DRender(uint8 *pixels, uint8* row_deemp)
{
#if CANVAS_SCALER == 1
	int k = 256 * INPUT_ROW_OFFS;
	int m = 0;
	for (int row = INPUT_ROW_OFFS; row < 224 + INPUT_ROW_OFFS; ++row) {
		int deemp = row_deemp[row];
		for (int x = 256; x != 0; --x) {
			tmpBuf[m] = lookupRGBA[pixels[k] + deemp];
			++m;
			++k;
		}
	}
#else
	int k = 256 * INPUT_ROW_OFFS;
	int m = 0;
	for (int row = INPUT_ROW_OFFS; row < 224 + INPUT_ROW_OFFS; ++row) {
		int deemp = row_deemp[row];
		for (int x = 256; x != 0; --x) {
			tmpBuf[m] = tmpBuf[m + 1] = tmpBuf[m + CANVAS_W]
				= tmpBuf[m+1 + CANVAS_W]
				= lookupRGBA[pixels[k] + deemp];
			m += 2;
			++k;
		}
		m += CANVAS_W;
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
}

void canvas2DInit()
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

	EM_ASM_ARGS({
		var canvas = Module.canvas;
		canvas.width = canvas.widthNative = $0;
		canvas.height = canvas.heightNative = $1;
		FCEM.ctx = Module.createContext(canvas, false, true);
		FCEM.image = FCEM.ctx.getImageData(0, 0, $0, $1);
	}, CANVAS_W, CANVAS_H);

	tmpBuf = (uint32*) malloc(sizeof(uint32) * CANVAS_W*CANVAS_H);
}

