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

#define M_PI		3.14159265359

uniform float u_scanlines;
uniform sampler2D u_sharpenTex;
varying vec2 v_uv[2];
void main()
{
	// Sample adjacent scanlines and average to smoothen slightly vertically.
	vec3 c0 = texture2D(u_sharpenTex, v_uv[0]).rgb;
	vec3 c1 = texture2D(u_sharpenTex, v_uv[1]).rgb;
	vec3 color = c0*c0 + c1*c1; // Average linear-space color values.
//	vec3 color = c0 + c1;
	// Use oscillator as scanline modulator.
	float scanlines = u_scanlines * (1.0 - abs(sin(M_PI*IDX_H * v_uv[0].y - M_PI*0.125)));
	// This formula dims dark colors, but keeps brights. Output is linear.
	gl_FragColor = vec4(sqrt(mix(0.5 * color, max(color - 1.0, 0.0), scanlines)), 1.0);
//	gl_FragColor = vec4(mix(0.5 * color, max(color - 1.0, 0.0), scanlines), 1.0);
}
