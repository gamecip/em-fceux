#ifndef _ES2N_H_
#define _ES2N_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

typedef struct t_es2n
{
    GLuint ntscTex;
    GLuint quadbuf;
    GLuint prog;
    GLuint baseTex;
    GLuint kernelTex;
} es2n;

void es2nInit(es2n *p, int left, int right, int top, int bottom);
void es2nDeinit(es2n *p);
void es2nRender(es2n *p, GLushort *pixels);

#endif
