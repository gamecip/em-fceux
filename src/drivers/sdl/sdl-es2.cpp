#include "sdl.h"
#include "sdl-es2.h"
#include "../common/vidblit.h"
#include "../../utils/memory.h"
#include "es2n.h"

#ifndef APIENTRY
#define APIENTRY
#endif

static es2n s_es2n;

extern uint8 deempScan[240];
extern uint8 PALRAM[0x20];

void SetOpenGLPalette(uint8*)
{
}

void BlitOpenGL(uint8 *buf)
{
    es2nRender(&s_es2n, buf, deempScan, PALRAM[0]);
	SDL_GL_SwapBuffers();
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
		int efx,
		int ipolate,
		int stretchx,
		int stretchy,
		SDL_Surface *screen)
{
	if(screen->flags & SDL_FULLSCREEN)
	{
		xscale=(double)screen->w / (double)(right-left);
		yscale=(double)screen->h / (double)(bottom-top);
		if(xscale<yscale) yscale = xscale;
		if(yscale<xscale) xscale = yscale;
	}

	{
		int rw=(int)((right-left)*xscale);
		int rh=(int)((bottom-top)*yscale);
		int sx=(screen->w-rw)/2;    // Start x
		int sy=(screen->h-rh)/2;    // Start y

		if(stretchx) { sx=0; rw=screen->w; }
		if(stretchy) { sy=0; rh=screen->h; }
		glViewport(sx, sy, rw, rh);
	}

    es2nInit(&s_es2n, left, right, top, bottom);

	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	return 1;
}

