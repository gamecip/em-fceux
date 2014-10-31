#include "es2n.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
// TODO: remove, not needed
#include <stdio.h>

#define TEST 1

#define STR(s_) _STR(s_)
#define _STR(s_) #s_

#define IDX_I       0
#define DEEMP_I     1
#define LOOKUP_I    2
#define SIGNAL_I    3
#define RGB_I       4
#define TEX(i_)     (GL_TEXTURE0+(i_))

#define NUM_CYCLES 3
#define NUM_COLORS (64 * 8) // 64 palette colors, 8 color de-emphasis settings.
#define PERSISTENCE_R 1.25*0.165 // Red phosphor persistence.
#define PERSISTENCE_G 1.25*0.205 // Green "
#define PERSISTENCE_B 1.25*0.225 // Blue "

// This source code is modified (and fixed!) from original at:
// http://wiki.nesdev.com/w/index.php/NTSC_video

// Bounds for signal level normalization
#define ATTENUATION 0.746
#define LOWEST      (0.350 * ATTENUATION)
#define HIGHEST     1.962
#define BLACK       0.518
#define WHITE       HIGHEST

#if TEST == 1

#define NUM_SUBPS 4
#define NUM_TAPS 7
// Following must be POT >= NUM_CYCLES*NUM_TAPS*NUM_SUBPS, ex. 3*4*7=84 -> 128
#define LOOKUP_W 128
// Half-widths of kernels, should be multiples of 6.0 (=chroma subcarrier wavelenth in samples / 2)
#define YW2 6.0
#define IW2 12.0
#define QW2 24.0

// TODO: move to es2n struct
static float s_mins[3];
static float s_maxs[3];

// TODO: could be a macro?
static double box(double w2, double center, double x)
{
    return abs(x - center) < w2 ? 1.0 : 0.0;
}

#else // TEST == 0

#define NUM_CYCLES_TEXTURE 4

#endif // TEST

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
#if TEST == 1
	double *yiqs = (double*) calloc(3 * NUM_CYCLES*NUM_SUBPS*NUM_TAPS * NUM_COLORS, sizeof(double));

 	s_mins[0] = s_mins[1] = s_mins[2] = 0.0f;
 	s_maxs[0] = s_maxs[1] = s_maxs[2] = 0.0f;

    // Generate lookup for every color, cycle, tap and subpixel combination.
    // Average the two NTSC fields to generate ideal output.
    // Complexity looks horrid but it's not really.
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
            for (int subp = 0; subp < NUM_SUBPS; subp++) {
                const double kernel_center = 1.5 + 2*subp + 8*(NUM_TAPS/2);

                for (int tap = 0; tap < NUM_TAPS; tap++) {
                    float yiq[3] = {0.0f, 0.0f, 0.0f};

                    for (int field = 0; field < 2; field++) {
                        const int phase = 8 * (cycle+field);
                        const double shift = phase + 3.9;

                        for (int p = 0; p < 8; p++) {
                            const double x = p + 8.0*tap;
                            const double level = ((NTSCsignal(color, phase+p) - BLACK) / (WHITE-BLACK)) / (2.0*8.0);

                            yiq[0] += box(YW2, kernel_center, x) * level;
                            yiq[1] += box(IW2, kernel_center, x) * level * cos(M_PI * (shift+p) / 6.0);
                            yiq[2] += box(QW2, kernel_center, x) * level * sin(M_PI * (shift+p) / 6.0);
                        }
                    }

                    for (int i = 0; i < 3; i++) {
                        s_mins[i] = fmin(s_mins[i], yiq[i]);
                        s_maxs[i] = fmax(s_maxs[i], yiq[i]);
                    }

                    const int k = 3 * (color*NUM_CYCLES*NUM_SUBPS*NUM_TAPS + cycle*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + tap);
                    yiqs[k+0] = yiq[0];
                    yiqs[k+1] = yiq[1];
                    yiqs[k+2] = yiq[2];
                }
            }
        }
    }

	unsigned char *result = (unsigned char*) calloc(3 * LOOKUP_W * NUM_COLORS, sizeof(unsigned char));
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
            for (int subp = 0; subp < NUM_SUBPS; subp++) {
                for (int tap = 0; tap < NUM_TAPS; tap++) {
                    const int ik = 3 * (color*NUM_CYCLES*NUM_SUBPS*NUM_TAPS + cycle*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + tap);
                    const int ok = 3 * (color*LOOKUP_W + cycle*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + tap);
                    for (int i = 0; i < 3; i++) {
                        const double clamped = (yiqs[ik+i]-s_mins[i]) / (s_maxs[i]-s_mins[i]);
                        result[ok+i] = (unsigned char) (255.0 * clamped + 0.5);
                    }
                }
            }
        }
    }

	glActiveTexture(TEX(LOOKUP_I));
    glGenTextures(1, &p->lookup_tex);
    glBindTexture(GL_TEXTURE_2D, p->lookup_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, LOOKUP_W, NUM_COLORS, 0, GL_RGB, GL_UNSIGNED_BYTE, result);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	free(yiqs);
	free(result);
#else
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

    glActiveTexture(TEX(LOOKUP_I));
    glGenTextures(1, &p->lookup_tex);
    glBindTexture(GL_TEXTURE_2D, p->lookup_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NUM_COLORS, NUM_CYCLES_TEXTURE, 0, GL_RGBA, GL_UNSIGNED_BYTE, result);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    free(result);
#endif
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
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

    glGenFramebuffers(1, fb);
    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void setUniforms(GLuint prog)
{
    GLint k;
    k = glGetUniformLocation(prog, "u_idxTex");
    glUniform1i(k, IDX_I);
    k = glGetUniformLocation(prog, "u_deempTex");
    glUniform1i(k, DEEMP_I);
    k = glGetUniformLocation(prog, "u_lookupTex");
    glUniform1i(k, LOOKUP_I);
    k = glGetUniformLocation(prog, "u_signalTex");
    glUniform1i(k, SIGNAL_I);
    k = glGetUniformLocation(prog, "u_rgbTex");
    glUniform1i(k, RGB_I);
#if TEST == 1
    k = glGetUniformLocation(prog, "u_mins");
    glUniform3fv(k, 1, s_mins);
    k = glGetUniformLocation(prog, "u_maxs");
    glUniform3fv(k, 1, s_maxs);
#endif
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
    printf("left:%d right:%d top:%d bottom:%d\n", left, right, top, bottom);
    memset(p, 0, sizeof(es2n));

    p->overscan_pixels = (GLubyte*) malloc(256*240);
    p->overscan_color = 0xFE; // Set bogus value to ensure overscan update.

    glGetIntegerv(GL_VIEWPORT, p->viewport);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR);
    glBlendColor(PERSISTENCE_R, PERSISTENCE_G, PERSISTENCE_B, 0.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnableVertexAttribArray(0);

    // Quad vertices, packed as: x, y, u, v
    const GLfloat quadverts[4 * 4] = {
        -1.0f, -1.0f, left / 512.0f, bottom / 256.0f,
         1.0f, -1.0f, right / 512.0f, bottom / 256.0f,
        -1.0f,  1.0f, left / 512.0f, top / 256.0f, 
         1.0f,  1.0f, right / 512.0f, top / 256.0f
    };

    // Create quad vertex buffer.
    glGenBuffers(1, &p->quadbuf);
    glBindBuffer(GL_ARRAY_BUFFER, p->quadbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadverts), quadverts, GL_STATIC_DRAW);

    // Create CRT vertex buffer.
#define N 8
#define M 6
    const size_t size_verts = sizeof(GLfloat) * 4 * (2*N+1) * (2*M+1);
    GLfloat *crt_verts = (GLfloat*) malloc(size_verts);
    const double dx = 8.9;
    const double dy = 3.6;
    const double dx2 = dx*dx;
    const double dy2 = dy*dy;
    const double mx = sqrt(1.0 + dx2);
    const double my = sqrt(1.0 + dy2);
    for (int j = -M; j <= M; j++) {
        for (int i = -N; i <= N; i++) {
            int k = 4 * ((j+M) * (2*N+1) + i+N);
            const double x = (double) i/N;
            const double y = (double) j/M;
            const double r2 = x*x + y*y;
            crt_verts[k+0] = mx * x / sqrt(dx2 + r2);
            crt_verts[k+1] = my * y / sqrt(dy2 + r2);
            crt_verts[k+2] = 0.5 + 0.5*x;
            crt_verts[k+3] = ((top-bottom) * (0.5 + 0.5*y) + bottom) / 256.0f;
        }
    }
    glGenBuffers(1, &p->crt_verts_buf);
    glBindBuffer(GL_ARRAY_BUFFER, p->crt_verts_buf);
    glBufferData(GL_ARRAY_BUFFER, size_verts, crt_verts, GL_STATIC_DRAW);
    free(crt_verts);

    const size_t size_elems = sizeof(GLushort) * (2 * (2*N+1+1) * (2*M));
    GLushort *crt_elems = (GLushort*) malloc(size_elems);
    int k = 0;
    for (int j = 0; j < 2*M; j++) {
        for (int i = 0; i < 2*N+1; i++) {
            crt_elems[k+0] = i + (j+0) * (2*N+1);
            crt_elems[k+1] = i + (j+1) * (2*N+1);
            k += 2;
        }
        crt_elems[k+0] = crt_elems[k-1];
        crt_elems[k+1] = crt_elems[k-1 - 2*2*N];
        k += 2;
    }
    glGenBuffers(1, &p->crt_elems_buf);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, p->crt_elems_buf);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, size_elems, crt_elems, GL_STATIC_DRAW);
    free(crt_elems);

    // Setup input pixels texture.
    glActiveTexture(TEX(IDX_I));
    glGenTextures(1, &p->idx_tex);
    glBindTexture(GL_TEXTURE_2D, p->idx_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 512, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // Setup input de-emphasis rows texture.
    glActiveTexture(TEX(DEEMP_I));
    glGenTextures(1, &p->deemp_tex);
    glBindTexture(GL_TEXTURE_2D, p->deemp_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, 256, 1, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    genKernelTex(p);

#if TEST == 0
    // Configure levels framebuffer.
    glActiveTexture(TEX(SIGNAL_I));
    makeFBTex(&p->signal_tex, &p->signal_fb, 256, 256, GL_RGBA, GL_NEAREST);
    const char* signal_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[2];\n"
        "void main() {\n"
        "v_uv[0] = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "v_uv[1] = vec2(v_uv[0].y, 0.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* signal_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_idxTex;\n"
        "uniform sampler2D u_deempTex;\n"
        "uniform sampler2D u_lookupTex;\n"
        "uniform float u_field;\n"
        "varying vec2 v_uv[2];\n"
        "void main(void) {\n"
        "    vec2 la = vec2(texture2D(u_idxTex, v_uv[0]).r, texture2D(u_deempTex, v_uv[1]).r);\n"
        "    vec2 p = floor(256.0*v_uv[0]);\n"
        "    vec2 uv = vec2(\n"
        "        dot((255.0/511.0) * vec2(1.0, 64.0), la),\n" // color
        "        mod(p.x - p.y + u_field, 3.0) / 3.0\n" // phase
        "    );\n"
        "    vec4 result = texture2D(u_lookupTex, uv);\n"
        "    gl_FragColor = result;\n"
        "}\n";
    p->signal_prog = buildShader(signal_vert_src, signal_frag_src);
    setUniforms(p->signal_prog);
#endif

    // Configure RGB framebuffer.
    glActiveTexture(TEX(RGB_I));
#if TEST == 1
    makeFBTex(&p->rgb_tex, &p->rgb_fb, NUM_SUBPS*256, 256, GL_RGB, GL_LINEAR);
    const char* rgb_vert_src =
        "precision highp float;\n"
        "#define NUM_TAPS  " STR(NUM_TAPS)  ".0\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[int(NUM_TAPS)];\n"
        "#define S vec2(1.0/512.0, 0.0)\n"
        "#define UV_OUT(i_, o_) v_uv[i_] = vec2(-12.0/512.0, 0.0) + vec2(280.0/256.0, 1.0) * (vert.zw + (o_)*S)\n"
        "void main() {\n"
        "UV_OUT(0,-3.0);\n"
        "UV_OUT(1,-2.0);\n"
        "UV_OUT(2,-1.0);\n"
        "UV_OUT(3, 0.0);\n"
        "UV_OUT(4, 1.0);\n"
        "UV_OUT(5, 2.0);\n"
        "UV_OUT(6, 3.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* rgb_frag_src =
        "precision highp float;\n"
        "#define NUM_SUBPS " STR(NUM_SUBPS) ".0\n"
        "#define NUM_TAPS  " STR(NUM_TAPS)  ".0\n"
        "#define LOOKUP_W  " STR(LOOKUP_W)  ".0\n"
        "#define YW2  " STR(YW2)  "\n"
        "#define IW2  " STR(IW2)  "\n"
        "#define QW2  " STR(QW2)  "\n"
        "#define GAMMA (2.2 / 1.9)\n"
        "uniform sampler2D u_idxTex;\n"
        "uniform sampler2D u_deempTex;\n"
        "uniform sampler2D u_lookupTex;\n"
        "uniform vec3 u_mins;\n"
        "uniform vec3 u_maxs;\n"
        "varying vec2 v_uv[int(NUM_TAPS)];\n"
#if 1 // 0: texture test pass-through
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
        "const vec3 c_gamma = vec3(GAMMA);\n"
        "const vec4 one = vec4(1.0);\n"
        "const vec4 nil = vec4(0.0);\n"
        "\n"
        "#define P(i_)  p = floor(512.0*v_uv[i_])\n"
        "#define U(i_)  (mod(p.x - p.y, 3.0)*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + float(i_)) / (LOOKUP_W-1.0)\n"
        "#define LA(i_) la = vec2(texture2D(u_idxTex, v_uv[i_]).r, texture2D(u_deempTex, vec2(v_uv[i_].y, 0.0)).r)\n"
        "#define V()    dot((255.0/511.0) * vec2(1.0, 64.0), la)\n"
        "#define UV(i_) uv = vec2(U(i_), V())\n"
        "#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)\n"
        "#define SMP(i_) LA(i_); P(i_); UV(i_); yiq += RESCALE(texture2D(u_lookupTex, UV(i_)).rgb)\n"
        "\n"
        "void main(void) {\n"
        "float subp = mod(floor(NUM_SUBPS*512.0*v_uv[2].x), NUM_SUBPS);\n"
        "vec2 p;\n"
        "vec2 la;\n"
        "vec2 uv;\n"
        "vec3 yiq = vec3(0.0);\n"
        "SMP(0);\n"
        "SMP(1);\n"
        "SMP(2);\n"
        "SMP(3);\n"
        "SMP(4);\n"
        "SMP(5);\n"
        "SMP(6);\n"
        "yiq /= (vec3(YW2, IW2, QW2) / (8.0/2.0));\n"
        "vec3 result = c_convMat * yiq;\n"
        "gl_FragColor = vec4(pow(result, c_gamma), 1.0);\n"
        "}\n";
#else
        "void main(void) {\n"
        "gl_FragColor = texture2D(u_lookupTex, v_uv[2]);\n"
        "}\n";
#endif

#else // TEST == 0

    makeFBTex(&p->rgb_tex, &p->rgb_fb, 1024, 256, GL_RGB, GL_LINEAR);
    const char* rgb_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[7];\n"
        "#define S vec2(1.0/256.0, 0.0)\n"
        "#define UV_OUT(i_, o_) v_uv[i_] = vert.zw + (o_)*S\n"
        "void main() {\n"
        "UV_OUT(0,-5.0);\n"
        "UV_OUT(1,-4.0);\n"
        "UV_OUT(2,-3.0);\n"
        "UV_OUT(3,-2.0);\n"
        "UV_OUT(4,-1.0);\n"
        "UV_OUT(5, 0.0);\n"
        "UV_OUT(6, 1.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* rgb_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_signalTex;\n"
        "uniform float u_field;\n"
        "varying vec2 v_uv[7];\n"
#if 1 // 0: texture test pass-through
        "#define PI " STR(M_PI) "\n"
        "#define PIC (PI / 6.0)\n"
        "#define GAMMA (2.2 / 1.9)\n"
        "#define LOWEST  " STR(LOWEST) "\n"
        "#define HIGHEST " STR(HIGHEST) "\n"
        "#define BLACK   " STR(BLACK) "\n"
        "#define WHITE   " STR(WHITE) "\n"
        "#define RESCALE(v_) ((v_) * ((HIGHEST-LOWEST)/(WHITE-BLACK)) + ((LOWEST-BLACK)/(WHITE-BLACK)))\n"
        "#define SMP_IN(i_) v[i_] = RESCALE(texture2D(u_signalTex, v_uv[i_]))\n"
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
        "vec2 coord = floor(4.0*256.0*v_uv[5]);\n"
        "vec2 p = floor(coord / 4.0);\n"
        "vec4 offs = vec4(mod(coord.x, 4.0));\n"

        // q=36, fringe center, smoothing method
        "vec4 v[7];\n"
        "SMP_IN(0);\n"
        "SMP_IN(1);\n"
        "SMP_IN(2);\n"
        "SMP_IN(3);\n"
        "SMP_IN(4);\n"
        "SMP_IN(5);\n"
        "SMP_IN(6);\n"

        "v[0] *= step(0.0, v_uv[0].x);\n"
        "v[1] *= step(0.0, v_uv[1].x);\n"
        "v[2] *= step(0.0, v_uv[2].x);\n"
        "v[3] *= step(0.0, v_uv[3].x);\n"
        "v[4] *= step(0.0, v_uv[4].x);\n"
        "v[6] *= step(v_uv[5].x, 1.0);\n"

        "const vec4 c_base = PIC*(3.9 + vec4(0.5, 2.5, 4.5, 6.5));\n"
        "vec4 scanphase = c_base + PIC*mod(8.0 * (u_field-p.y), 12.0);\n"
        "vec4 ph[7];\n"
        "ph[0] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[0].x));\n"
        "ph[1] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[1].x));\n"
        "ph[2] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[2].x));\n"
        "ph[3] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[3].x));\n"
        "ph[4] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[4].x));\n"
        "ph[5] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[5].x));\n"
        "ph[6] = scanphase + ((8.0*PIC)*floor(256.0*v_uv[6].x));\n"

        "vec4 c0 = clamp01(vec4(1.0, 0.0,-1.0,-2.0) + offs);\n"
        "vec4 c1 = clamp01(vec4(2.0, 3.0, 4.0, 4.0) - offs);\n"
        "vec4 c2 = max(    vec4(0.0, 0.0, 0.0, 1.0) - offs, 0.0);\n"

        "vec3 s[4];\n"
        "s[3]  = sample(ph[0], v[0], one-c0);\n"
        "s[3] += sample(ph[1], v[1], one-c2);\n"
        "s[3] += sample(ph[2], v[2], one-c1);\n"

        "s[2]  = sample(ph[1], v[1], c2);\n"
        "s[2] += sample(ph[2], v[2], c1);\n"
        "s[2] += sample(ph[3], v[3], c0);\n"

        "s[1]  = sample(ph[3], v[3], one-c0);\n"
        "s[1] += sample(ph[4], v[4], one-c2);\n"
        "s[1] += sample(ph[5], v[5], one-c1);\n"

        "s[0]  = sample(ph[4], v[4], c2);\n"
        "s[0] += sample(ph[5], v[5], c1);\n"
        "s[0] += sample(ph[6], v[6], c0);\n"

#if 0
        "vec3 yiq = vec3(\n"
        "    s[1].r,\n"
        "    0.7*s[0].g + s[1].g + 0.7*s[2].g,\n"
        "    1.0*s[0].b + s[1].b + 1.0*s[2].b\n"
        ") / (6.0 * vec3(1.0, 2.4, 3.0));\n"
#elif 1
        "vec3 yiq = vec3(\n"
        "    s[1].r,\n"
        "    s[0].g + s[1].g,\n"
        "    s[0].b + s[1].b\n"
        ") / (6.0 * vec3(1.0, 2.0, 2.0));\n"
#else
        "vec3 yiq = vec3(\n"
        "    s[3].r,\n"
        "    s[2].g + s[3].g,\n"
        "    s[0].b + s[1].b + s[2].b + s[3].b\n"
        ") / (6.0 * vec3(1.0, 2.0, 4.0));\n"
#endif

        "vec3 result = c_convMat * yiq;\n"
        "gl_FragColor = vec4(pow(result, c_gamma), 1.0);\n"
        "}\n";
#else
        "void main(void) {\n"
        "gl_FragColor = texture2D(u_signalTex, v_uv[4]);\n"
        "}\n";
#endif
#endif // TEST
    p->rgb_prog = buildShader(rgb_vert_src, rgb_frag_src);
    setUniforms(p->rgb_prog);

    // Setup display (output) shader.
#if TEST == 1
    const char* disp_vert_src =
        "precision highp float;\n"
        "#define NUM_SUBPS " STR(NUM_SUBPS) ".0\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[5];\n"
        "void main() {\n"
        "vec2 uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "v_uv[0] = uv+vec2(2.0 / (NUM_SUBPS*256.0), 0.0);\n"
        "v_uv[1] = uv+vec2(1.0 / (NUM_SUBPS*256.0), 0.0);\n"
        "v_uv[2] = uv;\n"
        "v_uv[3] = uv-vec2(1.0 / (NUM_SUBPS*256.0), 0.0);\n"
        "v_uv[4] = uv-vec2(2.0 / (NUM_SUBPS*256.0), 0.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* disp_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_rgbTex;\n"
        "varying vec2 v_uv[5];\n"
        "#define SMP(i_, m_) color += (m_) * texture2D(u_rgbTex, v_uv[i_]).rgb\n"
        "void main(void) {\n"
#if 0
        "vec3 color = vec3(0.0);\n"
        "SMP(0, vec3(0.25, 0.00, 0.00));\n"
        "SMP(1, vec3(0.50, 0.25, 0.00));\n"
        "SMP(2, vec3(0.25, 0.50, 0.25));\n"
        "SMP(3, vec3(0.00, 0.25, 0.50));\n"
        "SMP(4, vec3(0.00, 0.00, 0.25));\n"
        "gl_FragColor = vec4(color, 1.0);\n"
#else
        // Direct color
        "gl_FragColor = texture2D(u_rgbTex, v_uv[2]);\n"
#endif

#else // TEST == 0

    const char* disp_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[3];\n"
        "void main() {\n"
        "vec2 uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "v_uv[0] = uv + vec2(1.0/1024.0, 0.0);\n"
        "v_uv[1] = uv;\n"
        "v_uv[2] = uv - vec2(1.0/1024.0, 0.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* disp_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_rgbTex;\n"
        "varying vec2 v_uv[3];\n"
        "void main(void) {\n"
        "vec3 color = vec3(texture2D(u_rgbTex, v_uv[0]).r, texture2D(u_rgbTex, v_uv[1]).g, texture2D(u_rgbTex, v_uv[2]).b);\n"
#if 0
// For 2x size 
        "float luma = dot(vec3(0.299, 0.587, 0.114), color);\n"
        "float scan = mod(floor((2.0*256.0) * v_uv[1].y), 2.0);\n"
        "vec3 result = color + ((1.0-luma) * (5.5/256.0) * (2.0*scan - 1.0));\n"
        "gl_FragColor = vec4(result, 1.0);\n"
#elif 0
// For 3x size 
        "float luma = dot(vec3(0.299, 0.587, 0.114), color);\n"
        "float m = mod(floor((3.0*256.0) * v_uv.y), 3.0);\n"
        "float d = distance(m, 1.0);\n"
        "float scan = 1.0 - d;\n"
        "vec3 result = color + ((1.0-luma) * (3.7/256.0) * (3.0*scan - 1.0));\n"
        "gl_FragColor = vec4(result, 1.0);\n"
#elif 0
// Unfinished! Testing grille
        "vec3 grille;\n"
        "if (m.x >= 3.0) grille =       vec3(0.333, 0.667, 1.000);\n"
        "else if (m.x >= 2.0) grille =  vec3(1.000, 0.667, 0.333);\n"
        "else if (m.x >= 1.0) grille =  vec3(0.333, 0.667, 1.000);\n"
        "else grille =                  vec3(1.000, 0.667, 0.333);\n"
#else
// Direct 
        "gl_FragColor = vec4(color, 1.0);\n"
#endif

#endif // TEST

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
    if (p->idx_tex) {
        glDeleteTextures(1, &p->idx_tex);
        p->idx_tex = 0;
    }
    if (p->lookup_tex) {
        glDeleteTextures(1, &p->lookup_tex);
        p->lookup_tex = 0;
    }
    if (p->rgb_prog) {
        glDeleteProgram(p->rgb_prog);
        p->rgb_prog = 0;
    }
*/
}

// TODO: remove
static void updateUniforms(GLuint prog)
{
}

void es2nRender(es2n *p, GLubyte *pixels, GLubyte *row_deemp, GLubyte overscan_color)
{
    // Update input pixels.
    glActiveTexture(TEX(IDX_I));
    glBindTexture(GL_TEXTURE_2D, p->idx_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    if (p->overscan_color != overscan_color) {
        p->overscan_color = overscan_color;
        printf("overscan: %02X\n", overscan_color);

        memset(p->overscan_pixels, overscan_color, 256*240);
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, 256, 0, 256, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
    }

    // Update input de-emphasis rows.
    glActiveTexture(TEX(DEEMP_I));
    glBindTexture(GL_TEXTURE_2D, p->deemp_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 240, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, row_deemp);

    glBindBuffer(GL_ARRAY_BUFFER, p->quadbuf);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

#if TEST == 1
    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 8, NUM_SUBPS*256, 224);
    glUseProgram(p->rgb_prog);
    updateUniforms(p->rgb_prog);
    glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
#else
    glBindFramebuffer(GL_FRAMEBUFFER, p->signal_fb);
    glViewport(0, 8, 256, 224);
    glUseProgram(p->signal_prog);
    updateUniforms(p->signal_prog);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 8, 1024, 224);
    glUseProgram(p->rgb_prog);
    updateUniforms(p->rgb_prog);
    glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
#endif

    glBindBuffer(GL_ARRAY_BUFFER, p->crt_verts_buf);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(p->viewport[0], p->viewport[1], p->viewport[2], p->viewport[3]);
    glUseProgram(p->disp_prog);
//    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDrawElements(GL_TRIANGLE_STRIP, 2 * (2*N+1+1) * (2*M), GL_UNSIGNED_SHORT, 0);
}

