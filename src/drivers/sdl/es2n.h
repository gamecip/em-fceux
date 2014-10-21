#ifndef _ES2N_H_
#define _ES2N_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

typedef struct t_es2n
{
    GLuint quadbuf;
    GLuint rgb_prog;

    GLuint base_tex;
    GLuint ntsc_tex;
    GLuint lvl_tex;  // Framebuffer texture for voltage levels (256x256x4).

    GLuint lvl_fb;   // FB for levels texture generation.
    GLuint lvl_prog; // Level FB shader.
} es2n;

void es2nInit(es2n *p, int left, int right, int top, int bottom);
void es2nDeinit(es2n *p);
void es2nRender(es2n *p, GLushort *pixels);

#endif
