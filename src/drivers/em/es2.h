#ifndef _ES2_H_
#define _ES2_H_
#include "es2util.h"

// Uniform locations.
typedef struct t_es2_uniforms
{
	GLint _rgb_brightness_loc;
	GLint _rgb_contrast_loc;
	GLint _rgb_color_loc;
	GLint _rgb_rgbppu_loc;
	GLint _rgb_gamma_loc;
	GLint _rgb_noiseAmp_loc;
	GLint _rgb_noiseRnd_loc;

	GLint _downsample_offsets_loc;
	GLint _downsample_downsampleTex_loc;

	GLint _sharpen_kernel_loc;
	GLint _sharpen_convergence_loc;

	GLint _stretch_scanlines_loc;
	GLint _stretch_smoothenOffs_loc;

	GLint _combine_glow_loc;

} es2_uniforms;

typedef struct t_es2
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

    GLuint direct_prog;	// Shader for rendering texture directly to the screen.

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

} es2;

void es2Init(double aspect);
void es2UpdateController(int idx, double v);
void es2Deinit();
void es2SetViewport(int width, int height);
void es2Render(GLubyte *pixels, GLubyte *row_deemp, GLubyte overscan_color);

#endif
