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
    GLfloat glow;       // Glow control.
    GLfloat sharpness;  // Sharpness control.
    GLfloat rgbppu;     // RGB PPU control.

    // Controls for CRT emulation. If CRT emulation is disabled, these do nothing.
    int crt_enabled;    // Set to zero to disable CRT emulation.
    GLfloat scanlines;  // CRT scanlines strength.
    GLfloat convergence; // CRT red-blue convergence.
    GLfloat noise;      // CRT noise control.

    // Uniform locations.
    GLint _brightness_loc;
    GLint _contrast_loc;
    GLint _color_loc;
    GLint _gamma_loc;
    GLint _noiseAmp_loc;
    GLint _noiseRnd_loc;
    GLint _screen_uvScale_loc;
    GLint _screen_mvp_loc;
    GLint _rgbppu_loc;
    GLint _convergence_loc;
    GLint _sharpen_kernel_loc;
    GLint _scanlines_loc;

    GLint _combine_glow_loc;

} es2n_controls;

typedef struct t_es2n
{
    GLuint idx_tex;     // Input indexed color texture (NES palette, 256x240).
    GLuint deemp_tex;   // Input de-emphasis bits per row (240x1).
    GLuint lookup_tex;  // Palette to voltage levels lookup texture.

    GLuint noise_tex;   // Noise texture.

    GLuint rgb_fb;      // Framebuffer for output RGB texture generation.
    GLuint rgb_tex;     // Output RGB texture.
    GLuint rgb_prog;    // Shader for RGB.

    GLuint sharpen_fb;  // Framebuffer for sharpened RGB texture.
    GLuint sharpen_tex; // Sharpened RGB texture.
    GLuint sharpen_prog; // Shader for sharpening.

    GLuint stretch_fb;   // Framebuffer for stretched RGB texture.
    GLuint stretch_tex;  // Output stretched RGB texture.
    GLuint stretch_prog; // Shader for stretched RGB.

    GLuint screen_prog;  // Shader for screen.

    GLuint tv_fb;        // Framebuffer to render screen and TV.
    GLuint tv_tex;       // Texture for screen/TV framebuffer.
    GLuint tv_prog;      // Shader for tv.

    GLuint combine_prog; // Shader for combine.

    GLint viewport[4];  // Screen viewport.

    GLfloat mvp_mat[4*4]; // Perspective MVP matrix for the meshes.
   
    GLuint downsample_fb[6];  // Framebuffers for downscaling.
    GLuint downsample_tex[6]; // Downsample textures.
    GLuint downsample_prog;   // Shader for downscaling.

    es2_mesh quad_mesh;
    es2_mesh screen_mesh;
    es2_mesh tv_mesh;

    GLubyte overscan_color;   // Current overscan color (background/zero color).
    GLubyte *overscan_pixels; // Temporary overscan pixels (1x240).

    GLfloat yiq_mins[3];
    GLfloat yiq_maxs[3];

    es2n_controls controls;

    // Uniform locations.
    GLint _downsample_weights_loc;
    GLint _downsample_offsets_loc;
    GLint _downsample_downsampleTex_loc;
} es2n;

void es2nInit(int left, int right, int top, int bottom);
void es2nUpdateControls();
void es2nDeinit();
void es2nRender(GLubyte *pixels, GLubyte *row_deemp, GLubyte overscan_color);

#endif
