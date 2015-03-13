#include "es2utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define VERT_LOC 0
#define NORM_LOC 1
#define EXTRA_LOC 2

void vec3Set(GLfloat *c, const GLfloat *a)
{
    c[0] = a[0];
    c[1] = a[1];
    c[2] = a[2];
}

void vec3Sub(GLfloat *c, const GLfloat *a, const GLfloat *b)
{
    c[0] = a[0] - b[0];
    c[1] = a[1] - b[1];
    c[2] = a[2] - b[2];
}

void vec3Add(GLfloat *c, const GLfloat *a, const GLfloat *b)
{
    c[0] = a[0] + b[0];
    c[1] = a[1] + b[1];
    c[2] = a[2] + b[2];
}

void vec3MulScalar(GLfloat *c, const GLfloat *a, GLfloat s)
{
    c[0] = a[0] * s;
    c[1] = a[1] * s;
    c[2] = a[2] * s;
}

void vec3DivScalar(GLfloat *c, const GLfloat *a, GLfloat s)
{
    c[0] = a[0] / s;
    c[1] = a[1] / s;
    c[2] = a[2] / s;
}

GLfloat vec3Dot(const GLfloat *a, const GLfloat *b)
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void vec3Cross(GLfloat *result, const GLfloat *a, const GLfloat *b)
{
    result[0] = a[1]*b[2] - a[2]*b[1];
    result[1] = a[2]*b[0] - a[0]*b[2];
    result[2] = a[0]*b[1] - a[1]*b[0];
}

GLfloat vec3Length2(const GLfloat *a)
{
    return vec3Dot(a, a);
}

GLfloat vec3Length(const GLfloat *a)
{
    return sqrt(vec3Length2(a));
}

GLfloat vec3Normalize(GLfloat *c, const GLfloat *a)
{
    GLfloat d = vec3Length(a);
    if (d > 0) {
        vec3DivScalar(c, a, d);
    } else {
        c[0] = c[1] = c[2] = 0;
    }
    return d;
}

GLfloat vec3ClosestOnSegment(GLfloat *result, const GLfloat *p, const GLfloat *a, const GLfloat *b)
{
    GLfloat pa[3], ba[3];
    vec3Sub(pa, p, a);
    vec3Sub(ba, b, a);
    GLfloat dotbaba = vec3Length2(ba);
    GLfloat t = 0;
    if (dotbaba != 0) {
        t = vec3Dot(pa, ba) / dotbaba;
        if (t < 0) t = 0;
        else if (t > 1) t = 1;
    }
    vec3MulScalar(ba, ba, t);
    vec3Add(result, a, ba);
    return t;
}

void mat4Persp(GLfloat *p, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
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

    GLint value;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &value);
    if (value == GL_FALSE) {
        printf("ERROR: Shader compilation failed:\n");
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &value);
        if (value > 0) {
          char *buf = (char*) malloc(value);
          glGetShaderInfoLog(shader, value, 0, buf);
          printf("%s\n", buf);
          free(buf);
        } else {
            printf("ERROR: No shader info log!\n");
        }
        printf("Shader source:\n%s\n", src);
    }


    return shader;
}

GLuint linkShader(GLuint vert_shader, GLuint frag_shader)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert_shader);
    glAttachShader(prog, frag_shader);
    glBindAttribLocation(prog, VERT_LOC, "a_vert");
    glBindAttribLocation(prog, NORM_LOC, "a_norm");
    glBindAttribLocation(prog, EXTRA_LOC, "a_extra");
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

// Inverse edge length weighted method. As described in Real-time rendering (3rd ed.) p.546.
// Ref: Max, N. L., (1999) 'Weights for computing vertex normals from facet normals'
//   in Journal of grahics tools, vol.4, no.2, pp.1-6.
static GLfloat* meshGenerateNormals(int num_verts, int num_elems, const GLfloat *verts, const GLushort *elems)
{
    num_verts *= 3;
    GLfloat *norms = (GLfloat*) calloc(num_verts, sizeof(GLfloat));
    GLfloat e1[3], e2[3], no[3];

    for (int i = 0; i < num_elems; i += 3) {
        for (int j = 0; j < 3; ++j) {
            int a = 3*elems[i+j];
            int b = 3*elems[i+((j+1)%3)];
            int c = 3*elems[i+((j+2)%3)];

            vec3Sub(e1, &verts[c], &verts[b]);
            vec3Sub(e2, &verts[a], &verts[b]);
            vec3Cross(no, e1, e2);

            GLfloat d = vec3Length2(e1) * vec3Length2(e2);
            vec3DivScalar(no, no, d);
            vec3Add(&norms[b], &norms[b], no);
        }
    }

    for (int i = 0; i < num_verts; i += 3) {
        vec3Normalize(&norms[i], &norms[i]);
    }

    return norms;
}

// DEBUG: tsone: for code to find mesh AABB below
#include <stdio.h>

void createMesh(es2_mesh *p, int num_verts, int num_elems, int extra_comps, const GLfloat *verts, const GLfloat *norms, const GLfloat *extra, const GLushort *elems)
{
    memset(p, 0, sizeof(es2_mesh));
    p->num_elems = num_elems;
    p->extra_comps = extra_comps;

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
// TODO: tsone: testing normal generation in code. would save some space
#if 0
            GLfloat *ns = meshGenerateNormals(num_verts, num_elems, verts, elems);
            createBuffer(&p->norm_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, ns);
            free(ns);
#else
            createBuffer(&p->norm_buf, GL_ARRAY_BUFFER, 3*sizeof(GLfloat)*num_verts, norms);
#endif
        }
    }

    if (extra) {
        createBuffer(&p->extra_buf, GL_ARRAY_BUFFER, extra_comps*sizeof(GLfloat)*num_verts, extra);
    }

    if (elems) {
        createBuffer(&p->elem_buf, GL_ELEMENT_ARRAY_BUFFER, sizeof(GLushort)*num_elems, elems);
    }
}

void deleteMesh(es2_mesh *p)
{
    deleteBuffer(&p->vert_buf);
    deleteBuffer(&p->norm_buf);
    deleteBuffer(&p->extra_buf);
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

    if (p->extra_buf) {
        glEnableVertexAttribArray(EXTRA_LOC);
        glBindBuffer(GL_ARRAY_BUFFER, p->extra_buf);
        glVertexAttribPointer(EXTRA_LOC, p->extra_comps, GL_FLOAT, GL_FALSE, 0, 0);
    } else {
        glDisableVertexAttribArray(EXTRA_LOC);
    }

    if (p->elem_buf) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->elem_buf);
        glDrawElements(GL_TRIANGLES, p->num_elems, GL_UNSIGNED_SHORT, 0);
    } else {
	    glDrawArrays(GL_TRIANGLE_STRIP, 0, p->num_elems);
    }
}

// num_elems: Must be 3*<number of triangles>, i.e. index buffer number of elements.
int *createUniqueEdges(int *num_edges, int num_elems, const GLushort *elems)
{
    int *edges = (int*) malloc(2*sizeof(int) * num_elems);
//    int *face_edges = (int*) malloc(sizeof(int) * num_elems);
    int n = 0;
    // Find unique edges using O(n^2) process. Small, but not fast.
    for (int i = 0; i < num_elems; i += 3) {
        for (int j = 0; j < 3; ++j) {
            int a0 = elems[i+j];
            int b0 = elems[i+((j+1)%3)];
            int k;
            // Check if we already have the same edge.
            for (k = 0; k < n; k += 2) {
                int a1 = edges[k];
                int b1 = edges[k+1];
                if (a0 == b1 && b0 == a1) {
                    // Duplicate edge with opposite direction.
//                    k = -k;
                    break;
                }
                if (a0 == a1 && b0 == b1) {
                    // Duplicate edge with same direction.
                    break;
                }
            }
            if (k == n) {
                edges[n] = a0;
                edges[n+1] = b0;
                n += 2;
            }
            // Set found edge as face edge.
//            face_edges[i+j] = k / 2;
        }
    }

    *num_edges = n / 2;
    return edges;
}

