#include "es2utils.h"
#include <string.h>
#include <math.h>

void mat4Proj(GLfloat *p, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
    const GLfloat f = 1.0 / tan(fovy / 2.0);
    memset(p, 0, 4*4*sizeof(GLfloat));
    p[4*0+0] = f / aspect;
    p[4*1+1] = f;
    p[4*2+2] = (zNear+zFar) / (zNear-zFar);
    p[4*2+3] = -1.0f;
    p[4*3+2] = (2.0f*zNear*zFar) / (zNear-zFar);
}

void mat4Trans(GLfloat *p, GLfloat *trans)
{
    memset(p, 0, 4*4*sizeof(GLfloat));
    p[4*0+0] = 1.0f;
    p[4*1+1] = 1.0f;
    p[4*2+2] = 1.0f;
    p[4*3+3] = 1.0f;
    p[4*3+0] = trans[0];
    p[4*3+1] = trans[1];
    p[4*3+2] = trans[2];
}

void mat4Mul(GLfloat *p, const GLfloat *a, const GLfloat *b)
{
    for (int j = 0; j < 4; j++) {
        for (int i = 0; i < 4; i++) {
            GLfloat v = 0;
            for (int k = 0; k < 4; k++) {
                v += a[4*k + j] * b[4*i + k];
            }
            p[4*i + j] = v;
        }
    }
}

GLuint compileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    return shader;
}

GLuint linkShader(GLuint vert_shader, GLuint frag_shader)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert_shader);
    glAttachShader(prog, frag_shader);
    glLinkProgram(prog);
    glDetachShader(prog, vert_shader);
    glDetachShader(prog, frag_shader);
    glUseProgram(prog);
    return prog;
}

GLuint buildShader(const char *vert_src, const char *frag_src)
{
    GLuint vert_shader = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_src);
    GLuint result = linkShader(vert_shader, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return result;
}

void deleteShader(GLuint *prog)
{
    if (prog && *prog) {
        glDeleteProgram(*prog);
        *prog = 0;
    }
}

void createBuffer(GLuint *buf, GLenum binding, GLsizei size, const void *data)
{
    glGenBuffers(1, buf);
    glBindBuffer(binding, *buf);
    glBufferData(binding, size, data, GL_STATIC_DRAW);
}

void deleteBuffer(GLuint *buf)
{
    if (buf && *buf) {
        glDeleteBuffers(1, buf);
        *buf = 0;
    }
}

void createTex(GLuint *tex, int w, int h, GLenum format, GLenum filter, void *data)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void deleteTex(GLuint *tex)
{
    if (tex && *tex) {
        glDeleteTextures(1, tex);
        *tex = 0;
    }
}

void createFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum filter)
{
    createTex(tex, w, h, format, filter, 0);

    glGenFramebuffers(1, fb);
    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void deleteFBTex(GLuint *tex, GLuint *fb)
{
    deleteTex(tex);
    if (fb && *fb) {
        glDeleteFramebuffers(1, fb);
        *fb = 0;
    }
}

void createMesh(es2_mesh *p, GLuint prog, int num_verts, int num_elems, const GLfloat *verts, const GLfloat *norms, const GLfloat *uvs, const GLushort *elems)
{
// TODO: should memset(0) ?
    p->num_elems = num_elems;

    createBuffer(&p->vert_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, verts);
    createBuffer(&p->norm_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, norms);
    if (uvs) {
        createBuffer(&p->uv_buf, GL_ARRAY_BUFFER, 2*sizeof(GLfloat)*num_verts, uvs);
    } else {
// TODO: should memset(0)?
        p->uv_buf = 0;
    }
    createBuffer(&p->elem_buf, GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_elems, elems);
    p->_vert_loc = glGetAttribLocation(prog, "a_vert");
    p->_norm_loc  = glGetAttribLocation(prog, "a_norm");
    p->_uv_loc  = glGetAttribLocation(prog, "a_uv");
}

void deleteMesh(es2_mesh *p)
{
    deleteBuffer(&p->vert_buf);
    deleteBuffer(&p->norm_buf);
    deleteBuffer(&p->uv_buf);
    deleteBuffer(&p->elem_buf);
}

void meshRender(es2_mesh *p)
{
    glBindBuffer(GL_ARRAY_BUFFER, p->vert_buf);
    glVertexAttribPointer(p->_vert_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    glEnableVertexAttribArray(p->_norm_loc);
    glBindBuffer(GL_ARRAY_BUFFER, p->norm_buf);
    glVertexAttribPointer(p->_norm_loc, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (p->uv_buf) {
        glEnableVertexAttribArray(p->_uv_loc);
        glBindBuffer(GL_ARRAY_BUFFER, p->uv_buf);
        glVertexAttribPointer(p->_uv_loc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->elem_buf);
    glDrawElements(GL_TRIANGLES, p->num_elems, GL_UNSIGNED_SHORT, 0);

    glDisableVertexAttribArray(p->_norm_loc);
    if (p->uv_buf) {
        glDisableVertexAttribArray(p->_uv_loc);
    }
}

