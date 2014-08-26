#include "sdl.h"
#include "sdl-es2.h"
#include "../common/vidblit.h"
#include "../../utils/memory.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#include <cstring>
#include <cstdlib>

#ifndef APIENTRY
#define APIENTRY
#endif

static int left,right,top,bottom; // right and bottom are not inclusive.
static GLuint textures[2]={0,0};	// Normal image, scanline overlay.
static int scanlines;
static void *HiBuffer;
static GLuint quadbuf = 0;
static GLuint vert_shader = 0;
static GLuint frag_shader = 0;
static GLuint prog = 0;

static const char* vert_shader_src =
"precision mediump float;\n"
"attribute vec2 pos;\n"
"varying vec2 uv;\n"
"void main() {\n"
"uv = 0.5 + vec2(0.5, -0.5) * sign(pos);\n"
"gl_Position = vec4(pos, 0.0, 1.0);\n"
"}\n";

static const char* frag_shader_src =
"precision lowp float;\n"
"uniform sampler2D tex;\n"
"varying vec2 uv;\n"
"void main() {\n"
"gl_FragColor = texture2D(tex, uv);\n"
"}\n";

static const GLfloat quadverts[4*2] = {
	-1.0f, -1.0f,
	 1.0f, -1.0f,
    -1.0f,  1.0f,
	 1.0f,  1.0f
};

void
SetOpenGLPalette(uint8 *data)
{
	SetPaletteBlitToHigh((uint8*)data);
}

void
BlitOpenGL(uint8 *buf)
{
	glClear(GL_COLOR_BUFFER_BIT);

	Blit8ToHigh(buf, (uint8*)HiBuffer, 256, 240, 256*4, 1, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, HiBuffer);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

#if 0
	if(scanlines) {
		glEnable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, textures[1]);


		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, textures[0]);
	}
#endif
	SDL_GL_SwapBuffers();
}

void
KillOpenGL(void)
{
	if(textures[0]) {
		glDeleteTextures(2, &textures[0]);
	}
	textures[0]=0;
	free(HiBuffer);
	HiBuffer=0;
}
/* Rectangle, left, right(not inclusive), top, bottom(not inclusive). */

int
InitOpenGL(int l,
		int r,
		int t,
		int b,
		double xscale,
		double yscale,
		int efx,
		int ipolate,
		int stretchx,
		int stretchy,
		SDL_Surface *screen)
{
	left=l;
	right=r;
	top=t;
	bottom=b;

	HiBuffer = FCEU_malloc(4*256*256);
	memset(HiBuffer,0x00,4*256*256);
  #ifndef LSB_FIRST
	InitBlitToHigh(4,0xFF000000,0xFF0000,0xFF00,efx&2,0,0);
  #else
	InitBlitToHigh(4,0xFF,0xFF00,0xFF0000,efx&2,0,0);
  #endif
 
	if(screen->flags & SDL_FULLSCREEN)
	{
		xscale=(double)screen->w / (double)(r-l);
		yscale=(double)screen->h / (double)(b-t);
		if(xscale<yscale) yscale = xscale;
		if(yscale<xscale) xscale = yscale;
	}

	{
		int rw=(int)((r-l)*xscale);
		int rh=(int)((b-t)*yscale);
		int sx=(screen->w-rw)/2;     // Start x
		int sy=(screen->h-rh)/2;      // Start y

		if(stretchx) { sx=0; rw=screen->w; }
		if(stretchy) { sy=0; rh=screen->h; }
		glViewport(sx, sy, rw, rh);
	}

	glGenTextures(2, textures);
	scanlines=0;

	if(efx&1)
	{
		uint8 *buf;
		int x,y;

		scanlines=1;

		glBindTexture(GL_TEXTURE_2D, textures[1]);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,ipolate?GL_LINEAR:GL_NEAREST);

		buf=(uint8*)FCEU_dmalloc(256*(256*2)*4);

		for(y=0;y<(256*2);y++)
			for(x=0;x<256;x++)
			{
				buf[y*256*4+x*4]=0;
				buf[y*256*4+x*4+1]=0;
				buf[y*256*4+x*4+2]=0;
				buf[y*256*4+x*4+3]=(y&1)?0x00:0xFF; //?0xa0:0xFF; // <-- Pretty
				//buf[y*256+x]=(y&1)?0x00:0xFF;
			}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, (scanlines==2)?256*4:512, 0,
				GL_RGBA,GL_UNSIGNED_BYTE,buf);

		glBlendFunc(GL_DST_COLOR, GL_SRC_ALPHA);

		FCEU_dfree(buf);
	}

	glBindTexture(GL_TEXTURE_2D, textures[0]);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);	// Background color to black.

	// Create quad vertex buffer.
	glGenBuffers(1, &quadbuf);
	glBindBuffer(GL_ARRAY_BUFFER, quadbuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadverts), quadverts, GL_STATIC_DRAW);

	// Create vertex attribute array.
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	// Create shaders and program.
	vert_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vert_shader, 1, &vert_shader_src, 0);
	glCompileShader(vert_shader);

	frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(frag_shader, 1, &frag_shader_src, 0);
	glCompileShader(frag_shader);

	prog = glCreateProgram();
	glAttachShader(prog, vert_shader);
	glAttachShader(prog, frag_shader);
	glLinkProgram(prog);
	glDetachShader(prog, vert_shader);
	glDetachShader(prog, frag_shader);
	glUseProgram(prog);

	// In a double buffered setup with page flipping, be sure to clear both buffers.
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	return 1;
}
