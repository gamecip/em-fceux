#define _NUM_COLORS	(64 * 8)
#define _NUM_PHASES	3
#define _NUM_SUBPS	4
#define _NUM_TAPS	5
#define _LOOKUP_W	64
#define _OVERSCAN_W	12
#define _INPUT_W	256
#define _INPUT_H	240
#define _INPUT_ROW_OFFS	((_INPUT_H-_IDX_H) / 2)
#define _IDX_W		(_INPUT_W + 2*_OVERSCAN_W)
#define _IDX_H		224
#define _NOISE_W	256
#define _NOISE_H	256
#define _RGB_W		(_NUM_SUBPS * _IDX_W)
#define _SCREEN_W	(_NUM_SUBPS * _INPUT_W)
#define _SCREEN_H	(4 * _IDX_H)
#define _YW2		6.0
#define _CW2		12.0

#define NUM_COLORS	float(_NUM_COLORS)
#define NUM_PHASES	float(_NUM_PHASES)
#define NUM_SUBPS	float(_NUM_SUBPS)
#define NUM_TAPS	float(_NUM_TAPS)
#define LOOKUP_W	float(_LOOKUP_W)
#define OVERSCAN_W	float(_OVERSCAN_W)
#define INPUT_W		float(_INPUT_W)
#define INPUT_H		float(_INPUT_H)
#define INPUT_ROW_OFFS	float(_INPUT_ROW_OFFS)
#define IDX_W		float(_IDX_W)
#define IDX_H		float(_IDX_H)
#define NOISE_W		float(_NOISE_W)
#define NOISE_H		float(_NOISE_H)
#define RGB_W		float(_RGB_W)
#define SCREEN_W	float(_SCREEN_W)
#define SCREEN_H	float(_SCREEN_H)
#define YW2		float(_YW2)
#define CW2		float(_CW2)

uniform sampler2D u_idxTex;
uniform sampler2D u_deempTex;
uniform sampler2D u_lookupTex;
uniform sampler2D u_noiseTex;
uniform vec3 u_mins;
uniform vec3 u_maxs;
uniform float u_brightness;
uniform float u_contrast;
uniform float u_color;
uniform float u_gamma;
uniform float u_noiseAmp;
varying vec2 v_uv;
varying vec2 v_deemp_uv;
varying vec2 v_noiseUV;
const mat3 c_convMat = mat3(
	1.0,        1.0,        1.0,       // Y
	0.946882,   -0.274788,  -1.108545, // I
	0.623557,   -0.635691,  1.709007   // Q
);
#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)

void main()
{
	float deemp = (255.0/511.0) * 2.0 * texture2D(u_deempTex, v_deemp_uv).r;
	float uv_y = (255.0/511.0) * texture2D(u_idxTex, v_uv).r + deemp;
	// Snatch in RGB PPU; uv.x is already calculated, so just read from lookup tex with u=1.0.
	vec3 yiq = RESCALE(texture2D(u_lookupTex, vec2(1.0, uv_y)).rgb);
	// Add noise to YIQ color, boost/reduce color and convert to RGB.
	yiq.r += u_noiseAmp * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);
	yiq.gb *= u_color;
	vec3 result = clamp(c_convMat * yiq, 0.0, 1.0);
	// Gamma convert RGB from NTSC space to space similar to sRGB.
	result = pow(result, vec3(u_gamma));
	// NOTE: While this seems to be wrong (after gamma), it works well in practice...?
	result = clamp(u_contrast * result + u_brightness, 0.0, 1.0);
	// Write linear color (banding is not visible for pixelated graphics).
// TODO: test encode gamma
	gl_FragColor = vec4(result, 1.0);
}
