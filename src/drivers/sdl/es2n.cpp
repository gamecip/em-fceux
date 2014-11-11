#include "es2n.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
// TODO: remove, not needed
#include <stdio.h>

#define STR(s_) _STR(s_)
#define _STR(s_) #s_

#define IDX_I       0
#define DEEMP_I     1
#define LOOKUP_I    2
#define RGB_I       3
#define TEX(i_)     (GL_TEXTURE0+(i_))

#define NUM_CYCLES 3
#define NUM_COLORS (64 * 8) // 64 palette colors, 8 color de-emphasis settings.
#define PERSISTENCE_R 0.165 // Red phosphor persistence.
#define PERSISTENCE_G 0.205 // Green "
#define PERSISTENCE_B 0.225 // Blue "

// This source code is modified (and fixed!) from original at:
// http://wiki.nesdev.com/w/index.php/NTSC_video

// Bounds for signal level normalization
#define ATTENUATION 0.746
#define LOWEST      (0.350 * ATTENUATION)
#define HIGHEST     1.962
#define BLACK       0.518
#define WHITE       HIGHEST

#define DEFINE(name_) "#define " #name_ " float(" STR(name_) ")\n"

#define NUM_SUBPS 4
#define NUM_TAPS 7
// Following must be POT >= NUM_CYCLES*NUM_TAPS*NUM_SUBPS, ex. 3*4*7=84 -> 128
#define LOOKUP_W 128
// Set overscan on left and right sides as 12px (total 24px).
#define OVERSCAN_W 12
#define IDX_W (256 + 2*OVERSCAN_W)
#define RGB_W (NUM_SUBPS * IDX_W)
// Half-width of Y and C box filter kernels.
#define YW2 6.0
#define CW2 12.0

// TODO: move to es2n struct
static float s_mins[3];
static float s_maxs[3];

// Boxcar
static double box(double w2, double center, double x)
{
    return abs(x - center) < w2 ? 1.0 : 0.0;
}

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
	double *yiqs = (double*) calloc(3 * NUM_CYCLES*NUM_SUBPS*NUM_TAPS * NUM_COLORS, sizeof(double));

 	s_mins[0] = s_mins[1] = s_mins[2] = 0.0f;
 	s_maxs[0] = s_maxs[1] = s_maxs[2] = 0.0f;

    // Generate lookup for every color, cycle, tap and subpixel combination.
    // Average the two NTSC fields to generate ideal output.
    // ...Looks horrid, and yes it's complex, but computation is quite fast.
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
            for (int subp = 0; subp < NUM_SUBPS; subp++) {
                const double kernel_center = 1.5 + 2*subp + 8*(NUM_TAPS/2);

                for (int tap = 0; tap < NUM_TAPS; tap++) {
                    float yiq[3] = {0.0f, 0.0f, 0.0f};

                    const int phase0 = 8 * cycle; // Color generator outputs 8 samples per PPU pixel.
                    const int phase1 = phase0 + 8; // PPU skips 1st pixel for less jaggies when interleaved fields are combined.
                    const double shift = phase0 + 6.0 - 6.0*33.0/180.0; // -33 degrees phase shift, UV->IQ

                    for (int p = 0; p < 8; p++) {
                        const double x = p + 8.0*tap;
                        double level0 = NTSCsignal(color, phase0+p);
                        double level1 = NTSCsignal(color, phase1+p);

                        double my = box(YW2, kernel_center, x) / 8.0;
                        double mc = box(CW2, kernel_center, x) / 8.0;
                        level0 = ((level0-BLACK) / (WHITE-BLACK));
                        level1 = ((level1-BLACK) / (WHITE-BLACK));
                        double y = my * (level0+level1) / 2.0;
                        double c = mc * (level0-level1) / 2.0;
                        yiq[0] += y;
                        yiq[1] += 1.40*c * cos(M_PI * (shift+p) / 6.0);
                        yiq[2] += 1.48*c * sin(M_PI * (shift+p) / 6.0);
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
    k = glGetUniformLocation(prog, "u_rgbTex");
    glUniform1i(k, RGB_I);
    k = glGetUniformLocation(prog, "u_mins");
    glUniform3fv(k, 1, s_mins);
    k = glGetUniformLocation(prog, "u_maxs");
    glUniform3fv(k, 1, s_maxs);
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
    printf("left:%d right:%d top:%d bottom:%d\n", left, right, top, bottom);
    memset(p, 0, sizeof(es2n));

    p->overscan_pixels = (GLubyte*) malloc(OVERSCAN_W*240);
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
        -1.0f, -1.0f, left  / 256.0f, bottom / 256.0f,
         1.0f, -1.0f, right / 256.0f, bottom / 256.0f,
        -1.0f,  1.0f, left  / 256.0f, top / 256.0f, 
         1.0f,  1.0f, right / 256.0f, top / 256.0f
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, IDX_W, 256, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
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

    // Configure RGB framebuffer.
    glActiveTexture(TEX(RGB_I));
    makeFBTex(&p->rgb_tex, &p->rgb_fb, RGB_W, 256, GL_RGB, GL_LINEAR);
    const char* rgb_vert_src =
        "precision highp float;\n"
        DEFINE(NUM_TAPS)
        DEFINE(IDX_W)
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[int(NUM_TAPS)];\n"
        "#define S vec2(1.0/IDX_W, 0.0)\n"
        "#define UV_OUT(i_, o_) v_uv[i_] = vert.zw + (o_)*S\n"
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
        DEFINE(NUM_SUBPS)
        DEFINE(NUM_TAPS)
        DEFINE(LOOKUP_W)
        DEFINE(IDX_W)
        DEFINE(YW2)
        DEFINE(CW2)
        "#define GAMMA (2.2 / 1.92)\n"
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
        "#define P(i_)  p = floor(IDX_W*v_uv[i_])\n"
        "#define U(i_)  (mod(p.x - p.y, 3.0)*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + float(i_)) / (LOOKUP_W-1.0)\n"
        "#define LA(i_) la = vec2(texture2D(u_idxTex, v_uv[i_]).r, texture2D(u_deempTex, vec2(v_uv[i_].y, 0.0)).r)\n"
        "#define V()    dot((255.0/511.0) * vec2(1.0, 64.0), la)\n"
        "#define UV(i_) uv = vec2(U(i_), V())\n"
        "#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)\n"
        "#define SMP(i_) LA(i_); P(i_); UV(i_); yiq += RESCALE(texture2D(u_lookupTex, UV(i_)).rgb)\n"
        "\n"
        "void main(void) {\n"
        "float subp = mod(floor(NUM_SUBPS*IDX_W*v_uv[int(NUM_TAPS)/2].x), NUM_SUBPS);\n"
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
        "yiq /= (vec3(YW2, CW2, CW2) / (8.0/2.0));\n"
        "vec3 result = c_convMat * yiq;\n"
        "gl_FragColor = vec4(pow(result, c_gamma), 1.0);\n"
        "}\n";
#else
        "void main(void) {\n"
        "gl_FragColor = texture2D(u_lookupTex, v_uv[2]);\n"
        "}\n";
#endif
    p->rgb_prog = buildShader(rgb_vert_src, rgb_frag_src);
    setUniforms(p->rgb_prog);

    // Setup display (output) shader.
    const char* disp_vert_src =
        "precision highp float;\n"
        DEFINE(RGB_W)
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[5];\n"
        "#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0) \n"
        "void main() {\n"
        "vec2 uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "TAP(0,-2.0);\n"
        "TAP(1,-1.0);\n"
        "TAP(2, 0.0);\n"
        "TAP(3, 1.0);\n"
        "TAP(4, 2.0);\n"
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
#elif 0
        // Sharpen
        "vec3 color = vec3(0.0);\n"
        "SMP(0, vec3(-0.500));\n"
//        "SMP(1, vec3(-0.250));\n"
        "SMP(2, vec3( 2.000));\n"
//        "SMP(3, vec3(-0.250));\n"
        "SMP(4, vec3(-0.500));\n"
        "gl_FragColor = vec4(color, 1.0);\n"
#else
        // Filter nearest on y, linear on x.
        "vec2 uv = v_uv[2];\n"
        "uv.y = floor(256.0 * uv.y)/256.0 + 0.5/256.0;\n"
        "gl_FragColor = texture2D(u_rgbTex, uv);\n"
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
// TODO: cleanup is needed!!!
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
    glTexSubImage2D(GL_TEXTURE_2D, 0, OVERSCAN_W, 0, 256, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    if (p->overscan_color != overscan_color) {
        p->overscan_color = overscan_color;
        printf("overscan: %02X\n", overscan_color);

        memset(p->overscan_pixels, overscan_color, OVERSCAN_W*240);
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, OVERSCAN_W, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
        glTexSubImage2D(GL_TEXTURE_2D, 0, IDX_W-OVERSCAN_W, 0, OVERSCAN_W, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
    }

    // Update input de-emphasis rows.
    glActiveTexture(TEX(DEEMP_I));
    glBindTexture(GL_TEXTURE_2D, p->deemp_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 240, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, row_deemp);

    glBindBuffer(GL_ARRAY_BUFFER, p->quadbuf);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 8, RGB_W, 224);
    glUseProgram(p->rgb_prog);
    updateUniforms(p->rgb_prog);
    glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);

//    glBindBuffer(GL_ARRAY_BUFFER, p->crt_verts_buf);
//    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(p->viewport[0], p->viewport[1], p->viewport[2], p->viewport[3]);
    glUseProgram(p->disp_prog);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
//    glDrawElements(GL_TRIANGLE_STRIP, 2 * (2*N+1+1) * (2*M), GL_UNSIGNED_SHORT, 0);
}

