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

uniform vec3 u_lightDir;
uniform vec3 u_viewPos;
uniform vec4 u_material;
uniform vec3 u_fresnel;
uniform vec4 u_shadowPlane;

float shadeBlinn(const vec3 p, const vec3 n)
{
	vec3 v = normalize(u_viewPos - p);
	vec3 h = normalize(u_lightDir + v);
	float ndotl = dot(n, u_lightDir);
	float result;
	if (ndotl > 0.0) {
		// Front light.
		float ndoth = max(dot(n, h), 0.0);
		float fr = u_fresnel[0] + u_fresnel[1] * pow(1.0-ndotl, u_fresnel[2]);
		result = mix(u_material[0], u_material[1] * pow(ndoth, u_material[2]), fr) * ndotl;
	} else {
		// Back light (bi-directional lighting).
		result = -ndotl * u_material[3];
	}
//	float ddiag = -0.9*u_mouse.y/256.0 + 0.9 + dot(normalize(cross(vec3(1.0, 0.0, 0.0), u_lightDir)), p);
//	float dflat = 0.2*u_mouse.x/224.0 - 0.1 + p.z;
//	float ddiag = -0.9*155.0/256.0 + 0.9 + dot(normalize(cross(vec3(1.0, 0.0, 0.0), u_lightDir)), p);
//	float dflat = 0.2*88.0/224.0 - 0.1 + p.z;
	// Calculate (fake) shadow using two planes
	float ddiag = dot(u_shadowPlane.xyz, p) - u_shadowPlane.w;
	float dflat = p.z - 0.023;
	result *= 0.21 + 0.79*clamp(38.0 * max(ddiag, dflat), 0.0, 1.0);
	return result;
}

uniform sampler2D u_downsample1Tex;
uniform sampler2D u_downsample3Tex;
uniform sampler2D u_downsample5Tex;
uniform sampler2D u_noiseTex;
varying vec3 v_radiance;
varying vec2 v_noiseUVs[2];
varying vec3 v_pos;
varying vec3 v_norm;
varying vec2 v_uv;
varying float v_blend;
void main()
{
	// Sample noise at low and high frequencies.
	float noiseLF = texture2D(u_noiseTex, v_noiseUVs[1]).r;
	float noiseHF = texture2D(u_noiseTex, v_noiseUVs[0]).r;

	// Sample downsampled (blurry) textures and gamma decode.
	vec3 ds0 = texture2D(u_downsample1Tex, v_uv).rgb;
	vec3 ds1 = texture2D(u_downsample3Tex, v_uv).rgb;
	vec3 ds2 = texture2D(u_downsample5Tex, v_uv).rgb;
	ds0 *= ds0;
	ds1 *= ds1;
	ds2 *= ds2;

	vec3 color;

	// Approximate diffuse and specular by sampling downsampled texture and
	// blending according to the fragment distance from the TV screen (emitter).
	color = v_radiance.r * mix(ds1, ds2, v_blend);
	color += v_radiance.g * mix(ds0, ds1, v_blend);

	// Add slight graininess for rough plastic (also to hide interpolation artifacts).
	float graininess = 0.15625 * v_blend + 0.14578;
	color *= 1.0 - graininess * sqrt(abs(2.0*noiseLF - 1.0));

// TODO: tsone: As a matter of fact, this is the proper shading calculation...
	vec3 n = normalize(v_norm);
	float shade = shadeBlinn(v_pos, n);

// TODO: tsone: tone-mapping?

// TODO: tsone: vignette?
//	color = vec3(0.0);
//	vec2 nuv = 2.0*v_uv - 1.0;
//	float vignette = max(1.0 - length(nuv), 0.0);
//	vec3 lightColor = vec3(v_shade);

	// Gamma encode color w/ sqrt().
	gl_FragColor = vec4(sqrt(color + shade) + (1.5/128.0) * (noiseHF-0.5), 1.0);
}
