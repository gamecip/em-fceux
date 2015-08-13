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

attribute vec4 a_0;
attribute vec3 a_1;
attribute vec2 a_2;
uniform mat4 u_mvp;
uniform vec2 u_uvScale;
varying vec2 v_uv;
varying vec3 v_norm;
varying vec3 v_pos;
varying vec2 v_noiseUV;
void main()
{
	v_uv = 0.5 + u_uvScale.xy * (a_2 - 0.5);
	v_norm = a_1;
	v_pos = a_0.xyz;
// TODO: tsone: duplicated (screen, tv), manual homogenization
	gl_Position = u_mvp * a_0;
	gl_Position = vec4(gl_Position.xy / gl_Position.w, 0.0, 1.0);
	vec2 vwc = gl_Position.xy * 0.5 + 0.5;
	v_noiseUV = vec2(SCREEN_W, SCREEN_H) * vwc / vec2(NOISE_W, NOISE_H);
}
