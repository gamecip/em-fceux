/* FCE Ultra - NES/Famicom Emulator
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
#include "../../utils/memory.h"
#include "es2n.h"
#include <emscripten.h>

#ifndef APIENTRY
#define APIENTRY
#endif

static es2n s_es2n;

extern uint8 deempScan[240];
extern uint8 PALRAM[0x20];

void SetOpenGLPalette(uint8*)
{
}

void BlitScreen(uint8 *buf)
{
// TODO: possibly move somewhere else?
    // Update NTSC controls (from js).
    s_es2n.controls.brightness = EM_ASM_DOUBLE_V({ return Module.brightnessControl||0; });
    s_es2n.controls.contrast = EM_ASM_DOUBLE_V({ return Module.contrastControl||0; });
    s_es2n.controls.color = EM_ASM_DOUBLE_V({ return Module.colorControl||0; });
    s_es2n.controls.gamma = EM_ASM_DOUBLE_V({ return Module.gammaControl||0; });
    s_es2n.controls.glow = EM_ASM_DOUBLE_V({ return Module.glowControl||0; });
    s_es2n.controls.sharpness = EM_ASM_DOUBLE_V({ return Module.sharpnessControl||0; });
    s_es2n.controls.rgbppu = EM_ASM_DOUBLE_V({ return Module.rgbppuControl||0; });

    // Update CRT emulation controls (from js).
    s_es2n.controls.crt_enabled = EM_ASM_INT_V({ return Module.crtControl||0; });
    s_es2n.controls.scanlines = EM_ASM_DOUBLE_V({ return Module.scanlinesControl||0; });
    s_es2n.controls.convergence = EM_ASM_DOUBLE_V({ return Module.convergenceControl||0; });
    s_es2n.controls.noise = EM_ASM_DOUBLE_V({ return Module.noiseControl||0; });

    es2nRender(&s_es2n, buf, deempScan, PALRAM[0]);
//	SDL_GL_SwapBuffers();
}

void KillOpenGL(void)
{
    es2nDeinit(&s_es2n);
}

int InitOpenGL(int left,
		int right,
		int top,
		int bottom,
		double xscale,
		double yscale,
		int stretchx,
		int stretchy)
{
// TODO: tsone: erhrm, what?
/*
	if(screen->flags & SDL_FULLSCREEN)
	{
		xscale=(double)screen->w / (double)(right-left);
		yscale=(double)screen->h / (double)(bottom-top);
		if(xscale<yscale) yscale = xscale;
		if(yscale<xscale) xscale = yscale;
	}
*/

	{
		int rw=(int)((right-left)*xscale);
		int rh=(int)((bottom-top)*yscale);
//		int sx=(screen->w-rw)/2;    // Start x
//		int sy=(screen->h-rh)/2;    // Start y

//		if(stretchx) { sx=0; rw=screen->w; }
//		if(stretchy) { sy=0; rh=screen->h; }
//		glViewport(sx, sy, rw, rh);
		glViewport(0, 0, rw, rh);
	}

    es2nInit(&s_es2n, left, right, top, bottom);

//	glClear(GL_COLOR_BUFFER_BIT);
//	SDL_GL_SwapBuffers();
//	glClear(GL_COLOR_BUFFER_BIT);
//	SDL_GL_SwapBuffers();
	return 1;
}

