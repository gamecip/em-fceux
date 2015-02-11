#ifndef _ES2N_H_
#define _ES2N_H_
#include "es2utils.h"

typedef struct t_es2n_controls
{
    // All controls should be in range [-1,1]. Default is 0.
    GLfloat brightness; // Brightness control.
    GLfloat contrast;   // Contrast control.
    GLfloat color;      // Color control.
    GLfloat gamma;      // Gamma control.
    GLfloat rgbppu;     // RGB PPU control.

    // Controls for CRT emulation. If CRT emulation is disabled, these do nothing.
    int crt_enabled;    // Set to zero to disable CRT emulation.
    GLfloat scanline;   // CRT scanline strength.
    GLfloat convergence; // CRT red-blue convergence.
    GLfloat sharpness;  // CRT sharpness control.

    // Uniform locations.
    GLint _brightness_loc;
    GLint _contrast_loc;
    GLint _color_loc;
    GLint _disp_gamma_loc;
    GLint _rgbppu_loc;
    GLint _convergence_loc;
    GLint _sharpen_kernel_loc;
    GLint _scanline_loc;

    GLint _tv_gamma_loc;
} es2n_controls;

typedef struct t_es2n
{
    GLuint idx_tex;     // Input indexed color texture (NES palette, 256x240).
    GLuint deemp_tex;   // Input de-emphasis bits per row (240x1).
    GLuint lookup_tex;  // Palette to voltage levels lookup texture.

    GLuint rgb_fb;      // Framebuffer for output RGB texture generation.
    GLuint rgb_tex;     // Output RGB texture (1024x256x3).
    GLuint rgb_prog;    // Shader for RGB.

    GLuint stretch_fb;   // Framebuffer for stretched RGB texture.
    GLuint stretch_tex;  // Output stretched RGB texture (1024x1024x3).
    GLuint stretch_prog; // Shader for stretched RGB.

    GLuint disp_prog;   // Shader for final display.

    GLint viewport[4];  // Original viewport.

    GLfloat mvp_mat[4*4]; // MVP matrix for the meshes.
   
    GLuint tv_prog;   // Shader for TV.

    GLuint downscale_fb[3];  // Framebuffers for downscaling.
    GLuint downscale_tex[3]; // Downscale textures.
    GLuint downscale_prog;   // Shader for downscaling.

    es2_mesh quad_mesh;
    es2_mesh screen_mesh;
    es2_mesh tv_mesh;

    GLubyte overscan_color;   // Current overscan color (background/zero color).
    GLubyte *overscan_pixels; // Temporary overscan pixels (1x240).

    GLfloat yiq_mins[3];
    GLfloat yiq_maxs[3];

    es2n_controls controls;

    // Uniform locations.
    GLint _downscale_invResolution_loc;
    GLint _downscale_downscaleTex_loc;
} es2n;

void es2nInit(es2n *p, int left, int right, int top, int bottom);
void es2nUpdateControls(es2n *p);
void es2nDeinit(es2n *p);
void es2nRender(es2n *p, GLubyte *pixels, GLubyte *row_deemp, GLubyte overscan_color);

#endif
