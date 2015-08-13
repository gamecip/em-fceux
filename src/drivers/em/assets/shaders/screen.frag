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

uniform sampler2D u_stretchTex;
uniform sampler2D u_noiseTex;
varying vec2 v_uv;
varying vec3 v_norm;
varying vec3 v_pos;
varying vec2 v_noiseUV;
void main()
{
	// Base radiance from phosphors.
	vec3 color = texture2D(u_stretchTex, v_uv).rgb;
// TODO: test encode gamma
	color *= color;
	// Radiance from dust scattering.
	vec3 n = normalize(v_norm);
// TODO: test encode gamma
	vec3 tmp = texture2D(u_stretchTex, v_uv - 0.021*n.xy).rgb;
	color += 0.018 * tmp*tmp;
//	color += 0.018 * texture2D(u_stretchTex, v_uv - 0.021*n.xy).rgb;

	// Set black if outside the border
//	vec2 uvd = max(abs(v_uv - 0.5) - vec2(128.0-6.5, 112.0-3.5) / vec2(IDX_W, IDX_H), 0.0);
	vec2 uvd = max(abs(v_uv - 0.5) - 0.5 * vec2(1.0 - 9.5/INPUT_W, 1.0 - 7.0/IDX_H), 0.0);
	float border = clamp(3.0 - 3.0*3000.0 * dot(uvd, uvd), 0.0, 1.0);
	color *= border;

	float shade = shadeBlinn(v_pos, n);
	// Shading affected by black screen border material.
	shade *= 0.638*border + 0.362;

	// CRT emitter radiance attenuation from the curvature
//	color *= pow(dot(n, v), 16.0*(u_mouse.x/256.0));

	float noise = (1.5/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);

	// Use gamma encoding as we may output smooth color changes here.
	gl_FragColor = vec4(sqrt(color + shade) + noise, 1.0);
//	gl_FragColor = vec4(vec3(2.0*abs((128.0/1.5) * noise)), 1.0);
//	gl_FragColor = vec4(vec3(texture2D(u_noiseTex, v_noiseUV).r), 1.0);
//	gl_FragColor = vec4(vec3(texture2D(u_noiseTex, gl_FragCoord.xy/256.0 - 2.0).r), 1.0);
}
