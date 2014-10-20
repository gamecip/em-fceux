#include "sdl.h"
#include "sdl-es2.h"
#include "../common/vidblit.h"
#include "../../utils/memory.h"
#include "es2n.h"

#include <cstring>

#ifndef APIENTRY
#define APIENTRY
#endif

static es2n s_es2n;
static uint16* s_tempXBuf;

void SetOpenGLPalette(uint8 *data)
{
	SetPaletteBlitToHigh((uint8*)data);
}

extern uint8 deempScan[240];
void BlitOpenGL(uint8 *buf)
{
    for (size_t y = 0, i = 0; y < 240; y++) {
        for (size_t x = 0; x < 256; x++) {
            s_tempXBuf[i] = buf[i] | (deempScan[y] << 8);
            i++;
        }
    }

    es2nRender(&s_es2n, s_tempXBuf);

// TODO: remove
#if 0
    // Update uniforms.
    {
        int x, y;
        SDL_GetMouseState(&x, &y);
        GLfloat mousePos[2] = { x / (4*256.0f), y / (4*224.0f) };
        GLint uMousePosLoc = glGetUniformLocation(prog, "u_mousePos");
        glUniform2fv(uMousePosLoc, 1, mousePos);
    }
#endif

	SDL_GL_SwapBuffers();
}

void KillOpenGL(void)
{
    es2nDeinit(&s_es2n);

    if (s_tempXBuf) {
        free(s_tempXBuf);
        s_tempXBuf = 0;
    }
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
    s_tempXBuf = (uint16*) FCEU_malloc(sizeof(uint16) * 256*256);

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

	// In a double buffered setup with page flipping, be sure to clear both buffers.
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	return 1;
}

