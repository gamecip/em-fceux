static const char* common_src =
"precision highp float;\n"
DEFINE(M_PI)
#if DBG_MODE
"uniform vec3 u_mouse;\n"
#endif
"\n"
"uniform vec3 u_lightDir;\n"
"uniform vec3 u_viewPos;\n"
"uniform vec4 u_material;\n"
"uniform vec3 u_fresnel;\n"
"uniform vec4 u_shadowPlane;\n"
"\n"
"float shadeBlinn(const vec3 p, const vec3 n)\n"
"{\n"
	"vec3 v = normalize(u_viewPos - p);\n"
	"vec3 h = normalize(u_lightDir + v);\n"
	"float ndotl = dot(n, u_lightDir);\n"
	"float result;\n"
	"if (ndotl > 0.0) {"
		// Front light.
		"float ndoth = max(dot(n, h), 0.0);\n"
		"float fr = u_fresnel[0] + u_fresnel[1] * pow(1.0-ndotl, u_fresnel[2]);\n"
		"result = mix(u_material[0], u_material[1] * pow(ndoth, u_material[2]), fr) * ndotl;\n"
	"} else {\n"
		// Back light (bi-directional lighting).
		"result = -ndotl * u_material[3];\n"
	"}\n"
//	"float ddiag = -0.9*u_mouse.y/256.0 + 0.9 + dot(normalize(cross(vec3(1.0, 0.0, 0.0), u_lightDir)), p);\n"
//	"float dflat = 0.2*u_mouse.x/224.0 - 0.1 + p.z;\n"
//	"float ddiag = -0.9*155.0/256.0 + 0.9 + dot(normalize(cross(vec3(1.0, 0.0, 0.0), u_lightDir)), p);\n"
//	"float dflat = 0.2*88.0/224.0 - 0.1 + p.z;\n"
	// Calculate (fake) shadow using two planes
	"float ddiag = dot(u_shadowPlane.xyz, p) - u_shadowPlane.w;\n"
	"float dflat = p.z - 0.023;\n"
	"result *= 0.21 + 0.79*clamp(38.0 * max(ddiag, dflat), 0.0, 1.0);\n"
	"return result;\n"
"}\n";

static const char* rgb_vert_src =
DEFINE(IDX_W)
DEFINE(IDX_H)
DEFINE(NOISE_W)
DEFINE(NOISE_H)
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"uniform vec2 u_noiseRnd;\n"
"varying vec2 v_uv;\n"
"varying vec2 v_deemp_uv;\n"
"varying vec2 v_noiseUV;\n"
"void main()\n"
"{\n"
	"v_uv = a_2;\n"
	"v_deemp_uv = vec2(v_uv.y, 0.0);\n"
	"v_noiseUV = vec2(IDX_W/NOISE_W, IDX_H/NOISE_H)*a_2 + u_noiseRnd;\n"
	"gl_Position = a_0;\n"
"}\n";
static const char* rgb_frag_src =
DEFINE(NUM_SUBPS)
DEFINE(NUM_TAPS)
DEFINE(LOOKUP_W)
DEFINE(IDX_W)
DEFINE(YW2)
DEFINE(CW2)
"uniform sampler2D u_idxTex;\n"
"uniform sampler2D u_deempTex;\n"
"uniform sampler2D u_lookupTex;\n"
"uniform sampler2D u_noiseTex;\n"
"uniform vec3 u_mins;\n"
"uniform vec3 u_maxs;\n"
"uniform float u_brightness;\n"
"uniform float u_contrast;\n"
"uniform float u_color;\n"
"uniform float u_gamma;\n"
"uniform float u_noiseAmp;\n"
"varying vec2 v_uv;\n"
"varying vec2 v_deemp_uv;\n"
"varying vec2 v_noiseUV;\n"
"const mat3 c_convMat = mat3(\n"
	"1.0,        1.0,        1.0,\n"       // Y
	"0.946882,   -0.274788,  -1.108545,\n" // I
	"0.623557,   -0.635691,  1.709007\n"   // Q
");\n"
"#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)\n"

"void main()\n"
"{\n"
	"float deemp = (255.0/511.0) * 2.0 * texture2D(u_deempTex, v_deemp_uv).r;\n"
	"float uv_y = (255.0/511.0) * texture2D(u_idxTex, v_uv).r + deemp;\n"
	// Snatch in RGB PPU; uv.x is already calculated, so just read from lookup tex with u=1.0.
	"vec3 yiq = RESCALE(texture2D(u_lookupTex, vec2(1.0, uv_y)).rgb);\n"
	// Add noise to YIQ color, boost/reduce color and convert to RGB.
	"yiq.r += u_noiseAmp * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"
	"yiq.gb *= u_color;\n"
	"vec3 result = clamp(c_convMat * yiq, 0.0, 1.0);\n"
	// Gamma convert RGB from NTSC space to space similar to sRGB.
	"result = pow(result, vec3(u_gamma));\n"
	// NOTE: While this seems to be wrong (after gamma), it works well in practice...?
	"result = clamp(u_contrast * result + u_brightness, 0.0, 1.0);\n"
	// Write linear color (banding is not visible for pixelated graphics).
// TODO: test encode gamma
	"gl_FragColor = vec4(result, 1.0);\n"
"}\n";

static const char* ntsc_vert_src =
DEFINE(NUM_TAPS)
DEFINE(IDX_W)
DEFINE(IDX_H)
DEFINE(NOISE_W)
DEFINE(NOISE_H)
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"uniform vec2 u_noiseRnd;\n"
"varying vec2 v_uv[int(NUM_TAPS)];\n"
"varying vec2 v_deemp_uv;\n"
"varying vec2 v_noiseUV;\n"
"#define UV_OUT(i_, o_) v_uv[i_] = vec2(uv.x + (o_)/IDX_W, uv.y)\n"
"void main()\n"
"{\n"
	"vec2 uv = a_2;\n"
	"UV_OUT(0,-2.0);\n"
	"UV_OUT(1,-1.0);\n"
	"UV_OUT(2, 0.0);\n"
	"UV_OUT(3, 1.0);\n"
	"UV_OUT(4, 2.0);\n"
	"v_deemp_uv = vec2(uv.y, 0.0);\n"
	"v_noiseUV = vec2(IDX_W/NOISE_W, IDX_H/NOISE_H)*a_2 + u_noiseRnd;\n"
	"gl_Position = a_0;\n"
"}\n";
static const char* ntsc_frag_src =
DEFINE(NUM_SUBPS)
DEFINE(NUM_TAPS)
DEFINE(LOOKUP_W)
DEFINE(IDX_W)
DEFINE(YW2)
DEFINE(CW2)
"uniform sampler2D u_idxTex;\n"
"uniform sampler2D u_deempTex;\n"
"uniform sampler2D u_lookupTex;\n"
"uniform sampler2D u_noiseTex;\n"
"uniform vec3 u_mins;\n"
"uniform vec3 u_maxs;\n"
"uniform float u_brightness;\n"
"uniform float u_contrast;\n"
"uniform float u_color;\n"
"uniform float u_gamma;\n"
"uniform float u_noiseAmp;\n"
"varying vec2 v_uv[int(NUM_TAPS)];\n"
"varying vec2 v_deemp_uv;\n"
"varying vec2 v_noiseUV;\n"
"const mat3 c_convMat = mat3(\n"
	"1.0,        1.0,        1.0,\n"       // Y
	"0.946882,   -0.274788,  -1.108545,\n" // I
	"0.623557,   -0.635691,  1.709007\n"   // Q
");\n"
"#define P(i_)  p = floor(IDX_W * v_uv[i_])\n"
"#define U(i_)  (mod(p.x - p.y, 3.0)*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + float(i_)) / (LOOKUP_W-1.0)\n"
"#define V(i_)  ((255.0/511.0) * texture2D(u_idxTex, v_uv[i_]).r + deemp)\n"
"#define UV(i_) uv = vec2(U(i_), V(i_))\n"
"#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)\n"
"#define SMP(i_) P(i_); UV(i_); yiq += RESCALE(texture2D(u_lookupTex, uv).rgb)\n"

"void main()\n"
"{\n"
	"float deemp = (255.0/511.0) * 2.0 * texture2D(u_deempTex, v_deemp_uv).r;\n"
	"float subp = mod(floor(NUM_SUBPS*IDX_W * v_uv[int(NUM_TAPS)/2].x), NUM_SUBPS);\n"
	"vec2 p;\n"
	"vec2 la;\n"
	"vec2 uv;\n"
	"vec3 yiq = vec3(0.0);\n"
	"SMP(0);\n"
	"SMP(1);\n"
	"SMP(2);\n"
	"SMP(3);\n"
	"SMP(4);\n"
	// Working multiplier for filtered chroma to match PPU is 2/5 (for CW2=12).
	// Is this because color fringing with composite?
	"yiq *= (8.0/2.0) / vec3(YW2, CW2-2.0, CW2-2.0);\n"
	"yiq.r += u_noiseAmp * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"
	"yiq.gb *= u_color;\n"
	"vec3 result = clamp(c_convMat * yiq, 0.0, 1.0);\n"
	// Gamma convert RGB from NTSC space to space similar to sRGB.
	"result = pow(result, vec3(u_gamma));\n"
	// NOTE: While this seems to be wrong (after gamma), it works well in practice...?
	"result = clamp(u_contrast * result + u_brightness, 0.0, 1.0);\n"
	// Write linear color (banding is not visible for pixelated graphics).
// TODO: test encode gamma
	"gl_FragColor = vec4(result, 1.0);\n"
"}\n";

static const char* sharpen_vert_src =
DEFINE(OVERSCAN_W)
DEFINE(INPUT_W)
DEFINE(IDX_W)
DEFINE(RGB_W)
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"uniform float u_convergence;\n"
"varying vec2 v_uv[5];\n"
"#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0)\n"
"void main()\n"
"{\n"
	"vec2 uv = a_2;\n"
	"uv.x = floor(INPUT_W*uv.x + OVERSCAN_W) / IDX_W;\n"
	"TAP(0,-4.0);\n"
	"TAP(1, u_convergence);\n"
	"TAP(2, 0.0);\n"
	"TAP(3,-u_convergence);\n"
	"TAP(4, 4.0);\n"
	"gl_Position = a_0;\n"
"}\n";
static const char* sharpen_frag_src =
"uniform sampler2D u_rgbTex;\n"
"uniform float u_convergence;\n"
"uniform vec3 u_sharpenKernel[5];\n"
"varying vec2 v_uv[5];\n"
"void main()\n"
"{\n"
	"vec3 color = vec3(0.0);\n"
	// Sharpening done in linear color space. Output is linear.
// TODO: test encode gamma
	"for (int i = 0; i < 5; i++) {\n"
		"vec3 tmp = texture2D(u_rgbTex, v_uv[i]).rgb;\n"
		"color += u_sharpenKernel[i] * tmp*tmp;\n"
//		"color += u_sharpenKernel[i] * texture2D(u_rgbTex, v_uv[i]).rgb;\n"
	"}\n"
	"gl_FragColor = vec4(sqrt(color), 1.0);\n"
// TODO: test encode gamma
//	"gl_FragColor = vec4(color, 1.0);\n"
"}\n";

static const char* stretch_vert_src =
"uniform vec2 u_smoothenOffs;\n"
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"varying vec2 v_uv[2];\n"
"void main()\n"
"{\n"
	"vec2 uv = a_2;\n"
	"v_uv[0] = vec2(uv.x, 1.0 - uv.y);\n"
	"v_uv[1] = v_uv[0] + u_smoothenOffs;\n"
	"gl_Position = a_0;\n"
"}\n";
static const char* stretch_frag_src =
DEFINE(IDX_H)
"uniform float u_scanlines;\n"
"uniform sampler2D u_sharpenTex;\n"
"varying vec2 v_uv[2];\n"
"void main()\n"
"{\n"
	// Sample adjacent scanlines and average to smoothen slightly vertically.
	"vec3 c0 = texture2D(u_sharpenTex, v_uv[0]).rgb;\n"
	"vec3 c1 = texture2D(u_sharpenTex, v_uv[1]).rgb;\n"
	"vec3 color = c0*c0 + c1*c1;\n" // Average linear-space color values.
//	"vec3 color = c0 + c1;\n"
	// Use oscillator as scanline modulator.
	"float scanlines = u_scanlines * (1.0 - abs(sin(M_PI*IDX_H * v_uv[0].y - M_PI*0.125)));\n"
	// This formula dims dark colors, but keeps brights. Output is linear.
	"gl_FragColor = vec4(sqrt(mix(0.5 * color, max(color - 1.0, 0.0), scanlines)), 1.0);\n"
//	"gl_FragColor = vec4(mix(0.5 * color, max(color - 1.0, 0.0), scanlines), 1.0);\n"
"}\n";

static const char* screen_vert_src =
DEFINE(SCREEN_W)
DEFINE(SCREEN_H)
DEFINE(NOISE_W)
DEFINE(NOISE_H)
"attribute vec4 a_0;\n"
"attribute vec3 a_1;\n"
"attribute vec2 a_2;\n"
"uniform mat4 u_mvp;\n"
"uniform vec2 u_uvScale;\n"
"varying vec2 v_uv;\n"
"varying vec3 v_norm;\n"
"varying vec3 v_pos;\n"
"varying vec2 v_noiseUV;\n"
"void main()\n"
"{\n"
	"v_uv = 0.5 + u_uvScale.xy * (a_2 - 0.5);\n"
	"v_norm = a_1;\n"
	"v_pos = a_0.xyz;\n"
// TODO: tsone: duplicated (screen, tv), manual homogenization
	"gl_Position = u_mvp * a_0;\n"
	"gl_Position = vec4(gl_Position.xy / gl_Position.w, 0.0, 1.0);\n"
	"vec2 vwc = gl_Position.xy * 0.5 + 0.5;\n"
	"v_noiseUV = vec2(SCREEN_W, SCREEN_H) * vwc / vec2(NOISE_W, NOISE_H);\n"
"}\n";
static const char* screen_frag_src =
DEFINE(INPUT_W)
DEFINE(IDX_W)
DEFINE(IDX_H)
"uniform sampler2D u_stretchTex;\n"
"uniform sampler2D u_noiseTex;\n"
"varying vec2 v_uv;\n"
"varying vec3 v_norm;\n"
"varying vec3 v_pos;\n"
"varying vec2 v_noiseUV;\n"
"void main()\n"
"{\n"
	// Base radiance from phosphors.
	"vec3 color = texture2D(u_stretchTex, v_uv).rgb;\n"
// TODO: test encode gamma
	"color *= color;\n"
	// Radiance from dust scattering.
	"vec3 n = normalize(v_norm);\n"
// TODO: test encode gamma
	"vec3 tmp = texture2D(u_stretchTex, v_uv - 0.021*n.xy).rgb;\n"
	"color += 0.018 * tmp*tmp;\n"
//	"color += 0.018 * texture2D(u_stretchTex, v_uv - 0.021*n.xy).rgb;\n"

	// Set black if outside the border
//	"vec2 uvd = max(abs(v_uv - 0.5) - vec2(128.0-6.5, 112.0-3.5) / vec2(IDX_W, IDX_H), 0.0);\n"
	"vec2 uvd = max(abs(v_uv - 0.5) - 0.5 * vec2(1.0 - 9.5/INPUT_W, 1.0 - 7.0/IDX_H), 0.0);\n"
	"float border = clamp(3.0 - 3.0*3000.0 * dot(uvd, uvd), 0.0, 1.0);\n"
	"color *= border;\n"

	"float shade = shadeBlinn(v_pos, n);\n"
	// Shading affected by black screen border material.
	"shade *= 0.638*border + 0.362;\n"

	// CRT emitter radiance attenuation from the curvature
//	"color *= pow(dot(n, v), 16.0*(u_mouse.x/256.0));\n"

	"float noise = (1.5/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"

	// Use gamma encoding as we may output smooth color changes here.
	"gl_FragColor = vec4(sqrt(color + shade) + noise, 1.0);\n"
//	"gl_FragColor = vec4(vec3(2.0*abs((128.0/1.5) * noise)), 1.0);\n"
//	"gl_FragColor = vec4(vec3(texture2D(u_noiseTex, v_noiseUV).r), 1.0);\n"
//	"gl_FragColor = vec4(vec3(texture2D(u_noiseTex, gl_FragCoord.xy/256.0 - 2.0).r), 1.0);\n"
"}\n";

static const char* tv_vert_src =
DEFINE(SCREEN_W)
DEFINE(SCREEN_H)
DEFINE(NOISE_W)
DEFINE(NOISE_H)
"attribute vec4 a_0;\n"
"attribute vec3 a_1;\n"
"attribute vec3 a_2;\n"
"attribute vec3 a_3;\n"
"uniform mat4 u_mvp;\n"
"varying vec3 v_radiance;\n"
"varying vec2 v_noiseUVs[2];\n"
"varying vec2 v_uv;\n"
"varying vec3 v_pos;\n"
"varying vec3 v_norm;\n"
"varying float v_blend;\n"
"void main()\n"
"{\n"
	"vec4 p = a_0;\n"
// TODO: tsone: no need to normalize?
	"vec3 n = normalize(a_1);\n"
	"vec3 dl = a_2;\n"

	"v_norm = a_1;\n"
	"v_pos = a_0.xyz;\n"

	// Vertex color contains baked radiance from the TV screen.
	// Radiance from diffuse is red and specular in green component
	// (blue is unused).
	// Gamma decode and modulate by material diffuse and specular.
	"v_radiance = (a_3*a_3) * vec3(0.15, 0.20, 1.0);\n"

// TODO: tsone: precalculate the uvs and blend coeff?
	// Calculate texture coordinate for the downsampled texture sampling.
	// Fake reflection inwards to the center of the TV screen based on
	// distance to the nearest TV screen point/edge (precalculated).
	"float andy = length(dl);\n"
	"float les = length(dl.xy);\n"
	"vec2 clay = (les > 0.0) ? dl.xy / les : vec2(0.0);\n"
	"vec2 nxy = normalize(n.xy);\n"
	"float mixer = max(1.0 - 30.0*les, 0.0);\n"
	"vec2 pool = normalize(mix(clay, nxy, mixer));\n"
	"vec4 tmp = u_mvp * vec4(p.xy + 5.5*vec2(1.0, 7.0/8.0)*(vec2(les) + vec2(0.0014, 0.0019))*pool, p.zw);\n"
	"v_uv = 0.5 + 0.5 * tmp.xy / tmp.w;\n"

// TODO: tsone: duplicated (screen, tv), manual homogenization
	"gl_Position = u_mvp * a_0;\n"
	"gl_Position = vec4(gl_Position.xy / gl_Position.w, 0.0, 1.0);\n"
	"vec2 vwc = gl_Position.xy * 0.5 + 0.5;\n"
	"v_noiseUVs[0] = vec2(SCREEN_W, SCREEN_H) * vwc / vec2(NOISE_W, NOISE_H);\n"
	"v_noiseUVs[1] = 0.707*v_noiseUVs[0] + 0.5 / vec2(NOISE_W, NOISE_H);\n"

	"v_blend = min(0.28*70.0 * andy, 1.0);\n"
"}\n";
static const char* tv_frag_src =
"uniform sampler2D u_downsample1Tex;\n"
"uniform sampler2D u_downsample3Tex;\n"
"uniform sampler2D u_downsample5Tex;\n"
"uniform sampler2D u_noiseTex;\n"
"varying vec3 v_radiance;\n"
"varying vec2 v_noiseUVs[2];\n"
"varying vec3 v_pos;\n"
"varying vec3 v_norm;\n"
"varying vec2 v_uv;\n"
"varying float v_blend;\n"
"void main()\n"
"{\n"
	// Sample noise at low and high frequencies.
	"float noiseLF = texture2D(u_noiseTex, v_noiseUVs[1]).r;\n"
	"float noiseHF = texture2D(u_noiseTex, v_noiseUVs[0]).r;\n"

	// Sample downsampled (blurry) textures and gamma decode.
	"vec3 ds0 = texture2D(u_downsample1Tex, v_uv).rgb;\n"
	"vec3 ds1 = texture2D(u_downsample3Tex, v_uv).rgb;\n"
	"vec3 ds2 = texture2D(u_downsample5Tex, v_uv).rgb;\n"
	"ds0 *= ds0;\n"
	"ds1 *= ds1;\n"
	"ds2 *= ds2;\n"

	"vec3 color;\n"

	// Approximate diffuse and specular by sampling downsampled texture and
	// blending according to the fragment distance from the TV screen (emitter).
	"color = v_radiance.r * mix(ds1, ds2, v_blend);\n"
	"color += v_radiance.g * mix(ds0, ds1, v_blend);\n"

	// Add slight graininess for rough plastic (also to hide interpolation artifacts).
	"float graininess = 0.15625 * v_blend + 0.14578;\n"
	"color *= 1.0 - graininess * sqrt(abs(2.0*noiseLF - 1.0));\n"

// TODO: tsone: As a matter of fact, this is the proper shading calculation...
	"vec3 n = normalize(v_norm);\n"
	"float shade = shadeBlinn(v_pos, n);\n"

// TODO: tsone: tone-mapping?

// TODO: tsone: vignette?
//	"color = vec3(0.0);\n"
//	"vec2 nuv = 2.0*v_uv - 1.0;\n"
//	"float vignette = max(1.0 - length(nuv), 0.0);\n"
//	"vec3 lightColor = vec3(v_shade);\n"

	// Gamma encode color w/ sqrt().
	"gl_FragColor = vec4(sqrt(color + shade) + (1.5/128.0) * (noiseHF-0.5), 1.0);\n"
"}\n";

// Downsample shader.
static const char* downsample_vert_src =
"uniform vec2 u_offsets[8];\n"
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"varying vec2 v_uv[8];\n"
"void main()\n"
"{\n"
	"gl_Position = a_0;\n"
	"for (int i = 0; i < 8; i++) {\n"
		"v_uv[i] = a_2 + u_offsets[i];\n"
	"}\n"
"}\n";
static const char* downsample_frag_src =
"uniform sampler2D u_downsampleTex;\n"
"uniform float u_weights[8];\n"
"varying vec2 v_uv[8];\n"
"void main()\n"
"{\n"
	"vec3 result = vec3(0.0);\n"
	"for (int i = 0; i < 8; i++) {\n"
		"vec3 color = texture2D(u_downsampleTex, v_uv[i]).rgb;\n"
		"result += u_weights[i] * color*color;\n"
	"}\n"
	"gl_FragColor = vec4(sqrt(result), 1.0);\n"
"}\n";

// Combine shader.
static const char* combine_vert_src =
DEFINE(NOISE_W)
DEFINE(NOISE_H)
// TODO: tsone: these are wrong, proper size must be from a uniform
DEFINE(SCREEN_W)
DEFINE(SCREEN_H)
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"varying vec2 v_uv;\n"
"varying vec2 v_noiseUV;\n"
"void main()\n"
"{\n"
	"gl_Position = a_0;\n"
	"v_uv = a_2;\n"
	"v_noiseUV = (vec2(SCREEN_W, SCREEN_H) / vec2(NOISE_W, NOISE_H)) * a_2;\n"
"}\n";
static const char* combine_frag_src =
"uniform sampler2D u_tvTex;\n"
"uniform sampler2D u_downsample3Tex;\n"
"uniform sampler2D u_downsample5Tex;\n"
"uniform sampler2D u_noiseTex;\n"
"uniform vec3 u_glow;\n"
"varying vec2 v_uv;\n"
"varying vec2 v_noiseUV;\n"
"void main()\n"
"{\n"
	// Sample screen/tv and downsampled (blurry) textures for glow.
	"vec3 color = texture2D(u_tvTex, v_uv).rgb;\n"
	"vec3 ds3 = texture2D(u_downsample3Tex, v_uv).rgb;\n"
	"vec3 ds5 = texture2D(u_downsample5Tex, v_uv).rgb;\n"
	// Linearize color values.
	"color *= color;\n"
	"ds3 *= ds3;\n"
	"ds5 *= ds5;\n"
	// Blend in glow as blurry highlight allowing slight bleeding on higher u_glow values.
	"color = (color + u_glow[0]*ds3 + u_glow[1]*ds5) / (1.0 + u_glow[1]);\n"
//	"float noise = u_glow * (u_mouse.x/256.0) * (1.5/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"
//	"float noise = u_glow[2] * (u_mouse.x/256.0) * (8.0/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"
	"float noise = u_glow[2] * (6.0/128.0) * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"
	// Gamma encode w/ sqrt() to something similar to sRGB space.
	"gl_FragColor = vec4(sqrt(color) + noise, 1.0);\n"
"}\n";

static const char* direct_vert_src =
"attribute vec4 a_0;\n"
"attribute vec2 a_2;\n"
"varying vec2 v_uv;\n"
"void main()\n"
"{\n"
	"gl_Position = a_0;\n"
        "v_uv = a_2;\n"
"}\n";
static const char* direct_frag_src =
"uniform sampler2D u_tex;\n"
"varying vec2 v_uv;\n"
"void main()\n"
"{\n"
	"gl_FragColor = texture2D(u_tex, v_uv);\n"
"}\n";

