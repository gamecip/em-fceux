#ifndef _ES2UTILS_H_
#define _ES2UTILS_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

typedef struct t_es2_mesh
{
    GLuint vert_buf;
    GLuint norm_buf;
    GLuint uv_buf;
    GLuint elem_buf;

    GLint num_elems;

    // Uniform locations.
    GLint _vert_loc;
    GLint _norm_loc;
    GLint _uv_loc;
} es2_mesh;

void mat4Proj(GLfloat *p, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
void mat4Trans(GLfloat *p, GLfloat *trans);
void mat4Mul(GLfloat *p, const GLfloat *a, const GLfloat *b);
GLuint compileShader(GLenum type, const char *src);
GLuint linkShader(GLuint vert_shader, GLuint frag_shader);
GLuint buildShader(const char *vert_src, const char *frag_src);
void deleteShader(GLuint *prog);
void createBuffer(GLuint *buf, GLenum binding, GLsizei size, const void *data);
void deleteBuffer(GLuint *buf);
void createTex(GLuint *tex, int w, int h, GLenum format, GLenum filter, void *data);
void deleteTex(GLuint *tex);
void createFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum filter);
void deleteFBTex(GLuint *tex, GLuint *fb);
void createMesh(es2_mesh *p, GLuint prog, int num_verts, int num_elems, const GLfloat *verts, const GLfloat *norms, const GLfloat *uvs, const GLushort *elems);
void deleteMesh(es2_mesh *p);
void meshRender(es2_mesh *p);

#endif
