#ifndef _ES2N_H_
#define _ES2N_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

typedef struct t_es2n
{
    GLuint quadbuf;     // Fullscreen quad vertex buffer.

    GLuint base_tex;    // Base input texture (indexed NES palette).
    GLuint ntsc_tex;    // Palette to voltage levels lookup texture.

    GLuint lvl_fb;      // Framebuffer for voltage level texture generation.
    GLuint lvl_tex;     // Texture of voltage levels (256x256x4).
    GLuint lvl_prog;    // Shader for levels.

    GLuint rgb_fb;      // Framebuffer for output RGB texture generation.
    GLuint rgb_tex;     // Output RGB texture (1024x256x3).
    GLuint rgb_prog;    // Shader for RGB.

    GLuint disp_prog;   // Shader for final display.

    GLint viewport[4]; // Original viewport.
} es2n;

void es2nInit(es2n *p, int left, int right, int top, int bottom);
void es2nDeinit(es2n *p);
void es2nRender(es2n *p, GLushort *pixels);

#endif
