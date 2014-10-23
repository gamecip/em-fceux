#include "es2n.h"
#include <cstdlib>
#include <cstring>
#include <cmath>

#define STR(s_) _STR(s_)
#define _STR(s_) #s_

#define NUM_CYCLES 3
#define NUM_CYCLES_TEXTURE 4
#define NUM_COLORS (64 * 8) // 64 palette colors, 8 color de-emphasis settings.
#define PERSISTENCE_R 0.16  // Red phosphor persistence.
#define PERSISTENCE_G 0.20  // Green "
#define PERSISTENCE_B 0.22  // Blue "

// Source code modified from:
// http://wiki.nesdev.com/w/index.php/NTSC_video

// Bounds for signal level normalization
#define ATTENUATION 0.746
#define LOWEST      (0.350 * ATTENUATION)
#define HIGHEST     1.962
#define BLACK       0.518
#define WHITE       HIGHEST

// Generate the square wave
static bool inColorPhase(int color, int phase)
{
    return (color + phase) % 12 < 6;
};

// pixel = Pixel color (9-bit) given as input. Bitmask format: "eeellcccc".
// phase = Signal phase. It is a variable that increases by 8 each pixel.
static double NTSCsignal(int pixel, int phase)
{
    // Voltage levels, relative to synch voltage
    const double levels[8] = {
        0.350, 0.518, 0.962, 1.550, // Signal low
        1.094, 1.506, 1.962, 1.962  // Signal high
    };

    // Decode the NES color.
    int color = (pixel & 0x0F);    // 0..15 "cccc"
    int level = (pixel >> 4) & 3;  // 0..3  "ll"
    int emphasis = (pixel >> 6);   // 0..7  "eee"
    if (color > 0x0D) { level = 1; } // Level 1 forced for colors $0E..$0F

    // The square wave for this color alternates between these two voltages:
    float low  = levels[0 + level];
    float high = levels[4 + level];
    if (color == 0) { low = high; } // For color 0, only high level is emitted
    if (color > 0x0C) { high = low; } // For colors $0D..$0F, only low level is emitted

    double signal = inColorPhase(color, phase) ? high : low;

    // When de-emphasis bits are set, some parts of the signal are attenuated:
    if ((color < 0x0E) && ( // Not for colors $0E..$0F (Wiki sample code doesn't have this)
        ((emphasis & 1) && inColorPhase(0, phase))
        || ((emphasis & 2) && inColorPhase(4, phase))
        || ((emphasis & 4) && inColorPhase(8, phase)))) {
        signal = signal * ATTENUATION;
    }

    return signal;
}

static void genKernelTex(es2n *p)
{
    unsigned char *result = (unsigned char*) calloc(4 * NUM_CYCLES_TEXTURE * NUM_COLORS, sizeof(GLubyte));

    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        const int phase = cycle * 8;

        for (int color = 0; color < NUM_COLORS; color++) {

            for (int p = 0; p < 4; p++) {
                double signal;

                signal = (NTSCsignal(color, 2*p + phase) + NTSCsignal(color, 2*p + phase+1)) / 2.0;
                signal = (signal-LOWEST) / (HIGHEST-LOWEST);
                result[4 * (cycle*NUM_COLORS + color) + p] = 0.5 + 255.0 * signal;
            }
        }
    }

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &p->ntsc_tex);
    glBindTexture(GL_TEXTURE_2D, p->ntsc_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NUM_COLORS, NUM_CYCLES_TEXTURE, 0, GL_RGBA, GL_UNSIGNED_BYTE, result);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    glActiveTexture(GL_TEXTURE0);
    free(result);
}

static GLuint compileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    return shader;
}

static GLuint linkShader(GLuint vert_shader, GLuint frag_shader)
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

void makeFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum filter)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glGenFramebuffers(1, fb);
    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void setUniforms(GLuint prog)
{
    GLint k;
    k = glGetUniformLocation(prog, "u_baseTex");
    glUniform1i(k, 0);
    k = glGetUniformLocation(prog, "u_ntscTex");
    glUniform1i(k, 1);
    k = glGetUniformLocation(prog, "u_lvlTex");
    glUniform1i(k, 2);
    k = glGetUniformLocation(prog, "u_rgbTex");
    glUniform1i(k, 3);
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
    memset(p, 0, sizeof(es2n));

    glGetIntegerv(GL_VIEWPORT, p->viewport);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    // Quad vertices, packed as: x, y, u, v
    const GLfloat quadverts[4 * 4] = {
        -1.0f, -1.0f, left / 256.0f, bottom / 256.0f,
         1.0f, -1.0f, right / 256.0f, bottom / 256.0f,
        -1.0f,  1.0f, left / 256.0f, top / 256.0f, 
         1.0f,  1.0f, right / 256.0f, top / 256.0f
    };

    // Create quad vertex buffer.
    glGenBuffers(1, &p->quadbuf);
    glBindBuffer(GL_ARRAY_BUFFER, p->quadbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadverts), quadverts, GL_STATIC_DRAW);

    // Create vertex attribute array.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // Setup input texture.
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &p->base_tex);
    glBindTexture(GL_TEXTURE_2D, p->base_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 256, 256, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    genKernelTex(p);

    // Configure levels framebuffer.
    glActiveTexture(GL_TEXTURE2);
    makeFBTex(&p->lvl_tex, &p->lvl_fb, 256, 256, GL_RGBA, GL_NEAREST);
    const char* lvl_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "v_uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* lvl_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_baseTex;\n"
        "uniform sampler2D u_ntscTex;\n"
        "varying vec2 v_uv;\n"
        "void main(void) {\n"
        "    vec2 la = texture2D(u_baseTex, v_uv).ra;\n"
        "    vec2 p = floor(256.0*v_uv);\n"
        "    vec2 uv = vec2(\n"
        "        dot((255.0/511.0) * vec2(1.0, 64.0), la),\n" // color
        "        mod(p.x - p.y, 3.0) / 3.0\n" // phase
        "    );\n"
        "    vec4 result = texture2D(u_ntscTex, uv);\n"
        "    gl_FragColor = result;\n"
        "}\n";
    p->lvl_prog = buildShader(lvl_vert_src, lvl_frag_src);
    setUniforms(p->lvl_prog);

    // Configure RGB framebuffer.
    glActiveTexture(GL_TEXTURE3);
    makeFBTex(&p->rgb_tex, &p->rgb_fb, 1024, 256, GL_RGB, GL_LINEAR);
    const char* rgb_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[6];\n"
        "#define S vec2(1.0/256.0, 0.0)\n"
        "#define UV_OUT(i_, o_) v_uv[i_] = vert.zw + (o_)*S\n"
        "void main() {\n"
        "UV_OUT(0,-4.0);\n"
        "UV_OUT(1,-3.0);\n"
        "UV_OUT(2,-2.0);\n"
        "UV_OUT(3,-1.0);\n"
        "UV_OUT(4, 0.0);\n"
        "UV_OUT(5, 1.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* rgb_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_lvlTex;\n"
        "uniform vec2 u_mousePos;\n"
        "varying vec2 v_uv[6];\n"
#if 1 // 0: texture test pass-through
        "#define PI " STR(M_PI) "\n"
        "#define PIC (PI / 6.0)\n"
        "#define GAMMA (2.2 / 1.89)\n"
        "#define LOWEST  " STR(LOWEST) "\n"
        "#define HIGHEST " STR(HIGHEST) "\n"
        "#define BLACK   " STR(BLACK) "\n"
        "#define WHITE   " STR(WHITE) "\n"
        "#define RESCALE(v_) ((v_) * ((HIGHEST-LOWEST)/(WHITE-BLACK)) + ((LOWEST-BLACK)/(WHITE-BLACK)))\n"
        "#define SMP_IN(i_) v[i_] = RESCALE(texture2D(u_lvlTex, v_uv[i_]))\n"
        "const mat3 c_convMat = mat3(\n"
        "    1.0,        1.0,        1.0,       // Y\n"
#if 0
        // from nesdev wiki
        "    0.946882,   -0.274788,  -1.108545, // I\n"
        "    0.623557,   -0.635691,  1.709007   // Q\n"
#else
        // from nes_ntsc by blargg, default
        "    0.956,   -0.272,  -1.105, // I\n"
        "    0.621,   -0.647,  1.702   // Q\n"
#endif
        ");\n"
        "\n"
        "#define LOWEST  " STR(LOWEST) "\n"
        "#define HIGHEST " STR(HIGHEST) "\n"
        "#define BLACK   " STR(BLACK) "\n"
        "#define WHITE   " STR(WHITE) "\n"
        "#define clamp01(v) clamp(v, 0.0, 1.0)\n"
        "\n"
        "const vec3 c_gamma = vec3(GAMMA);\n"
        "const vec4 one = vec4(1.0);\n"
        "const vec4 nil = vec4(0.0);\n"
        "\n"
        "vec3 sample(in vec4 a, in vec4 v, in vec4 k) {\n"
        "    return vec3(\n"
        "        dot(k, v),\n"
        "        dot(k, v*cos(a)),\n"
        "        dot(k, v*sin(a))\n"
        "    );\n"
        "}\n"
        "void main(void) {\n"
        "vec2 coord = floor(4.0*256.0*v_uv[4]);\n"
        "vec2 p = floor(coord / 4.0);\n"
        "vec4 offs = vec4(mod(coord.x, 4.0));\n"

        // q=36, fringe center, smoothing method
        "vec4 v[6];\n"
        "SMP_IN(0);\n"
        "SMP_IN(1);\n"
        "SMP_IN(2);\n"
        "SMP_IN(3);\n"
        "SMP_IN(4);\n"
        "SMP_IN(5);\n"

        "v[0] *= step(0.0, v_uv[0].x);\n"
        "v[1] *= step(0.0, v_uv[1].x);\n"
        "v[2] *= step(0.0, v_uv[2].x);\n"
        "v[3] *= step(0.0, v_uv[3].x);\n"
        "v[5] *= step(v_uv[5].x, 1.0);\n"

        "const vec4 c_base = PIC*(3.9 + vec4(0.5, 2.5, 4.5, 6.5));\n"
        "vec4 scanphase = c_base - PIC*mod(p.y*8.0, 12.0);\n"
        "vec4 ph[6];\n"
        "ph[0] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[0].x));\n"
        "ph[1] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[1].x));\n"
        "ph[2] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[2].x));\n"
        "ph[3] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[3].x));\n"
        "ph[4] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[4].x));\n"
        "ph[5] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[5].x));\n"

        "vec4 c0 = clamp01(vec4(1.0, 0.0,-1.0,-2.0) + offs);\n"
        "vec4 c1 = clamp01(vec4(2.0, 3.0, 4.0, 4.0) - offs);\n"
        "vec4 c2 = max(    vec4(0.0, 0.0, 0.0, 1.0) - offs, 0.0);\n"

        "vec3 s[3];\n"
        "s[2]  = sample(ph[0], v[0], c2);\n"
        "s[2] += sample(ph[1], v[1], c1);\n"
        "s[2] += sample(ph[2], v[2], c0);\n"

        "s[1]  = sample(ph[2], v[2], one-c0);\n"
        "s[1] += sample(ph[3], v[3], one-c2);\n"
        "s[1] += sample(ph[4], v[4], one-c1);\n"

        "s[0]  = sample(ph[3], v[3], c2);\n"
        "s[0] += sample(ph[4], v[4], c1);\n"
        "s[0] += sample(ph[5], v[5], c0);\n"

        "vec3 yiq = vec3(\n"
        "    s[0].r,\n"
        "    s[0].g + s[1].g,\n"
        "    s[0].b + s[1].b + 0.5*s[2].b\n"
        ") / (6.0 * vec3(1.0, 2.0, 2.5));\n"

        "vec3 result = c_convMat * yiq;\n"
        "gl_FragColor = vec4(pow(result, c_gamma), 1.0);\n"
        "}\n";
#else
        "void main(void) {\n"
        "gl_FragColor = texture2D(u_lvlTex, v_uv[4]);\n"
        "}\n";
#endif
    p->rgb_prog = buildShader(rgb_vert_src, rgb_frag_src);
    setUniforms(p->rgb_prog);

    // Setup display (output) shader.
    const char* disp_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "v_uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* disp_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_rgbTex;\n"
        "varying vec2 v_uv;\n"
        "void main(void) {\n"
        "vec3 color = texture2D(u_rgbTex, v_uv).rgb;\n"
#if 1
        "float luma = dot(vec3(0.299, 0.587, 0.114), color);\n"
        "float m = mod(floor((3.0*256.0) * v_uv.y), 3.0);\n"
        "float d = distance(m, 1.0);\n"
        "float scan = 1.0 - d;\n"
        "vec3 result = color + ((1.0-luma) * (3.0/256.0) * (3.0*scan - 1.0));\n"
        "gl_FragColor = vec4(result, 1.0);\n"
#else
        "vec3 grille;\n"
        "if (m.x >= 3.0) grille =       vec3(0.333, 0.667, 1.000);\n"
        "else if (m.x >= 2.0) grille =  vec3(1.000, 0.667, 0.333);\n"
        "else if (m.x >= 1.0) grille =  vec3(0.333, 0.667, 1.000);\n"
        "else grille =                  vec3(1.000, 0.667, 0.333);\n"
#endif
        "}\n";
    p->disp_prog = buildShader(disp_vert_src, disp_frag_src);
    setUniforms(p->disp_prog);

// TODO: remove from final
/*
    GLfloat mousePos[2] = { 0.0f, 0.0f };
    GLint uMousePosLoc = glGetUniformLocation(p->rgb_prog, "u_mousePos");
    glUniform2fv(uMousePosLoc, 1, mousePos);
*/
}

void es2nDeinit(es2n *p)
{
// TODO: is cleanup needed?
/*
    if (p->base_tex) {
        glDeleteTextures(1, &p->base_tex);
        p->base_tex = 0;
    }
    if (p->ntsc_tex) {
        glDeleteTextures(1, &p->ntsc_tex);
        p->ntsc_tex = 0;
    }
    if (p->rgb_prog) {
        glDeleteProgram(p->rgb_prog);
        p->rgb_prog = 0;
    }
*/
}

void es2nRender(es2n *p, GLushort *pixels)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, p->base_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pixels);

    glBindFramebuffer(GL_FRAMEBUFFER, p->lvl_fb);
    glViewport(0, 8, 256, 224);
    glUseProgram(p->lvl_prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 8, 1024, 224);
    glUseProgram(p->rgb_prog);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR);
    glBlendColor(PERSISTENCE_R, PERSISTENCE_G, PERSISTENCE_B, 0.0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(p->viewport[0], p->viewport[1], p->viewport[2], p->viewport[3]);
    glUseProgram(p->disp_prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

