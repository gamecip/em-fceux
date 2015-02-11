#include "es2n.h"
#include <cstdlib>
#include <cstring>
#include <cmath>
// TODO: remove, not needed
#include <stdio.h>
#include "es2utils.h"
#include "meshes.h"

#define STR(s_) _STR(s_)
#define _STR(s_) #s_

#define IDX_I       0
#define DEEMP_I     1
#define LOOKUP_I    2
#define RGB_I       3
#define STRETCH_I   4
#define DOWNSCALE0_I 5
#define DOWNSCALE1_I 6
#define DOWNSCALE2_I 7
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
#define DOWNSCALE0_S 256
#define DOWNSCALE1_S (DOWNSCALE0_S >> 2)
#define DOWNSCALE2_S (DOWNSCALE1_S >> 2)
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
    createTex(&p->lookup_tex, LOOKUP_W, NUM_COLORS, GL_RGB, GL_NEAREST, GL_NEAREST, result);

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
    v = c->rgbppu + 0.1f;
    glUniform1f(c->_rgbppu_loc, v);
}

static void updateControlUniformsStretch(const es2n_controls *c)
{
    GLfloat v;
    v = c->scanlines * c->crt_enabled * (4.0f/255.0f);
    glUniform1f(c->_scanlines_loc, v);
}

static void updateControlUniformsDisp(const es2n_controls *c, int use_gamma)
{
    GLfloat v;
    v = c->crt_enabled * -2.0f * (c->convergence+0.3f);
    glUniform1f(c->_convergence_loc, v);

    if (use_gamma) {
        v = 0.45f + 0.1f*c->gamma;
        glUniform1f(c->_disp_gamma_loc, v);
    } else {
        glUniform1f(c->_disp_gamma_loc, 1.0f);
    }
    v = 0.03f + 0.03f*c->glow;
    glUniform1f(c->_disp_glow_loc, v);
    v = (0.9f-c->rgbppu) * 0.4f * (c->sharpness+0.5f);
    GLfloat sharpen_kernel[3 * 3] = {
        0.0f, -v, 0.0f, 
        0.0f, 1.0f+2.0f*v, 0.0f, 
        0.0f, -v, 0.0f 
    };
    glUniformMatrix3fv(c->_sharpen_kernel_loc, 1, GL_FALSE, sharpen_kernel);
}

static void updateControlUniformsTV(const es2n_controls *c)
{
    GLfloat v;
    v = 0.45f + 0.1f*c->gamma;
    glUniform1f(c->_tv_gamma_loc, v);
    v = 0.03f + 0.03f*c->glow;
    glUniform1f(c->_tv_glow_loc, v);
}

static void updateUniformsDownscale(const es2n *p, int size, int texIdx)
{
    GLfloat s = 1.0f / (GLfloat) size;
    glUniform2f(p->_downscale_invResolution_loc, s, s);
    glUniform1i(p->_downscale_downscaleTex_loc, texIdx);
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
    c->_scanlines_loc = glGetUniformLocation(prog, "u_scanlines");
    updateControlUniformsStretch(&p->controls);
}

static void initUniformsDisp(es2n *p)
{
    GLint k;
    GLuint prog = p->disp_prog;

    k = glGetUniformLocation(prog, "u_stretchTex");
    glUniform1i(k, STRETCH_I);
    k = glGetUniformLocation(prog, "u_downscale1Tex");
    glUniform1i(k, DOWNSCALE1_I);
    k = glGetUniformLocation(prog, "u_mvp");
    glUniformMatrix4fv(k, 1, GL_FALSE, p->mvp_mat);

    es2n_controls *c = &p->controls;
    c->_convergence_loc = glGetUniformLocation(prog, "u_convergence");
    c->_sharpen_kernel_loc = glGetUniformLocation(prog, "u_sharpenKernel");
    c->_disp_gamma_loc = glGetUniformLocation(prog, "u_gamma");
    c->_disp_glow_loc = glGetUniformLocation(prog, "u_glow");
    updateControlUniformsDisp(&p->controls, 1);
}

static void initUniformsTV(es2n *p)
{
    GLint k;
    GLuint prog = p->tv_prog;

    k = glGetUniformLocation(prog, "u_downscale0Tex");
    glUniform1i(k, DOWNSCALE0_I);
    k = glGetUniformLocation(prog, "u_downscale1Tex");
    glUniform1i(k, DOWNSCALE1_I);
    k = glGetUniformLocation(prog, "u_downscale2Tex");
    glUniform1i(k, DOWNSCALE2_I);
    k = glGetUniformLocation(prog, "u_mvp");
    glUniformMatrix4fv(k, 1, GL_FALSE, p->mvp_mat);

    es2n_controls *c = &p->controls;
    c->_tv_gamma_loc = glGetUniformLocation(prog, "u_gamma");
    c->_tv_glow_loc = glGetUniformLocation(prog, "u_glow");
    updateControlUniformsTV(&p->controls);
}

static void initUniformsDownscale(es2n *p)
{
    GLuint prog = p->downscale_prog;

    p->_downscale_invResolution_loc = glGetUniformLocation(prog, "u_invResolution");
    p->_downscale_downscaleTex_loc = glGetUniformLocation(prog, "u_downscaleTex");
    updateUniformsDownscale(p, DOWNSCALE0_S, DOWNSCALE0_I);
}

static void passRGB(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, p->rgb_fb);
    glViewport(0, 8, RGB_W, 240);
    glUseProgram(p->rgb_prog);
    updateControlUniformsRGB(&p->controls);

    if (p->controls.crt_enabled) {
        glEnable(GL_BLEND);
    }
    meshRender(&p->quad_mesh);
    glDisable(GL_BLEND);
}

static void passStretch(es2n *p)
{
    glBindFramebuffer(GL_FRAMEBUFFER, p->stretch_fb);
// TODO: check defines
    glViewport(0, 4*8, RGB_W, 4*240);
    glUseProgram(p->stretch_prog);
    updateControlUniformsStretch(&p->controls);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

static void passDownscale(es2n *p)
{
    glUseProgram(p->disp_prog);
    glBindFramebuffer(GL_FRAMEBUFFER, p->downscale_fb[0]);
    glViewport(0, 0, DOWNSCALE0_S, DOWNSCALE0_S);
    updateControlUniformsDisp(&p->controls, 0);
    meshRender(&p->screen_mesh);

    glUseProgram(p->downscale_prog);
    glBindFramebuffer(GL_FRAMEBUFFER, p->downscale_fb[1]);
    glViewport(0, 0, DOWNSCALE1_S, DOWNSCALE1_S);
    updateUniformsDownscale(p, DOWNSCALE0_S, DOWNSCALE0_I);
    meshRender(&p->quad_mesh);

    glBindFramebuffer(GL_FRAMEBUFFER, p->downscale_fb[2]);
    glViewport(0, 0, DOWNSCALE2_S, DOWNSCALE2_S);
    updateUniformsDownscale(p, DOWNSCALE1_S, DOWNSCALE1_I);
    meshRender(&p->quad_mesh);
}

static void passDisp(es2n *p)
{
    // tv screen
    glUseProgram(p->disp_prog);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(p->viewport[0], p->viewport[1], p->viewport[2], p->viewport[3]);
    updateControlUniformsDisp(&p->controls, 1);
    meshRender(&p->screen_mesh);

    // tv plastic rim
    glUseProgram(p->tv_prog);
    meshRender(&p->tv_mesh);
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
//    printf("left:%d right:%d top:%d bottom:%d\n", left, right, top, bottom);
    memset(p, 0, sizeof(es2n));

    // Build MVP matrix.
    GLfloat trans[3] = { 0, 0, -2.5 };
    GLfloat proj[4*4];
    GLfloat view[4*4];
    mat4Proj(proj, 0.25*M_PI*224.0/280.0, 280.0/224.0, 0.125, 16.0);
    mat4Trans(view, trans);
    mat4Mul(p->mvp_mat, proj, view);

    p->overscan_pixels = (GLubyte*) malloc(OVERSCAN_W*240);
    p->overscan_color = 0xFE; // Set bogus value to ensure overscan update.

    glGetIntegerv(GL_VIEWPORT, p->viewport);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBlendFunc(GL_ONE_MINUS_CONSTANT_COLOR, GL_CONSTANT_COLOR);
    glBlendColor(PERSISTENCE_R, PERSISTENCE_G, PERSISTENCE_B, 0.0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glEnableVertexAttribArray(0);

    createMesh(&p->quad_mesh, p->rgb_prog, mesh_quad_vert_num, 2*mesh_quad_face_num, mesh_quad_verts, 0, 0, 0);

    // Setup input pixels texture.
    glActiveTexture(TEX(IDX_I));
    createTex(&p->idx_tex, IDX_W, 256, GL_LUMINANCE, GL_NEAREST, GL_NEAREST, 0);

    // Setup input de-emphasis rows texture.
    glActiveTexture(TEX(DEEMP_I));
    createTex(&p->deemp_tex, 256, 1, GL_LUMINANCE, GL_NEAREST, GL_NEAREST, 0);

    genLookupTex(p);

    // Configure RGB framebuffer.
    glActiveTexture(TEX(RGB_I));
    createFBTex(&p->rgb_tex, &p->rgb_fb, RGB_W, 256, GL_RGB, GL_NEAREST, GL_NEAREST);
    p->rgb_prog = buildShader(rgb_vert_src, rgb_frag_src);
    initUniformsRGB(p);

    // Setup stretch framebuffer.
    glActiveTexture(TEX(STRETCH_I));
    createFBTex(&p->stretch_tex, &p->stretch_fb, RGB_W, STRETCH_H, GL_RGB, GL_LINEAR, GL_LINEAR);
    p->stretch_prog = buildShader(stretch_vert_src, stretch_frag_src);
    initUniformsStretch(p);

    // Setup downscale framebuffers.
    glActiveTexture(TEX(DOWNSCALE0_I));
    createFBTex(&p->downscale_tex[0], &p->downscale_fb[0], DOWNSCALE0_S, DOWNSCALE0_S, GL_RGB, GL_LINEAR, GL_LINEAR);
    glActiveTexture(TEX(DOWNSCALE1_I));
    createFBTex(&p->downscale_tex[1], &p->downscale_fb[1], DOWNSCALE1_S, DOWNSCALE1_S, GL_RGB, GL_LINEAR, GL_LINEAR);
    glActiveTexture(TEX(DOWNSCALE2_I));
    createFBTex(&p->downscale_tex[2], &p->downscale_fb[2], DOWNSCALE2_S, DOWNSCALE2_S, GL_RGB, GL_LINEAR, GL_LINEAR);

    // Setup downscale shader.
    p->downscale_prog = buildShader(downscale_vert_src, downscale_frag_src);
    initUniformsDownscale(p);

    // Setup display (output) shader.
    p->disp_prog = buildShader(disp_vert_src, disp_frag_src);
    createMesh(&p->screen_mesh, p->disp_prog, mesh_screen_vert_num, 3*mesh_screen_face_num, mesh_screen_verts, mesh_screen_norms, mesh_screen_uvs, mesh_screen_faces);
    initUniformsDisp(p);

    // Setup TV shader.
    p->tv_prog = buildShader(tv_vert_src, tv_frag_src);
    createMesh(&p->tv_mesh, p->tv_prog, mesh_rim_vert_num, 3*mesh_rim_face_num, mesh_rim_verts, mesh_rim_norms, 0, mesh_rim_faces);
    initUniformsTV(p);
}

void es2nDeinit(es2n *p)
{
    deleteFBTex(&p->rgb_tex, &p->rgb_fb);
    deleteFBTex(&p->stretch_tex, &p->stretch_fb);
    deleteFBTex(&p->downscale_tex[0], &p->downscale_fb[0]);
    deleteFBTex(&p->downscale_tex[1], &p->downscale_fb[1]);
    deleteFBTex(&p->downscale_tex[2], &p->downscale_fb[2]);
    deleteTex(&p->idx_tex);
    deleteTex(&p->deemp_tex);
    deleteTex(&p->lookup_tex);
    deleteShader(&p->rgb_prog);
    deleteShader(&p->stretch_prog);
    deleteShader(&p->downscale_prog);
    deleteShader(&p->disp_prog);
    deleteShader(&p->tv_prog);
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

    passRGB(p);
    passStretch(p);
    passDownscale(p);
    passDisp(p);
}

