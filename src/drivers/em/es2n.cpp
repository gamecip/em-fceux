#include "es2n.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
// TODO: remove, not needed
#include <stdio.h>
#include <emscripten.h>
#include "es2utils.h"
#include "meshes.h"

#define STR(s_) _STR(s_)
#define _STR(s_) #s_
#define ARRAY_SIZE(a_) (sizeof(a_) / sizeof(*(a_)))

// TODO: tsone: set elsewhere?
#define DBG_MODE 1
#if DBG_MODE
#define DBG(x_) x_;
#else
#define DBG(x_)
#endif

#define IDX_I       0
#define DEEMP_I     1
#define LOOKUP_I    2
#define RGB_I       3
#define STRETCH_I   4
// NOTE: tsone: tv and downsample texture must be in incremental order
#define TV_I        5
#define DOWNSAMPLE0_I 6
#define DOWNSAMPLE1_I 7
#define DOWNSAMPLE2_I 8
#define DOWNSAMPLE3_I 9
#define DOWNSAMPLE4_I 10
#define DOWNSAMPLE5_I 11
#define NOISE_I     12
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
// Lookup width must be POT >= NUM_PHASES*NUM_TAPS*NUM_SUBPS, ex. 3*5*4=60 -> 64
#define LOOKUP_W 64
// Set overscan on left and right sides as 12px (total 24px).
#define OVERSCAN_W 12
#define IDX_W (256 + 2*OVERSCAN_W)
#define IDX_H 240
#define NOISE_W 256 
#define NOISE_H 256 
#define RGB_W (NUM_SUBPS * IDX_W)
#define STRETCH_H (4 * 240)
#define SCREEN_H (4 * 224)
// Half-width of Y and C box filter kernels.
#define YW2 6.0
#define CW2 12.0

// Square wave generator as function of NES pal chroma index and phase.
#define IN_COLOR_PHASE(color_, phase_) (((color_) + (phase_)) % 12 < 6)

#include "shaders.h"

static const GLint mesh_quad_vert_num = 4;
static const GLint mesh_quad_face_num = 2;
static const GLfloat mesh_quad_verts[] = {
    -1.0f, -1.0f, 0.0f,
     1.0f, -1.0f, 0.0f,
    -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f, 0.0f
};
static const GLfloat mesh_quad_uvs[] = {
     0.0f,  0.0f,
     1.0f,  0.0f,
     0.0f,  1.0f,
     1.0f,  1.0f 
};
static const GLfloat mesh_quad_norms[] = {
     0.0f,  0.0f, 1.0f,
     0.0f,  0.0f, 1.0f,
     0.0f,  0.0f, 1.0f,
     0.0f,  0.0f, 1.0f
};
static es2_varray mesh_quad_varrays[] = {
    { 3, GL_FLOAT, 0, (const void*) mesh_quad_verts },
    { 3, GL_FLOAT, 0, (const void*) mesh_quad_norms },
    { 2, GL_FLOAT, 0, (const void*) mesh_quad_uvs }
};

static es2_varray mesh_screen_varrays[] = {
    { 3, GL_FLOAT, 0, (const void*) mesh_screen_verts },
    { 3, GL_FLOAT, VARRAY_ENCODED_NORMALS, (const void*) mesh_screen_norms },
    { 2, GL_FLOAT, 0, (const void*) mesh_screen_uvs }
};

static es2_varray mesh_rim_varrays[] = {
    { 3, GL_FLOAT, 0, (const void*) mesh_rim_verts },
    { 3, GL_FLOAT, VARRAY_ENCODED_NORMALS, (const void*) mesh_rim_norms },
    { 3, GL_FLOAT, 0, 0 },
//    { 3, GL_FLOAT, (const void*) mesh_rim_vcols }
    { 3, GL_UNSIGNED_BYTE, 0, (const void*) mesh_rim_vcols }
};

static const GLfloat mat4_identity[] = { 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 };

// Texture sample offsets and weights for gaussian w/ radius=8, sigma=4.
// Eliminates aliasing and blurs for "faked" glossy reflections and AO.
static const GLfloat s_downsample_offs[] = { -6.892337f, -4.922505f, -2.953262f, -0.98438f, 0.98438f, 2.953262f, 4.922505f, 6.892337f };
static const GLfloat s_downsample_ws[] = { 0.045894f, 0.096038f, 0.157115f, 0.200954f, 0.200954f, 0.157115f, 0.096038f, 0.045894f };

static const int s_downsample_widths[]  = { 1120, 280, 280,  70, 70, 18, 18 };
static const int s_downsample_heights[] = {  960, 960, 240, 240, 60, 60, 15 };

// This source code is modified from original at:
// http://wiki.nesdev.com/w/index.php/NTSC_video
// This version includes a fix to emphasis and applies normalization.
// Inputs:
//   pixel = Pixel color (9-bit) given as input. Bitmask format: "eeellcccc".
//   phase = Signal phase. It is a variable that increases by 8 each pixel.
static double NTSCsignal(int pixel, int phase)
{
    // Voltage levels, relative to synch voltage
    const GLfloat levels[8] = {
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
// the fields is slightly off. More specifically, we'd need phase difference of 6 samples
// (=half chroma wavelength), but we get 8 which comes from the discarded 1st pixel of
// the odd field. The error is 2 samples or 60 degrees. The error is fixed by adjusting
// the phase of the cos-sin (demodulator, not done in this function...).
static void comb1D(double *result, double level0, double level1, double x)
{
    // Apply the 1D comb to separate luma and chroma.
    double y = (level0+level1) / 2.0;
    double c = (level0-level1) / 2.0;
    double a = (2.0*M_PI/12.0) * x;
    // Demodulate and store YIQ result.
    result[0] = y;
// TODO: tsone: battling with scalers... weird
    // This scaler (8/6) was found with experimentation. Something with inadequate sampling?
//    result[1] = 1.333*c * cos(a);
//    result[2] = 1.333*c * sin(a);
// TODO: tsone: old scalers? seem to over-saturate. no idea where these from
    result[1] = 1.414*c * cos(a);
    result[2] = 1.414*c * sin(a);
//    result[2] = 1.480*c * sin(a);
}

static void adjustYIQLimits(es2n *p, double *yiq)
{
    for (int i = 0; i < 3; i++) {
        p->yiq_mins[i] = fmin(p->yiq_mins[i], yiq[i]);
        p->yiq_maxs[i] = fmax(p->yiq_maxs[i], yiq[i]);
    }
}

// Box filter kernel.
#define BOX_FILTER(w2_, center_, x_) (fabs((x_) - (center_)) < (w2_) ? 1.0 : 0.0)

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
                        const double kernel_center = (side + 2*subp + 8*(NUM_TAPS/2)) + 0.5;

                        // Accumulate filter sum over all 8 samples of the pixel.
                        for (int s = 0; s < 8; s++) {
                            // Calculate x in kernel.
                            const double x = s + 8.0*tap;
                            // Filter luma and chroma with different filter widths.
                            double my = BOX_FILTER(YW2, kernel_center, x) / (2.0*8.0);
                            double mc = BOX_FILTER(CW2, kernel_center, x) / (2.0*8.0);
                            // Lookup YIQ signal level and accumulate.
                            i = 3 * (8*(color + phase*NUM_COLORS) + s);
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
        double yiq[3] = {0, 0, 0};

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
    createTex(&p->lookup_tex, LOOKUP_W, NUM_COLORS, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE, result);

	free(ys);
	free(yiqs);
	free(result);
}

// Get uniformly distributed random number in [0,1] range.
static double rand01()
{
//    return (double) rand() / (double) RAND_MAX;
    return emscripten_random();
}

static void genNoiseTex(es2n *p)
{
    GLubyte *noise = (GLubyte*) malloc(NOISE_W*NOISE_H);

#if 0
    for (int j = 0; j < NOISE_H; j++) {
        for (int i = 0; i < NOISE_W; i++) {
            if ((i % 2) == 0) noise[NOISE_W*j+i] = 255;
            else noise[NOISE_W*j+i] = 0;
        }
    }
#else
    // Box-Muller method gaussian noise.
    // Results are clamped to 0..255 range, which skews the distribution slightly.
    const double SIGMA = 0.5/2.0; // Set 95% of noise values in [-0.5,0.5] range.
    const double MU = 0.5; // Offset range by +0.5 to map to [0,1].
    for (int i = 0; i < NOISE_W*NOISE_H; i++) {
        double x;
        do {
		x = rand01();
	} while (x < 1e-7); // Epsilon to avoid log(0).
        double r = SIGMA * sqrt(-2.0 * log10(x));
        r = MU + r*sin(2.0*M_PI * rand01()); // Take real part only, discard complex part as it's related.
        // Clamp result to [0,1].
        noise[i] = (r <= 0) ? 0 : (r >= 1) ? 255 : (int) (0.5 + 255.0*r);
    }
#endif

    glActiveTexture(TEX(NOISE_I));
    createTex(&p->noise_tex, NOISE_W, NOISE_H, GL_LUMINANCE, GL_LINEAR, GL_REPEAT, noise);
//    createTex(&p->noise_tex, NOISE_W, NOISE_H, GL_LUMINANCE, GL_LINEAR, GL_CLAMP_TO_EDGE, noise);

    free(noise);
}

#if DBG_MODE
extern int MouseData[3];
static void updateUniformsDebug()
{
    GLint prog = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
    int k = glGetUniformLocation(prog, "u_mouse");
    GLfloat mouse[3] = { MouseData[0], MouseData[1], MouseData[2] };
    glUniform3fv(k, 1, mouse);
}
#endif

static void updateUniformsRGB(const es2n_controls *c)
{
    DBG(updateUniformsDebug())
    double v;
    v = 0.15 * c->brightness;
    glUniform1f(c->_brightness_loc, v);
    v = 1.0 + 0.4*c->contrast;
    glUniform1f(c->_contrast_loc, v);
    v = 1.0 + c->color;
    glUniform1f(c->_color_loc, v);
    v = c->rgbppu + 0.1;
    glUniform1f(c->_rgbppu_loc, v);
    v = 2.4/2.2 + 0.3*c->gamma;
    glUniform1f(c->_gamma_loc, v);
    v = c->crt_enabled * 0.13 * c->noise;
    glUniform1f(c->_noiseAmp_loc, v);
    glUniform2f(c->_noiseRnd_loc, rand01(), rand01());
}

static void updateUniformsStretch(const es2n_controls *c)
{
    DBG(updateUniformsDebug())
    double v;
    v = c->crt_enabled * 0.5 * c->scanlines;
    glUniform1f(c->_scanlines_loc, v);
}

static void updateUniformsScreen(const es2n *p, int final_pass)
{
    DBG(updateUniformsDebug())
    const es2n_controls *c = &p->controls;
    double v;
    v = c->crt_enabled * 2.0 * c->convergence;
    glUniform1f(c->_convergence_loc, v);

    if (c->crt_enabled) {
        glUniform2f(c->_screen_uvScale_loc, 278.0/IDX_W, 228.0/IDX_H);
        glUniformMatrix4fv(c->_screen_mvp_loc, 1, GL_FALSE, p->mvp_mat);
    } else {
        glUniform2f(c->_screen_uvScale_loc, 260.0/IDX_W, 224.0/IDX_H);
        glUniformMatrix4fv(c->_screen_mvp_loc, 1, GL_FALSE, mat4_identity);
    }

    v = (0.9-c->rgbppu) * 0.4 * (c->sharpness+0.5);
    GLfloat sharpen_kernel[] = {
        -v, -v, -v,
        1, 0, 0, 
        2*v, 1+2*v, 2*v, 
        0, 0, 1, 
        -v, -v, -v
    };

    glUniform3fv(c->_sharpen_kernel_loc, 5, sharpen_kernel);
}

static void updateUniformsDownsample(const es2n *p, int w, int h, int texIdx, int isHorzPass)
{
    DBG(updateUniformsDebug())
    GLfloat offsets[2*8];
    if (isHorzPass) {
        for (int i = 0; i < 8; ++i) {
            offsets[2*i  ] = s_downsample_offs[i] / w;
            offsets[2*i+1] = 0;
        }
    } else {
        for (int i = 0; i < 8; ++i) {
            offsets[2*i  ] = 0;
            offsets[2*i+1] = s_downsample_offs[i] / h;
        }
    }
    glUniform2fv(p->_downsample_offsets_loc, 8, offsets);
    glUniform1fv(p->_downsample_weights_loc, 8, s_downsample_ws);
    glUniform1i(p->_downsample_downsampleTex_loc, texIdx);
}

// TODO: tsone: not necessary?
static void updateUniformsTV(const es2n *p)
{
    DBG(updateUniformsDebug())
}

static void updateUniformsCombine(const es2n *p)
{
    DBG(updateUniformsDebug())
    const es2n_controls *c = &p->controls;
    double v = 0.1 * c->glow;
    glUniform3f(c->_combine_glow_loc, v, v*v, v + v*v);
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
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);
    k = glGetUniformLocation(prog, "u_mins");
    glUniform3fv(k, 1, p->yiq_mins);
    k = glGetUniformLocation(prog, "u_maxs");
    glUniform3fv(k, 1, p->yiq_maxs);

    es2n_controls *c = &p->controls;
    c->_brightness_loc = glGetUniformLocation(prog, "u_brightness");
    c->_contrast_loc = glGetUniformLocation(prog, "u_contrast");
    c->_color_loc = glGetUniformLocation(prog, "u_color");
    c->_rgbppu_loc = glGetUniformLocation(prog, "u_rgbppu");
    c->_gamma_loc = glGetUniformLocation(prog, "u_gamma");
    c->_noiseAmp_loc = glGetUniformLocation(prog, "u_noiseAmp");
    c->_noiseRnd_loc = glGetUniformLocation(prog, "u_noiseRnd");
    updateUniformsRGB(&p->controls);
}

static void initUniformsStretch(es2n *p)
{
    GLint k;
    GLuint prog = p->stretch_prog;

    k = glGetUniformLocation(prog, "u_rgbTex");
    glUniform1i(k, RGB_I);

    es2n_controls *c = &p->controls;
    c->_scanlines_loc = glGetUniformLocation(prog, "u_scanlines");
    updateUniformsStretch(&p->controls);
}

// Using python oneliners:
// from math import *
// def rot(a,b): return [sin(a)*sin(b), -sin(a)*cos(b), -cos(a)]
// def rad(d): return pi*d/180
// rot(rad(90-65),rad(15))
//static GLfloat s_lightDir[] = { -0.109381654946615, 0.40821789367673483, 0.9063077870366499 }; // 90-65, 15
//static GLfloat s_lightDir[] = { -0.1830127018922193, 0.6830127018922193, 0.7071067811865476 }; // 90-45, 15
//static GLfloat s_lightDir[] = { -0.22414386804201336, 0.8365163037378078, 0.5000000000000001 }; // 90-30, 15
static GLfloat s_lightDir[] = { 0.0, 0.866025, 0.5 }; // 90-30, 0
static GLfloat s_viewPos[] = { 0, 0, 2.5 };

static void initShading(GLuint prog, float intensity, float diff, float fill, float spec, float m, float fr0, float frexp)
{
    int k = glGetUniformLocation(prog, "u_lightDir");
    glUniform3fv(k, 1, s_lightDir);
    k = glGetUniformLocation(prog, "u_viewPos");
    glUniform3fv(k, 1, s_viewPos);
    k = glGetUniformLocation(prog, "u_material");
    glUniform4f(k, intensity*diff / M_PI, intensity*spec * (m+8.0) / (8.0*M_PI), m, intensity*fill / M_PI);
    k = glGetUniformLocation(prog, "u_fresnel");
    glUniform3f(k, fr0, 1-fr0, frexp);
}

static void initUniformsScreen(es2n *p)
{
    GLint k;
    GLuint prog = p->screen_prog;

    k = glGetUniformLocation(prog, "u_stretchTex");
    glUniform1i(k, STRETCH_I);
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);

    initShading(prog, 1.5, 0.001, 0.0, 0.065, 41, 0.04, 4);

    es2n_controls *c = &p->controls;
    c->_convergence_loc = glGetUniformLocation(prog, "u_convergence");
    c->_sharpen_kernel_loc = glGetUniformLocation(prog, "u_sharpenKernel");
    c->_screen_uvScale_loc = glGetUniformLocation(prog, "u_uvScale");
    c->_screen_mvp_loc = glGetUniformLocation(prog, "u_mvp");
    updateUniformsScreen(p, 1);
}

static void initUniformsTV(es2n *p)
{
    GLint k;
    GLuint prog = p->tv_prog;

    k = glGetUniformLocation(prog, "u_downsample1Tex");
    glUniform1i(k, DOWNSAMPLE1_I);
    k = glGetUniformLocation(prog, "u_downsample3Tex");
    glUniform1i(k, DOWNSAMPLE3_I);
    k = glGetUniformLocation(prog, "u_downsample5Tex");
    glUniform1i(k, DOWNSAMPLE5_I);
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);

    k = glGetUniformLocation(prog, "u_mvp");
    glUniformMatrix4fv(k, 1, GL_FALSE, p->mvp_mat);

    initShading(prog, 1.5, 0.004, 0.004, 0.039, 49, 0.03, 4);

    updateUniformsTV(p);
}

static void initUniformsDownsample(es2n *p)
{
    GLuint prog = p->downsample_prog;

    p->_downsample_offsets_loc = glGetUniformLocation(prog, "u_offsets");
    p->_downsample_weights_loc = glGetUniformLocation(prog, "u_weights");
    p->_downsample_downsampleTex_loc = glGetUniformLocation(prog, "u_downsampleTex");
    updateUniformsDownsample(p, 280, 240, DOWNSAMPLE0_I, 1);
}

static void initUniformsCombine(es2n *p)
{
    GLint k;
    GLuint prog = p->combine_prog;

    k = glGetUniformLocation(prog, "u_tvTex");
    glUniform1i(k, TV_I);
    k = glGetUniformLocation(prog, "u_downsample3Tex");
    glUniform1i(k, DOWNSAMPLE3_I);
    k = glGetUniformLocation(prog, "u_downsample5Tex");
    glUniform1i(k, DOWNSAMPLE5_I);
    k = glGetUniformLocation(prog, "u_noiseTex");
    glUniform1i(k, NOISE_I);

    es2n_controls *c = &p->controls;
    c->_combine_glow_loc = glGetUniformLocation(prog, "u_glow");
}

static void passRGB(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 0, RGB_W, IDX_H);
    glUseProgram(p->rgb_prog);
    updateUniformsRGB(&p->controls);

    if (p->controls.crt_enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR);
    }
    meshRender(&p->quad_mesh);
    glDisable(GL_BLEND);
}

static void passStretch(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, p->stretch_fb);
    glViewport(0, 0, RGB_W, STRETCH_H);
    glUseProgram(p->stretch_prog);
    updateUniformsStretch(&p->controls);
    meshRender(&p->quad_mesh);
}

static void passDownsample(es2n *p)
{
    glUseProgram(p->downsample_prog);

    for (int i = 0; i < 6; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, p->downsample_fb[i]);
        glViewport(0, 0, s_downsample_widths[i+1], s_downsample_heights[i+1]);
        updateUniformsDownsample(p, s_downsample_widths[i], s_downsample_heights[i], TV_I + i, !(i & 1));
        meshRender(&p->quad_mesh);
    }
}

static void passScreen(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, p->tv_fb);
    glViewport(0, 0, RGB_W, SCREEN_H);
    glUseProgram(p->screen_prog);
    updateUniformsScreen(p, 1);

    if (p->controls.crt_enabled) {
        meshRender(&p->screen_mesh);
    } else {
        meshRender(&p->quad_mesh);
    }
}

static void passTV(es2n *p)
{
    if (!p->controls.crt_enabled) {
        return;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, p->tv_fb);
//    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, RGB_W, SCREEN_H);
//    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(p->tv_prog);
    updateUniformsTV(p);
    meshRender(&p->tv_mesh);
}

static void passCombine(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(p->viewport[0], p->viewport[1], p->viewport[2], p->viewport[3]);
    glUseProgram(p->combine_prog);
    updateUniformsCombine(p);
//    glEnable(GL_BLEND);
//    glBlendFunc(GL_ONE_MINUS_DST_ALPHA, GL_ONE);
    meshRender(&p->quad_mesh);
//    glDisable(GL_BLEND);
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
//    printf("left:%d right:%d top:%d bottom:%d\n", left, right, top, bottom);
    memset(p, 0, sizeof(es2n));

    // Build perspective MVP matrix.
    GLfloat trans[3] = { 0, 0, -2.5 };
    GLfloat proj[4*4];
    GLfloat view[4*4];
    mat4Persp(proj, 0.25*M_PI*224.0/280.0, 280.0/224.0, 0.125, 16.0);
    mat4Trans(view, trans);
    mat4Mul(p->mvp_mat, proj, view);

    p->overscan_pixels = (GLubyte*) malloc(OVERSCAN_W*240);
    p->overscan_color = 0xFE; // Set bogus value to ensure overscan update.

    glGetIntegerv(GL_VIEWPORT, p->viewport);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendColor(PERSISTENCE_R, PERSISTENCE_G, PERSISTENCE_B, 0);
    glClearColor(0, 0, 0, 0);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_FALSE);
    glStencilMask(GL_FALSE);
    glEnableVertexAttribArray(0);

    createMesh(&p->quad_mesh, mesh_quad_vert_num, ARRAY_SIZE(mesh_quad_varrays), mesh_quad_varrays, 2*mesh_quad_face_num, 0);

    // Setup input pixels texture.
    glActiveTexture(TEX(IDX_I));
    createTex(&p->idx_tex, IDX_W, IDX_H, GL_LUMINANCE, GL_NEAREST, GL_CLAMP_TO_EDGE, 0);

    // Setup input de-emphasis rows texture.
    glActiveTexture(TEX(DEEMP_I));
    createTex(&p->deemp_tex, IDX_H, 1, GL_LUMINANCE, GL_NEAREST, GL_CLAMP_TO_EDGE, 0);

    genLookupTex(p);
    genNoiseTex(p);

    // Configure RGB framebuffer.
    glActiveTexture(TEX(RGB_I));
    createFBTex(&p->rgb_tex, &p->rgb_fb, RGB_W, IDX_H, GL_RGB, GL_NEAREST, GL_CLAMP_TO_EDGE);
    p->rgb_prog = buildShader(rgb_vert_src, rgb_frag_src, common_src);
    initUniformsRGB(p);

    // Setup stretch framebuffer.
    glActiveTexture(TEX(STRETCH_I));
    createFBTex(&p->stretch_tex, &p->stretch_fb, RGB_W, STRETCH_H, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
    p->stretch_prog = buildShader(stretch_vert_src, stretch_frag_src, common_src);
    initUniformsStretch(p);

    // Setup screen/TV framebuffer.
    glActiveTexture(TEX(TV_I));
    createFBTex(&p->tv_tex, &p->tv_fb, RGB_W, SCREEN_H, GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);

    // Setup downsample framebuffers.
    for (int i = 0; i < 6; ++i) {
      glActiveTexture(TEX(DOWNSAMPLE0_I + i));
      createFBTex(&p->downsample_tex[i], &p->downsample_fb[i], s_downsample_widths[i+1], s_downsample_heights[i+1], GL_RGB, GL_LINEAR, GL_CLAMP_TO_EDGE);
    }

    // Setup downsample shader.
    p->downsample_prog = buildShader(downsample_vert_src, downsample_frag_src, common_src);
    initUniformsDownsample(p);

    // Setup screen shader.
    p->screen_prog = buildShader(screen_vert_src, screen_frag_src, common_src);
    createMesh(&p->screen_mesh, mesh_screen_vert_num, ARRAY_SIZE(mesh_screen_varrays), mesh_screen_varrays, 3*mesh_screen_face_num, mesh_screen_faces);
    initUniformsScreen(p);

    // Setup TV shader.
    p->tv_prog = buildShader(tv_vert_src, tv_frag_src, common_src);
// TODO: tsone: generate distances to crt screen edges
    int num_edges = 0;
    int *edges = createUniqueEdges(&num_edges, mesh_screen_vert_num, 3*mesh_screen_face_num, mesh_screen_faces);
    num_edges *= 2;

    GLfloat *rim_extra = (GLfloat*) malloc(3*sizeof(GLfloat) * mesh_rim_vert_num);
    for (int i = 0; i < mesh_rim_vert_num; ++i) {
        GLfloat p[3];
        vec3Set(p, &mesh_rim_verts[3*i]);
        GLfloat shortest[3] = { 0, 0, 0 };
        double shortestDist = 1000000;
        for (int j = 0; j < num_edges; j += 2) {
            int ai = 3*edges[j];
            int bi = 3*edges[j+1];
            GLfloat diff[3];
            vec3ClosestOnSegment(diff, p, &mesh_screen_verts[ai], &mesh_screen_verts[bi]);
            vec3Sub(diff, diff, p);
            double dist = vec3Length2(diff);
            if (dist < shortestDist) {
                shortestDist = dist;
                vec3Set(shortest, diff);
            }
        }
// TODO: tsone: could interpolate uv with vert normal here, and not in vertex shader
        rim_extra[3*i] = shortest[0];
        rim_extra[3*i+1] = shortest[1];
        rim_extra[3*i+2] = shortest[2];
    }
    mesh_rim_varrays[2].data = rim_extra;
    createMesh(&p->tv_mesh, mesh_rim_vert_num, ARRAY_SIZE(mesh_rim_varrays), mesh_rim_varrays, 3*mesh_rim_face_num, mesh_rim_faces);
    free(edges);
    free(rim_extra);
    initUniformsTV(p);

    // Setup combine shader.
    p->combine_prog = buildShader(combine_vert_src, combine_frag_src, common_src);
    initUniformsCombine(p);
}

void es2nDeinit(es2n *p)
{
    deleteFBTex(&p->rgb_tex, &p->rgb_fb);
    deleteFBTex(&p->stretch_tex, &p->stretch_fb);
    deleteFBTex(&p->tv_tex, &p->tv_fb);
    for (int i = 0; i < 6; ++i) {
        deleteFBTex(&p->downsample_tex[i], &p->downsample_fb[i]);
    }
    deleteTex(&p->idx_tex);
    deleteTex(&p->deemp_tex);
    deleteTex(&p->lookup_tex);
    deleteTex(&p->noise_tex);
    deleteShader(&p->rgb_prog);
    deleteShader(&p->stretch_prog);
    deleteShader(&p->screen_prog);
    deleteShader(&p->downsample_prog);
    deleteShader(&p->tv_prog);
    deleteShader(&p->combine_prog);
    deleteMesh(&p->screen_mesh);
    deleteMesh(&p->quad_mesh);
    deleteMesh(&p->tv_mesh);
    free(p->overscan_pixels);
}

void es2nRender(es2n *p, GLubyte *pixels, GLubyte *row_deemp, GLubyte overscan_color)
{
    // Update input pixels.
    glActiveTexture(TEX(IDX_I));
    glBindTexture(GL_TEXTURE_2D, p->idx_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, OVERSCAN_W, 0, IDX_W-2*OVERSCAN_W, IDX_H, GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);
    if (p->overscan_color != overscan_color) {
        p->overscan_color = overscan_color;
//        printf("overscan: %02X\n", overscan_color);

        memset(p->overscan_pixels, overscan_color, OVERSCAN_W * IDX_H);
        
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, OVERSCAN_W, IDX_H, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
        glTexSubImage2D(GL_TEXTURE_2D, 0, IDX_W-OVERSCAN_W, 0, OVERSCAN_W, IDX_H, GL_LUMINANCE, GL_UNSIGNED_BYTE, p->overscan_pixels);
    }

    // Update input de-emphasis rows.
    glActiveTexture(TEX(DEEMP_I));
    glBindTexture(GL_TEXTURE_2D, p->deemp_tex);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, IDX_H, 1, GL_LUMINANCE, GL_UNSIGNED_BYTE, row_deemp);

    passRGB(p);
    passStretch(p);
    passScreen(p);
    passDownsample(p);
    passTV(p);
    passCombine(p);
}

