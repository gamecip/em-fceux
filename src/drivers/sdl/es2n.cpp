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
#define STRETCH_I   4
#define TEX(i_)     (GL_TEXTURE0+(i_))

#define PERSISTENCE_R 0.165 // Red phosphor persistence.
#define PERSISTENCE_G 0.205 // Green "
#define PERSISTENCE_B 0.225 // Blue "

// Bounds for signal level normalization
#define ATTENUATION 0.746
#define LOWEST      (0.350 * ATTENUATION)
#define HIGHEST     1.962
#define BLACK       0.518
#define WHITE       HIGHEST

#define DEFINE(name_) "#define " #name_ " float(" STR(name_) ")\n"

// 64 palette colors, 8 color de-emphasis settings.
#define NUM_COLORS (64 * 8)
#define NUM_PHASES 3
#define NUM_SUBPS 4
#define NUM_TAPS 5
// Following must be POT >= NUM_PHASES*NUM_TAPS*NUM_SUBPS, ex. 3*5*4=60 -> 64
#define LOOKUP_W 64
// Set overscan on left and right sides as 12px (total 24px).
#define OVERSCAN_W 12
#define IDX_W (256 + 2*OVERSCAN_W)
#define RGB_W (NUM_SUBPS * IDX_W)
#define STRETCH_H (4 * 256)
// Half-width of Y and C box filter kernels.
#define YW2 6.0
#define CW2 12.0
// CRT mesh X and Y half-resolution.
#define CRT_XN 8
#define CRT_YN 6

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

static GLuint buildShader(const char *vert_src, const char *frag_src)
{
    GLuint vert_shader = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_src);
    GLuint result = linkShader(vert_shader, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return result;
}

static void deleteShader(GLuint *prog)
{
    if (prog && *prog) {
        glDeleteProgram(*prog);
        *prog = 0;
    }
}

static void createBuffer(GLuint *buf, GLenum binding, size_t size, const void *data)
{
    glGenBuffers(1, buf);
    glBindBuffer(binding, *buf);
    glBufferData(binding, size, data, GL_STATIC_DRAW);
}

static void deleteBuffer(GLuint *buf)
{
    if (buf && *buf) {
        glDeleteBuffers(1, buf);
        *buf = 0;
    }
}

static void createTex(GLuint *tex, int w, int h, GLenum format, GLenum filter, void *data)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

static void deleteTex(GLuint *tex)
{
    if (tex && *tex) {
        glDeleteTextures(1, tex);
        *tex = 0;
    }
}

static void createFBTex(GLuint *tex, GLuint *fb, int w, int h, GLenum format, GLenum filter)
{
    createTex(tex, w, h, format, filter, 0);

    glGenFramebuffers(1, fb);
    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void deleteFBTex(GLuint *tex, GLuint *fb)
{
    deleteTex(tex);
    if (fb && *fb) {
        glDeleteFramebuffers(1, fb);
        *fb = 0;
    }
}

// Square wave generator as function of NES pal chroma index and phase.
#define IN_COLOR_PHASE(color_, phase_) (((color_) + (phase_)) % 12 < 6)

// This source code is modified from original at:
// http://wiki.nesdev.com/w/index.php/NTSC_video
// This version includes a fix to emphasis and applies normalization.
// Inputs:
//   pixel = Pixel color (9-bit) given as input. Bitmask format: "eeellcccc".
//   phase = Signal phase. It is a variable that increases by 8 each pixel.
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
    double low  = levels[0 + level];
    double high = levels[4 + level];
    if (color == 0) { low = high; } // For color 0, only high level is emitted
    if (color > 0x0C) { high = low; } // For colors $0D..$0F, only low level is emitted

    double signal = IN_COLOR_PHASE(color, phase) ? high : low;

    // When de-emphasis bits are set, some parts of the signal are attenuated:
    if ((color < 0x0E) && ( // Not for colors $0E..$0F (Wiki sample code doesn't have this)
        ((emphasis & 1) && IN_COLOR_PHASE(0, phase))
        || ((emphasis & 2) && IN_COLOR_PHASE(4, phase))
        || ((emphasis & 4) && IN_COLOR_PHASE(8, phase)))) {
        signal = signal * ATTENUATION;
    }

    // Normalize to desired black and white range. This is a linear operation.
    signal = ((signal-BLACK) / (WHITE-BLACK));

    return signal;
}

// 1D comb filter which relies on the inverse phase of the adjacent even and odd fields.
// Ideally this allows extracting accurate luma and chroma. With NES PPU however, phase of
// the fields is slightly off. More specifically, we'd need 6 samples phase difference
// (=half chroma wavelength), but we get 8 which comes from the discarded 1st pixel of
// the odd field. The error is 2 samples or 60 degrees. The error is fixed by adjusting
// the phase of the cos-sin (demodulator).
static void comb1D(double *result, double level0, double level1, double x)
{
    // Apply the 1D comb to separate luma and chroma.
    double y = (level0+level1) / 2.0;
    double c = (level0-level1) / 2.0;
    double a = (2.0*M_PI/12.0) * x;
    // Demodulate and store YIQ result.
    result[0] = y;
// TODO: have constants for these color scalers?
    result[1] = 1.414*c * cos(a);
    result[2] = 1.480*c * sin(a);
}

static void adjustYIQLimits(es2n *p, double *yiq)
{
    for (int i = 0; i < 3; i++) {
        p->yiq_mins[i] = fmin(p->yiq_mins[i], yiq[i]);
        p->yiq_maxs[i] = fmax(p->yiq_maxs[i], yiq[i]);
    }
}

// Box filter kernel.
#define BOX_FILTER(w2_, center_, x_) (abs((x_) - (center_)) < (w2_) ? 1.0 : 0.0)

// Generate lookup texture.
static void genLookupTex(es2n *p)
{
    double *ys = (double*) calloc(3*8 * NUM_PHASES*NUM_COLORS, sizeof(double));
	double *yiqs = (double*) calloc(3 * LOOKUP_W * NUM_COLORS, sizeof(double));
	unsigned char *result = (unsigned char*) calloc(3 * LOOKUP_W * NUM_COLORS, sizeof(unsigned char));

    // Generate temporary lookup containing samplings of separated and normalized YIQ components
    // for each phase and color combination. Separation is performed using a simulated 1D comb filter.
    int i = 0;
    for (int phase = 0; phase < NUM_PHASES; phase++) {
        // PPU color generator outputs 8 samples per pixel, and we have 3 different phases.
        const int phase0 = 8 * phase;
        // While even field is normal, PPU skips 1st pixel of the odd field, causing offset of 8 samples.
        const int phase1 = phase0 + 8;
        // Phase (hue) shift for demodulation. 180-33 degree shift from NTSC standard.
        const double shift = phase0 + (12.0/360.0) * (180.0-33.0);

        for (int color = 0; color < NUM_COLORS; color++) {
            // Here we store the eight (8) generated YIQ samples for the pixel.
            for (int s = 0; s < 8; s++) {
                // Obtain NTSC signal level from PPU color generator for both fields.
                double level0 = NTSCsignal(color, phase0+s);
                double level1 = NTSCsignal(color, phase1+s);
                comb1D(&ys[i], level0, level1, shift + s);
                i += 3;
            }
        }
    }

    // Generate an exhausting lookup texture for every color, phase, tap and subpixel combination.
    // ...Looks horrid, and yes it's complex, but computation is quite fast.
    for (int color = 0; color < NUM_COLORS; color++) {
        for (int phase = 0; phase < NUM_PHASES; phase++) {
            for (int subp = 0; subp < NUM_SUBPS; subp++) {
                for (int tap = 0; tap < NUM_TAPS; tap++) {
                    const int k = 3 * (color*LOOKUP_W + phase*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + tap);
                    double *yiq = &yiqs[k];

                    // Because of half subpixel accuracy (4 vs 8), filter twice and average.
                    for (int side = 0; side < 2; side++) { // 0:left, 1: right
                        // Calculate filter kernel center.
                        const double kernel_center = side + 0.5 + 2*subp + 8*(NUM_TAPS/2);

                        // Accumulate filter sum over all 8 samples of the pixel.
                        for (int p = 0; p < 8; p++) {
                            // Calculate x in kernel.
                            const double x = p + 8.0*tap;
                            // Filter luma and chroma with different filter widths.
                            double my = BOX_FILTER(YW2, kernel_center, x) / (2.0 * 8.0);
                            double mc = BOX_FILTER(CW2, kernel_center, x) / (2.0 * 8.0);
                            // Lookup YIQ signal level and accumulate.
                            i = 3 * (8*(color + phase*NUM_COLORS) + p);
                            yiq[0] += my * ys[i+0];
                            yiq[1] += mc * ys[i+1];
                            yiq[2] += mc * ys[i+2];
                        }
                    }

                    adjustYIQLimits(p, yiq);
                }
            }
        }
    }

    // Make RGB PPU palette similarly but having 12 samples per color.
    for (int color = 0; color < NUM_COLORS; color++) {
        // For some reason we need additional shift of 1 sample (-30 degrees).
        const double shift = (12.0/360.0) * (180.0-30.0-33.0);
        double yiq[3] = {0.0f, 0.0f, 0.0f};

        for (int s = 0; s < 12; s++) {
            double level0 = NTSCsignal(color, s) / 12.0;
            double level1 = NTSCsignal(color, s+6) / 12.0; // Perfect chroma cancellation.
            double t[3];
            comb1D(t, level0, level1, shift + s);
            yiq[0] += t[0];
            yiq[1] += t[1];
            yiq[2] += t[2];
        }

        adjustYIQLimits(p, yiq);

        const int k = 3 * (color*LOOKUP_W + LOOKUP_W-1);
        yiqs[k+0] = yiq[0];
        yiqs[k+1] = yiq[1];
        yiqs[k+2] = yiq[2];
    }

    // Create lookup texture RGB as bytes by mapping voltages to the min-max range.
    // The conversion to bytes will lose some precision, which is unnoticeable however.
    for (int k = 0; k < 3 * LOOKUP_W * NUM_COLORS; k+=3) {
        for (int i = 0; i < 3; i++) {
            const double v = (yiqs[k+i]-p->yiq_mins[i]) / (p->yiq_maxs[i]-p->yiq_mins[i]);
            result[k+i] = (unsigned char) (255.0*v + 0.5);
        }
    }

	glActiveTexture(TEX(LOOKUP_I));
    createTex(&p->lookup_tex, LOOKUP_W, NUM_COLORS, GL_RGB, GL_NEAREST, result);

	free(ys);
	free(yiqs);
	free(result);
}

static void updateControlUniformsRGB(const es2n_controls *c)
{
    GLfloat v;
    v = 0.333f * c->brightness;
    glUniform1f(c->_brightness_loc, v);
    v = 1.0f + 0.5f*c->contrast;
    glUniform1f(c->_contrast_loc, v);
    v = 1.0f + c->color;
    glUniform1f(c->_color_loc, v);
    v = 2.5f/2.2f + 0.5f*c->gamma;
    glUniform1f(c->_gamma_loc, v);
    v = c->rgbppu + 0.1f;
    glUniform1f(c->_rgbppu_loc, v);
}

static void updateControlUniformsStretch(const es2n_controls *c)
{
    GLfloat v;
    v = c->crt_enabled * (8.0f/255.0f);
    glUniform1f(c->_scanline_loc, v);
}

static void updateControlUniformsDisp(const es2n_controls *c)
{
    GLfloat v;
    v = c->crt_enabled * -3.5f * (c->convergence+0.2f);
    glUniform1f(c->_convergence_loc, v);
    v = 0.25f * (c->sharpness+0.4f);
    GLfloat sharpen_kernel[3 * 3] = {
        0.0f, -v, 0.0f, 
        0.0f, 1.0f+2.0f*v, 0.0f, 
        0.0f, -v, 0.0f 
    };
    glUniformMatrix3fv(c->_sharpen_kernel_loc, 1, GL_FALSE, sharpen_kernel);
}

static void initUniformsRGB(es2n *p)
{
    GLint k;
    GLuint prog = p->rgb_prog;

    k = glGetUniformLocation(prog, "u_idxTex");
    glUniform1i(k, IDX_I);
    k = glGetUniformLocation(prog, "u_deempTex");
    glUniform1i(k, DEEMP_I);
    k = glGetUniformLocation(prog, "u_lookupTex");
    glUniform1i(k, LOOKUP_I);
    k = glGetUniformLocation(prog, "u_mins");
    glUniform3fv(k, 1, p->yiq_mins);
    k = glGetUniformLocation(prog, "u_maxs");
    glUniform3fv(k, 1, p->yiq_maxs);

    es2n_controls *c = &p->controls;
    c->_brightness_loc = glGetUniformLocation(prog, "u_brightness");
    c->_contrast_loc = glGetUniformLocation(prog, "u_contrast");
    c->_color_loc = glGetUniformLocation(prog, "u_color");
    c->_gamma_loc = glGetUniformLocation(prog, "u_gamma");
    c->_rgbppu_loc = glGetUniformLocation(prog, "u_rgbppu");
    updateControlUniformsRGB(&p->controls);
}

static void initUniformsStretch(es2n *p)
{
    GLint k;
    GLuint prog = p->stretch_prog;

    k = glGetUniformLocation(prog, "u_rgbTex");
    glUniform1i(k, RGB_I);

    es2n_controls *c = &p->controls;
    c->_scanline_loc = glGetUniformLocation(prog, "u_scanline");
    updateControlUniformsStretch(&p->controls);
}

static void initUniformsDisp(es2n *p)
{
    GLint k;
    GLuint prog = p->disp_prog;

    k = glGetUniformLocation(prog, "u_stretchTex");
    glUniform1i(k, STRETCH_I);

    es2n_controls *c = &p->controls;
    c->_convergence_loc = glGetUniformLocation(prog, "u_convergence");
    c->_sharpen_kernel_loc = glGetUniformLocation(prog, "u_sharpenKernel");
    updateControlUniformsDisp(&p->controls);
}

static void renderRGB(es2n *p)
{
    glBindBuffer(GL_ARRAY_BUFFER, p->quad_buf);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 8, RGB_W, 224);
    glUseProgram(p->rgb_prog);
    updateControlUniformsRGB(&p->controls);

    glEnable(GL_BLEND);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisable(GL_BLEND);
}

static void renderStretch(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, p->stretch_fb);
// TODO: check defines
    glViewport(0, 4*8, RGB_W, 4*224);
    glUseProgram(p->stretch_prog);
    updateControlUniformsStretch(&p->controls);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void renderDisp(es2n *p)
{
    if (p->controls.crt_enabled) {
        glBindBuffer(GL_ARRAY_BUFFER, p->crt_verts_buf);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(p->viewport[0], p->viewport[1], p->viewport[2], p->viewport[3]);
    glUseProgram(p->disp_prog);
    updateControlUniformsDisp(&p->controls);
    if (p->controls.crt_enabled) {
        glDrawElements(GL_TRIANGLE_STRIP, 2 * (2*CRT_XN+1+1) * (2*CRT_YN), GL_UNSIGNED_SHORT, 0);
    } else {
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
//    printf("left:%d right:%d top:%d bottom:%d\n", left, right, top, bottom);
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
    createBuffer(&p->quad_buf, GL_ARRAY_BUFFER, sizeof(quadverts), quadverts);

    // Create CRT vertex buffer.
    const size_t size_verts = sizeof(GLfloat) * 4 * (2*CRT_XN+1) * (2*CRT_YN+1);
    GLfloat *crt_verts = (GLfloat*) malloc(size_verts);
    const double dx = 8.9;
    const double dy = 3.6;
    const double dx2 = dx*dx;
    const double dy2 = dy*dy;
    const double mx = sqrt(1.0 + dx2);
    const double my = sqrt(1.0 + dy2);
    for (int j = -CRT_YN; j <= CRT_YN; j++) {
        for (int i = -CRT_XN; i <= CRT_XN; i++) {
            int k = 4 * ((j+CRT_YN) * (2*CRT_XN+1) + i+CRT_XN);
            const double x = (double) i/CRT_XN;
            const double y = (double) j/CRT_YN;
            const double r2 = x*x + y*y;
            crt_verts[k+0] = mx * x / sqrt(dx2 + r2);
            crt_verts[k+1] = my * y / sqrt(dy2 + r2);
            crt_verts[k+2] = 0.5 + 0.5*x;
            crt_verts[k+3] = ((top-bottom) * (0.5 + 0.5*y) + bottom) / 256.0f;
        }
    }
    createBuffer(&p->crt_verts_buf, GL_ARRAY_BUFFER, size_verts, crt_verts);
    free(crt_verts);

    const size_t size_elems = sizeof(GLushort) * (2 * (2*CRT_XN+1+1) * (2*CRT_YN));
    GLushort *crt_elems = (GLushort*) malloc(size_elems);
    int k = 0;
    for (int j = 0; j < 2*CRT_YN; j++) {
        for (int i = 0; i < 2*CRT_XN+1; i++) {
            crt_elems[k+0] = i + (j+0) * (2*CRT_XN+1);
            crt_elems[k+1] = i + (j+1) * (2*CRT_XN+1);
            k += 2;
        }
        crt_elems[k+0] = crt_elems[k-1];
        crt_elems[k+1] = crt_elems[k-1 - 2*2*CRT_XN];
        k += 2;
    }
    createBuffer(&p->crt_elems_buf, GL_ELEMENT_ARRAY_BUFFER, size_elems, crt_elems);
    free(crt_elems);

    // Setup input pixels texture.
    glActiveTexture(TEX(IDX_I));
    createTex(&p->idx_tex, IDX_W, 256, GL_LUMINANCE, GL_NEAREST, 0);

    // Setup input de-emphasis rows texture.
    glActiveTexture(TEX(DEEMP_I));
    createTex(&p->deemp_tex, 256, 1, GL_LUMINANCE, GL_NEAREST, 0);

    genLookupTex(p);

    // Configure RGB framebuffer.
    glActiveTexture(TEX(RGB_I));
    createFBTex(&p->rgb_tex, &p->rgb_fb, RGB_W, 256, GL_RGB, GL_LINEAR);
    const char* rgb_vert_src =
        "precision highp float;\n"
        DEFINE(NUM_TAPS)
        DEFINE(IDX_W)
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[int(NUM_TAPS)];\n"
        "#define S vec2(1.0/IDX_W, 0.0)\n"
        "#define UV_OUT(i_, o_) v_uv[i_] = vert.zw + (o_)*S\n"
        "void main() {\n"
        "UV_OUT(0,-2.0);\n"
        "UV_OUT(1,-1.0);\n"
        "UV_OUT(2, 0.0);\n"
        "UV_OUT(3, 1.0);\n"
        "UV_OUT(4, 2.0);\n"
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
        "uniform sampler2D u_idxTex;\n"
        "uniform sampler2D u_deempTex;\n"
        "uniform sampler2D u_lookupTex;\n"
        "uniform vec3 u_mins;\n"
        "uniform vec3 u_maxs;\n"
        "uniform float u_brightness;\n"
        "uniform float u_contrast;\n"
        "uniform float u_color;\n"
        "uniform float u_gamma;\n"
        "uniform float u_rgbppu;\n"
        "varying vec2 v_uv[int(NUM_TAPS)];\n"
        "const mat3 c_convMat = mat3(\n"
        "    1.0,        1.0,        1.0,       // Y\n"
        "    0.946882,   -0.274788,  -1.108545, // I\n"
        "    0.623557,   -0.635691,  1.709007   // Q\n"
        ");\n"
        "\n"
        "#define P(i_)  p = floor(IDX_W*v_uv[i_])\n"
        "#define U(i_)  (mod(p.x - p.y, 3.0)*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + float(i_)) / (LOOKUP_W-1.0)\n"
        "#define LA(i_) la = vec2(texture2D(u_idxTex, v_uv[i_]).r, texture2D(u_deempTex, vec2(v_uv[i_].y, 0.0)).r)\n"
        "#define V()    dot((255.0/511.0) * vec2(1.0, 64.0), la)\n"
        "#define UV(i_) uv = vec2(U(i_), V())\n"
        "#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)\n"
        "#define SMP(i_) LA(i_); P(i_); UV(i_); yiq += RESCALE(texture2D(u_lookupTex, uv).rgb)\n"
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
        // Snatch in RGB PPU; uv.x is already calculated, so just read from lookup tex with u=1.0.
        "vec3 rgbppu = RESCALE(texture2D(u_lookupTex, vec2(1.0, uv.y)).rgb);\n"
        "SMP(3);\n"
        "SMP(4);\n"
// TODO: Working multiplier for filtered chroma to match PPU is 2/5 (for CW2=12). Reason unknown?
        "yiq *= (8.0/2.0) / vec3(YW2, CW2-2.0, CW2-2.0);\n"
        "yiq = mix(yiq, rgbppu, u_rgbppu);\n"
        "yiq.gb *= u_color;\n"
        "vec3 result = c_convMat * yiq;\n"
        "gl_FragColor = vec4(u_contrast * pow(result, vec3(u_gamma)) + u_brightness, 1.0);\n"
        "}\n";
    p->rgb_prog = buildShader(rgb_vert_src, rgb_frag_src);
    initUniformsRGB(p);

    // Setup stretch framebuffer.
    glActiveTexture(TEX(STRETCH_I));
    createFBTex(&p->stretch_tex, &p->stretch_fb, RGB_W, STRETCH_H, GL_RGB, GL_LINEAR);
    const char* stretch_vert_src =
        "precision highp float;\n"
        DEFINE(RGB_W)
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[1];\n"
        "#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0)\n"
        "void main() {\n"
        "vec2 uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "TAP(0, 0.0);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* stretch_frag_src =
        "precision highp float;\n"
        "uniform float u_scanline;\n"
        "uniform sampler2D u_rgbTex;\n"
        // Filter nearest on y, linear on x. This is a slower dependent texture read.
        "#define UV(i_) uv = vec2(v_uv[i_].x, floor(256.0*v_uv[i_].y) / 256.0 + 0.5/256.0)\n"
        "#define SMP(i_, m_) UV(i_); color += (m_) * texture2D(u_rgbTex, uv).rgb\n"
        "varying vec2 v_uv[1];\n"
        "void main(void) {\n"
        "vec2 uv;\n"
        "vec3 color = vec3(0.0);\n"
        "SMP(0, 1.0);\n"
        "float scanline = u_scanline * (1.0 - min(mod(floor(4.0*256.0*v_uv[0].y), 4.0), 1.0));\n"
        "gl_FragColor = vec4((color - scanline) * (1.0+scanline), 1.0);\n"
        "}\n";
    p->stretch_prog = buildShader(stretch_vert_src, stretch_frag_src);
    initUniformsStretch(p);

    // Setup display (output) shader.
    const char* disp_vert_src =
        "precision highp float;\n"
        DEFINE(RGB_W)
        "attribute vec4 vert;\n"
        "uniform float u_convergence;\n"
        "varying vec2 v_uv[5];\n"
        "#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0)\n"
        "void main() {\n"
        "vec2 uv = vec2(vert.z, (240.0/256.0) - vert.w);\n"
        "TAP(0,-5.5);\n"
        "TAP(1,-u_convergence);\n"
        "TAP(2, 0.0);\n"
        "TAP(3, u_convergence);\n"
        "TAP(4, 5.5);\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* disp_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_stretchTex;\n"
        "uniform mat3 u_sharpenKernel;\n"
        "varying vec2 v_uv[5];\n"
        "void main(void) {\n"
#if 1
        "#define SMP(i_, m_) color += (m_) * texture2D(u_stretchTex, v_uv[i_]).rgb\n"
        "vec3 color = vec3(0.0);\n"
        "SMP(0, u_sharpenKernel[0]);\n"
        "SMP(1, vec3(1.0, 0.0, 0.0));\n"
        "SMP(2, u_sharpenKernel[1]);\n"
        "SMP(3, vec3(0.0, 0.0, 1.0));\n"
        "SMP(4, u_sharpenKernel[2]);\n"
        "gl_FragColor = vec4(color, 1.0);\n"
#else
        "gl_FragColor = texture2D(u_stretchTex, v_uv[2]);\n"
#endif
        "}\n";
    p->disp_prog = buildShader(disp_vert_src, disp_frag_src);
    initUniformsDisp(p);
}

void es2nDeinit(es2n *p)
{
    deleteFBTex(&p->rgb_tex, &p->rgb_fb);
    deleteFBTex(&p->stretch_tex, &p->stretch_fb);
    deleteTex(&p->idx_tex);
    deleteTex(&p->deemp_tex);
    deleteTex(&p->lookup_tex);
    deleteShader(&p->rgb_prog);
    deleteShader(&p->stretch_prog);
    deleteShader(&p->disp_prog);
    deleteBuffer(&p->quad_buf);
    deleteBuffer(&p->crt_verts_buf);
    deleteBuffer(&p->crt_elems_buf);
    free(p->overscan_pixels);
}

void es2nRender(es2n *p, GLubyte *pixels, GLubyte *row_deemp, GLubyte overscan_color)
{
    // Update input pixels.
    glActiveTexture(TEX(IDX_I));
    glBindTexture(GL_TEXTURE_2D, p->idx_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, OVERSCAN_W, 0, 256, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    if (p->overscan_color != overscan_color) {
        p->overscan_color = overscan_color;
//        printf("overscan: %02X\n", overscan_color);

        memset(p->overscan_pixels, overscan_color, OVERSCAN_W*240);
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, OVERSCAN_W, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
        glTexSubImage2D(GL_TEXTURE_2D, 0, IDX_W-OVERSCAN_W, 0, OVERSCAN_W, 240, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
    }

    // Update input de-emphasis rows.
    glActiveTexture(TEX(DEEMP_I));
    glBindTexture(GL_TEXTURE_2D, p->deemp_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 240, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, row_deemp);

    renderRGB(p);
    renderStretch(p);
    renderDisp(p);
}

