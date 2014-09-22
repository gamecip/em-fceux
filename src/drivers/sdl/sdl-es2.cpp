#include "sdl.h"
#include "sdl-es2.h"
#include "../common/vidblit.h"
#include "../../utils/memory.h"
#include "../common/nes_ntsc.h"

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#include <cstring>
#include <cstdlib>

#ifndef APIENTRY
#define APIENTRY
#endif

#define NTSC_EMULATION 1

static GLuint s_baseTex = 0;
static GLuint s_kernelTex = 0;
#if NTSC_EMULATION 
static uint8* s_tempXBuf = 0;
#endif
static GLuint quadbuf = 0;
static GLuint prog = 0;

//#include "testpixels.inc" 

#if NTSC_EMULATION
static void GenKernelTex()
{
    nes_ntsc_t ntsc;
    nes_ntsc_setup_t ntsc_setup;
    // in case of composite, make blurry
    ntsc_setup = nes_ntsc_composite;
    ntsc_setup.sharpness = -0.15; // make slightly blurry
//    ntsc_setup = nes_ntsc_svideo;
//    ntsc_setup = nes_ntsc_rgb;
//    ntsc_setup = nes_ntsc_monochrome;
    ntsc_setup.merge_fields = 1; // always merge fields
    ntsc_setup.brightness = 0.2; // slightly brigther due to scanline and grille
    nes_ntsc_init(&ntsc, &ntsc_setup, 2, 1);
    
    // convert kernel from nes_ntsc internal type to rgb texture
    size_t size = nes_ntsc_palette_size * nes_ntsc_entry_size;
    uint8* kernel = (uint8*) FCEU_malloc(3 * size);
    nes_ntsc_rgb_t* orig = (nes_ntsc_rgb_t*) ntsc.table;
    for (size_t i = 0; i < size; i++) {
        nes_ntsc_rgb_t p = orig[i];
        kernel[3*i+0] = (255 * ((p >> 21) & 0x3FF)) / 0x3FF;
        kernel[3*i+1] = (255 * ((p >> 11) & 0x3FF)) / 0x3FF;
        kernel[3*i+2] = (255 * ((p >>  1) & 0x3FF)) / 0x3FF;
    }
    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &s_kernelTex);
    glBindTexture(GL_TEXTURE_2D, s_kernelTex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, nes_ntsc_entry_size, nes_ntsc_palette_size, 0,
        GL_RGB, GL_UNSIGNED_BYTE, kernel);
    glActiveTexture(GL_TEXTURE0);

    free(kernel);
}
#endif

void SetOpenGLPalette(uint8 *data)
{
	SetPaletteBlitToHigh((uint8*)data);
}

void BlitOpenGL(uint8 *buf)
{
#if NTSC_EMULATION
//    buf = s_testPixels;
    for (size_t i = 0; i < 256 * 256; i++) {
       s_tempXBuf[i] = (uint8) (((buf[i] & 63) * 255.0f) / 63.0f); 
    }
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_LUMINANCE, GL_UNSIGNED_BYTE, s_tempXBuf);
//    buf = s_testPixels;
#else
	Blit8ToHigh(buf, (uint8*)HiBuffer, 256, 240, 256*4, 1, 1);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_RGBA, GL_UNSIGNED_BYTE, HiBuffer);
#endif

    // Update uniforms.
    {
        // TODO: control ntsc?
        int x, y;
        SDL_GetMouseState(&x, &y);
        GLfloat gridStrength[2] = { 0.1f * x / 602.0f, 0.1f * y / 448.0f }; // grille, scanline
        GLint uGridStrengthLoc = glGetUniformLocation(prog, "u_gridStrength");
        glUniform2fv(uGridStrengthLoc, 1, gridStrength);
    }

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	SDL_GL_SwapBuffers();
}

void KillOpenGL(void)
{
	if (s_baseTex) {
		glDeleteTextures(1, &s_baseTex);
	    s_baseTex = 0;
	}
	if (s_kernelTex) {
		glDeleteTextures(1, &s_kernelTex);
	    s_kernelTex = 0;
	}
    if (prog) {
        glDeleteProgram(prog);
        prog = 0;
    }
#if !NTSC_EMULATION
    if (HiBuffer) {
	    free(HiBuffer);
	    HiBuffer = 0;
    }
#endif
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
#if NTSC_EMULATION
    s_tempXBuf = (uint8*) FCEU_malloc(256 * 256);
#else
	HiBuffer = FCEU_malloc(4*256*256);
	memset(HiBuffer,0x00,4*256*256);
  #ifndef LSB_FIRST
	InitBlitToHigh(4,0xFF000000,0xFF0000,0xFF00,efx&2,0,0);
  #else
	InitBlitToHigh(4,0xFF,0xFF00,0xFF0000,efx&2,0,0);
  #endif
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

    glActiveTexture(GL_TEXTURE0);
	glGenTextures(1, &s_baseTex);
	glBindTexture(GL_TEXTURE_2D, s_baseTex);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,ipolate?GL_LINEAR:GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);

#if NTSC_EMULATION
    GenKernelTex();
#endif

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
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
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "v_uv = vec2(602.0, 240.0) * vert.zw;\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
	GLuint vert_shader = CompileShader(GL_VERTEX_SHADER, vert_src);
    
    const char* frag_src =
#if NTSC_EMULATION
        "precision highp float;\n"
        "#define W 255.0\n"
        "uniform sampler2D u_baseTex;\n"
        "uniform sampler2D u_kernelTex;\n"
        "uniform vec2 u_gridStrength;\n"
        "void main() {\n"
        "vec2  s = vec2(127.0, 1.0);\n"
        "vec2  sIn = vec2(255.0, 255.0);\n"
        "vec2  px = gl_FragCoord.xy - 0.5;\n"
        "vec2  grid = vec2(1.0) - u_gridStrength * mod(px, 2.0);\n"
        "px.y = 240.0-8.0 - floor(0.5*px.y);\n"
        "float m7X = mod(px.x, 7.0);\n"
        "float kX = mod(px.y, 3.0) * 42.0;\n"
        "vec2 pxIn = vec2(3.0 * floor(px.x/7.0) + 1.0, px.y) / W;\n"
        "float o1 = (m7X >= 2.0) ? 1.0/W : -2.0/W;\n"
        "float o2 = (m7X >= 4.0) ? 2.0/W : -1.0/W;\n"

        "float k0Y  = texture2D(u_baseTex, pxIn).r;\n"
        "float k1Y  = texture2D(u_baseTex, pxIn + vec2(o1, 0.0)).r;\n"
        "float k2Y  = texture2D(u_baseTex, pxIn + vec2(o2, 0.0)).r;\n"
        "float kx0Y = texture2D(u_baseTex, pxIn + vec2(-3.0/W, 0.0)).r;\n"
        "float kx1Y = texture2D(u_baseTex, pxIn + vec2(o1-3.0/W, 0.0)).r;\n"
        "float kx2Y = texture2D(u_baseTex, pxIn + vec2(o2-3.0/W, 0.0)).r;\n"

        "vec4 raw = texture2D(u_kernelTex, vec2(kX+m7X, k0Y)/s);\n"
        "raw += texture2D(u_kernelTex, vec2(kX+mod(m7X+12.0,7.0)+14.0, k1Y)/s);\n"
        "raw += texture2D(u_kernelTex, vec2(kX+mod(m7X+10.0,7.0)+28.0, k2Y)/s);\n"
        "raw += texture2D(u_kernelTex, vec2(kX+mod(m7X+7.0,14.0), kx0Y)/s);\n"
        "raw += texture2D(u_kernelTex, vec2(kX+mod(m7X+5.0,7.0)+21.0, kx1Y)/s);\n"
        "raw += texture2D(u_kernelTex, vec2(kX+mod(m7X+3.0,7.0)+35.0, kx2Y)/s);\n"
        "gl_FragColor = grid.x * grid.y * clamp(4.0*fract(raw)-2.0, 0.0, 1.0);\n"
        "}\n";
#else
        "precision lowp float;\n"
        "uniform sampler2D u_baseTex;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "gl_FragColor = texture2D(u_baseTex, v_uv);\n"
        "}\n";
#endif
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

    GLint uBaseTexLoc = glGetUniformLocation(prog, "u_baseTex");
    glUniform1i(uBaseTexLoc, 0);
    GLint uKernelTexLoc = glGetUniformLocation(prog, "u_kernelTex");
    glUniform1i(uKernelTexLoc, 1);
    GLfloat gridStrength[2] = { 0.032, 0.125 }; // grille, scanline
    GLint uGridStrengthLoc = glGetUniformLocation(prog, "u_gridStrength");
    glUniform2fv(uGridStrengthLoc, 1, gridStrength);

	// In a double buffered setup with page flipping, be sure to clear both buffers.
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();
	glClear(GL_COLOR_BUFFER_BIT);
	SDL_GL_SwapBuffers();

	return 1;
}

