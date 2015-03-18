#ifndef _ES2UTILS_H_
#define _ES2UTILS_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2platform.h>

#define VARRAY_ENCODED_NORMALS 1

typedef struct t_es2_varray
{
    int count;
    GLenum type;
    int flags;
    const void *data;
    GLuint _buf;
} es2_varray;

typedef struct t_es2_mesh
{
    es2_varray *varrays;
    int num_varrays;
    GLuint elem_buf;
    GLenum elem_type;
    int num_elems;
} es2_mesh;

void vec3Set(GLfloat *c, const GLfloat *a);
void vec3Sub(GLfloat *c, const GLfloat *a, const GLfloat *b);
void vec3Add(GLfloat *c, const GLfloat *a, const GLfloat *b);
void vec3MulScalar(GLfloat *c, const GLfloat *a, GLfloat s);
void vec3DivScalar(GLfloat *c, const GLfloat *a, GLfloat s);
GLfloat vec3Dot(const GLfloat *a, const GLfloat *b);
void vec3Cross(GLfloat *result, const GLfloat *a, const GLfloat *b);
GLfloat vec3Length2(const GLfloat *a);
GLfloat vec3Length(const GLfloat *a);
GLfloat vec3Normalize(GLfloat *c, const GLfloat *a);
GLfloat vec3ClosestOnSegment(GLfloat *result, const GLfloat *p, const GLfloat *a, const GLfloat *b);
void mat4Persp(GLfloat *p, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
void mat4Trans(GLfloat *p, GLfloat *trans);
void mat4Mul(GLfloat *p, const GLfloat *a, const GLfloat *b);
GLuint compileShader(GLenum type, const char *src);
GLuint linkShader(GLuint vert_shader, GLuint frag_shader);
GLuint buildShader(const char *vert_src, const char *frag_src);
void deleteShader(GLuint *prog);
void createBuffer(GLuint *buf, GLenum binding, GLsizei size, const void *data);
void deleteBuffer(GLuint *buf);
void createTex(GLuint *tex, int w, int h, GLenum format, GLenum filter, GLenum wrap, void *data);
void deleteTex(GLuint *tex);
void createFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum filter, GLenum wrap);
void deleteFBTex(GLuint *tex, GLuint *fb);
void createMesh(es2_mesh *p, int num_verts, int num_varrays, es2_varray *varrays, int num_elems, const void *elems);
void deleteMesh(es2_mesh *p);
void meshRender(es2_mesh *p);
int *createUniqueEdges(int *num_edges, int num_verts, int num_elems, const void *elems);

#endif
