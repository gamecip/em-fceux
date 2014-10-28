#ifndef _ES2N_H_
#define _ES2N_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

typedef struct t_es2n
{
    GLuint quadbuf;     // Fullscreen quad vertex buffer.

    GLuint idx_tex;     // Input indexed color texture (NES palette, 256x240).
    GLuint deemp_tex;   // Input de-emphasis bits per row (240x1).
    GLuint lookup_tex;  // Palette to voltage levels lookup texture.

    GLuint signal_fb;   // Framebuffer for signal texture generation.
    GLuint signal_tex;  // Texture of signal (256x256x4).
    GLuint signal_prog; // Shader for signal.

    GLuint rgb_fb;      // Framebuffer for output RGB texture generation.
    GLuint rgb_tex;     // Output RGB texture (1024x256x3).
    GLuint rgb_prog;    // Shader for RGB.

    GLuint disp_prog;   // Shader for final display.

    GLint viewport[4];  // Original viewport.

    GLuint crt_verts_buf;   // Vertex buffer for CRT.
    GLuint crt_elems_buf;   // Element buffer for CRT.
} es2n;

void es2nInit(es2n *p, int left, int right, int top, int bottom);
void es2nDeinit(es2n *p);
void es2nRender(es2n *p, GLubyte *pixels, GLubyte *row_deemp);

#endif
