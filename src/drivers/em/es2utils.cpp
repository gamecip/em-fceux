#include "es2utils.h"
#include <string.h>
#include <math.h>

#define VERT_LOC 0
#define NORM_LOC 1
#define UV_LOC   2

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
    glBindAttribLocation(prog, VERT_LOC, "a_vert");
    glBindAttribLocation(prog, NORM_LOC, "a_norm");
    glBindAttribLocation(prog, UV_LOC, "a_uv");
    glLinkProgram(prog);
    glUseProgram(prog);
    return prog;
}

GLuint buildShader(const char *vert_src, const char *frag_src)
{
    GLuint vert_shader = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_src);
    GLuint result = linkShader(vert_shader, frag_shader);
    glDetachShader(result, vert_shader);
    glDetachShader(result, frag_shader);
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

void createTex(GLuint *tex, int w, int h, GLenum format, GLenum min_filter, GLenum mag_filter, void *data)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

    if ((min_filter == GL_LINEAR_MIPMAP_LINEAR)
            || (min_filter == GL_LINEAR_MIPMAP_NEAREST)
            || (min_filter == GL_NEAREST_MIPMAP_LINEAR)
            || (min_filter == GL_NEAREST_MIPMAP_NEAREST)) {
        glGenerateMipmap(GL_TEXTURE_2D);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
}

void deleteTex(GLuint *tex)
{
    if (tex && *tex) {
        glDeleteTextures(1, tex);
        *tex = 0;
    }
}

void createFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum min_filter, GLenum mag_filter)
{
    createTex(tex, w, h, format, min_filter, mag_filter, 0);

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

// DEBUG: tsone: code to find mesh AABB
#include <stdio.h>

void createMesh(es2_mesh *p, GLuint prog, int num_verts, int num_elems, const GLfloat *verts, const GLfloat *norms, const GLfloat *uvs, const GLushort *elems)
{
    memset(p, 0, sizeof(es2_mesh));
    p->num_elems = num_elems;

    createBuffer(&p->vert_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, verts);
// DEBUG: tsone: code to find mesh AABB
    GLfloat mins[3] = { .0f, .0f, .0f }, maxs[3] = { .0f, .0f, .0f };
    for (int i = 0; i < 3*num_verts; i += 3) {
        for (int j = 0; j < 3; ++j) {
            if (verts[i+j] > maxs[j]) {
                maxs[j] = verts[i+j];
            } else if (verts[i+j] < mins[j]) {
                mins[j] = verts[i+j];
            }
        }
    }
    printf("verts:%d aabb: min:%.5f,%.5f,%.5f max:%.5f,%.5f,%.5f\n", num_verts,
        mins[0], mins[1], mins[2], maxs[0], maxs[1], maxs[2]);

    if (norms) {
        createBuffer(&p->norm_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, norms);
    }
    if (uvs) {
        createBuffer(&p->uv_buf, GL_ARRAY_BUFFER, 2*sizeof(GLfloat)*num_verts, uvs);
    }
    if (elems) {
        createBuffer(&p->elem_buf, GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_elems, elems);
    }
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
    glVertexAttribPointer(VERT_LOC, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (p->norm_buf) {
        glEnableVertexAttribArray(NORM_LOC);
        glBindBuffer(GL_ARRAY_BUFFER, p->norm_buf);
        glVertexAttribPointer(NORM_LOC, 3, GL_FLOAT, GL_FALSE, 0, 0);
    } else {
        glDisableVertexAttribArray(NORM_LOC);
    }

    if (p->uv_buf) {
        glEnableVertexAttribArray(UV_LOC);
        glBindBuffer(GL_ARRAY_BUFFER, p->uv_buf);
        glVertexAttribPointer(UV_LOC, 2, GL_FLOAT, GL_FALSE, 0, 0);
    } else {
        glDisableVertexAttribArray(UV_LOC);
    }

    if (p->elem_buf) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->elem_buf);
        glDrawElements(GL_TRIANGLES, p->num_elems, GL_UNSIGNED_SHORT, 0);
    } else {
	    glDrawArrays(GL_TRIANGLE_STRIP, 0, p->num_elems);
    }
}

