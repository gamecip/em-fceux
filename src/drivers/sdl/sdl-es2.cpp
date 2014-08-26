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

static GLuint texture = 0;
static void *HiBuffer;
static GLuint quadbuf = 0;
static GLuint prog = 0;

void SetOpenGLPalette(uint8 *data)
{
	SetPaletteBlitToHigh((uint8*)data);
}

void BlitOpenGL(uint8 *buf)
{
	glClear(GL_COLOR_BUFFER_BIT);

	Blit8ToHigh(buf, (uint8*)HiBuffer, 256, 240, 256*4, 1, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, HiBuffer);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	SDL_GL_SwapBuffers();
}

void KillOpenGL(void)
{
	if (texture) {
		glDeleteTextures(1, &texture);
	    texture = 0;
	}
    if (prog) {
        glDeleteProgram(prog);
        prog = 0;
    }
    if (HiBuffer) {
	    free(HiBuffer);
	    HiBuffer = 0;
    }
}

static GLuint CompileShader(GLenum type, const char *src)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &src, 0);
	glCompileShader(shader);
    return shader;
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
	HiBuffer = FCEU_malloc(4*256*256);
	memset(HiBuffer,0x00,4*256*256);
  #ifndef LSB_FIRST
	InitBlitToHigh(4,0xFF000000,0xFF0000,0xFF00,efx&2,0,0);
  #else
	InitBlitToHigh(4,0xFF,0xFF00,0xFF0000,efx&2,0,0);
  #endif
 
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

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

	glDisable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    const GLfloat quadverts[4 * 4] = {
        // Packed vertices: x, y, u, v
    	-1.0f, -1.0f, left / 256.0f, bottom / 256.0f,
    	 1.0f, -1.0f, right / 256.0f, bottom / 256.0f,
        -1.0f,  1.0f, left / 256.0f, top / 256.0f, 
    	 1.0f,  1.0f, right / 256.0f, top / 256.0f
    };

	// Create quad vertex buffer.
	glGenBuffers(1, &quadbuf);
	glBindBuffer(GL_ARRAY_BUFFER, quadbuf);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadverts), quadverts, GL_STATIC_DRAW);

	// Create vertex attribute array.
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

	// Create shaders and program.
    const char* vert_src =
        "precision mediump float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 uv;\n"
        "void main() {\n"
        "uv = vert.zw;\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
	GLuint vert_shader = CompileShader(GL_VERTEX_SHADER, vert_src);
    
    const char* frag_src =
        "precision lowp float;\n"
        "uniform sampler2D tex;\n"
        "varying vec2 uv;\n"
        "void main() {\n"
        "gl_FragColor = texture2D(tex, uv);\n"
        "}\n";
	GLuint frag_shader = CompileShader(GL_FRAGMENT_SHADER, frag_src);

	prog = glCreateProgram();
	glAttachShader(prog, vert_shader);
	glAttachShader(prog, frag_shader);
	glLinkProgram(prog);
	glDetachShader(prog, vert_shader);
	glDetachShader(prog, frag_shader);
	glDeleteShader(vert_shader);
	glDeleteShader(frag_shader);
	glUseProgram(prog);

	// In a double buffered setup with page flipping, be sure to clear both buffers.
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	return 1;
}
