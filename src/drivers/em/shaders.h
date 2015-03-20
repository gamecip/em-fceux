#if DBG_MODE
#define DBG_PROLOG() \
"uniform vec3 u_mouse;\n"
#else
#define DBG_PROLOG()
#endif
static const char* rgb_vert_src =
"precision highp float;\n"
DBG_PROLOG()
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
"void main() {\n"
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
static const char* rgb_frag_src =
"precision highp float;\n"
DBG_PROLOG()
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
"uniform float u_rgbppu;\n"
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

"void main(void) {\n"
    "float deemp = 64.0 * (255.0/511.0) * texture2D(u_deempTex, v_deemp_uv).r;\n"
    "float subp = mod(floor(NUM_SUBPS*IDX_W * v_uv[int(NUM_TAPS)/2].x), NUM_SUBPS);\n"
    "vec2 p;\n"
    "vec2 la;\n"
    "vec2 uv;\n"
    "vec3 yiq = vec3(0.0);\n"
    "SMP(0);\n"
    "SMP(1);\n"
    "SMP(2);\n"
    // Snatch in RGB PPU; uv.x is already calculated, so just read from lookup tex with u=1.0.
    "vec3 rgbppu = RESCALE(texture2D(u_lookupTex, vec2(1.0, uv.y)).rgb);\n"
    "SMP(3);\n"
    "SMP(4);\n"
    // Working multiplier for filtered chroma to match PPU is 2/5 (for CW2=12).
    // Is this because color fringing with composite?
    "yiq *= (8.0/2.0) / vec3(YW2, CW2-2.0, CW2-2.0);\n"
    "yiq = mix(yiq, rgbppu, u_rgbppu);\n"
    "yiq.r += u_noiseAmp * (texture2D(u_noiseTex, v_noiseUV).r - 0.5);\n"
    "yiq.gb *= u_color;\n"
    "vec3 result = clamp(c_convMat * yiq, 0.0, 1.0);\n"
    // Gamma convert RGB from NTSC space to space similar to SRGB.
    "result = pow(result, vec3(u_gamma));\n"
    // NOTE: While this seems to be wrong (after gamma), it works well in practice...?
    "result = clamp(u_contrast * result + u_brightness, 0.0, 1.0);\n"
    // Write linear color (banding is not visible for pixelated graphics).
// TODO: tsone: testing encoding everywhere
    "gl_FragColor = vec4(result, 1.0);\n"
//    "gl_FragColor = vec4(result * result, 1.0);\n"
"}\n";

static const char* stretch_vert_src =
    DEFINE(IDX_H)
    "precision highp float;\n"
DBG_PROLOG()
    "attribute vec4 a_0;\n"
    "attribute vec2 a_2;\n"
    "varying vec2 v_uv[2];\n"
    "void main() {\n"
    "vec2 uv = a_2;\n"
    "v_uv[0] = vec2(uv.x, 1.0 - uv.y);\n"
    "v_uv[1] = vec2(v_uv[0].x, v_uv[0].y - 0.25/IDX_H);\n"
    "gl_Position = a_0;\n"
    "}\n";
static const char* stretch_frag_src =
    "precision highp float;\n"
DBG_PROLOG()
    DEFINE(IDX_H)
    DEFINE(M_PI)
    "uniform float u_scanlines;\n"
    "uniform sampler2D u_rgbTex;\n"
    "varying vec2 v_uv[2];\n"
    "void main(void) {\n"
    // Sample adjacent scanlines and average to smoothen slightly vertically.
    "vec3 c0 = texture2D(u_rgbTex, v_uv[0]).rgb;\n"
    "vec3 c1 = texture2D(u_rgbTex, v_uv[1]).rgb;\n"
// TODO: tsone: testing encoding everywhere
    "vec3 color = 0.5 * (c0*c0 + c1*c1);\n"
//    "vec3 color = 0.5 * (c0 + c1);\n"
    // Use oscillator as scanline modulator.
    "float scanlines = u_scanlines * (1.0 - abs(sin(M_PI*IDX_H * v_uv[0].y - M_PI*0.125)));\n"
    // This formula dims dark colors, but keeps brights.
// TODO: tsone: testing encoding everywhere
    "gl_FragColor = vec4(sqrt(mix(color, max(2.0*color - 1.0, 0.0), scanlines)), 1.0);\n"
//    "gl_FragColor = vec4(mix(color, max(2.0*color - 1.0, 0.0), scanlines), 1.0);\n"
    "}\n";

static const char* screen_vert_src =
    "precision highp float;\n"
DBG_PROLOG()
    DEFINE(RGB_W)
    DEFINE(M_PI)
    "attribute vec4 a_0;\n"
    "attribute vec3 a_1;\n"
    "attribute vec2 a_2;\n"
    "uniform float u_convergence;\n"
    "uniform mat4 u_mvp;\n"
    "uniform vec2 u_uvScale;\n"
    "varying vec2 v_uv[5];\n"
    "varying vec3 v_color;\n"
    "varying vec3 v_norm;\n"
    "varying vec3 v_pos;\n"

    "#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0)\n"
    "void main() {\n"
    "vec2 uv = 0.5 + u_uvScale.xy * (a_2 - 0.5);\n"
    "TAP(0,-4.0);\n"
    "TAP(1, u_convergence);\n"
    "TAP(2, 0.0);\n"
    "TAP(3,-u_convergence);\n"
    "TAP(4, 4.0);\n"
// TODO: tsone: have some light on screen or not?
#if 1
    "vec3 view_pos = vec3(0.0, 0.0, 2.5);\n"
    "vec3 light_pos = vec3(-1.0, 6.0, 3.0);\n"
    "vec3 n = normalize(a_1);\n"
    "v_norm = a_1;\n"
    "v_pos = a_0.xyz;\n"
    "vec3 v = normalize(view_pos - a_0.xyz);\n"
    "vec3 p2l = light_pos - a_0.xyz;\n"
    "float p2lL = length(p2l);\n"
    "vec3 l = p2l / p2lL;\n"
    "vec3 h = normalize(l + v);\n"
    "float ndotl = max(dot(n, l), 0.0);\n"
    "float ndoth = max(dot(n, h), 0.0);\n"
    "float ndotv = max(dot(n, v), 0.0);\n"
    "float Cd = 0.5;\n"
    "float Cs = 0.5;\n"
    "float m = 11.0;\n"
    "float Cd0 = Cd / M_PI;\n"
    "float Cs0 = (Cs * (m + 8.0) / (8.0 * M_PI)) * pow(ndoth, m);\n"
    "float R = 10.0;\n"
    "float El = R / (1.0 + dot(p2l, p2l));\n"
//    "float Rf0 = 0.03;\n"
//    "float Rf = Rf0 + (1.0-Rf0) * pow(1.0-ndotl, 5.0);\n"
//    "float Cd2 = (21.0/(20.0*M_PI * (1.0-Rf0))) * (1.0-pow(1.0-ndotl, 5.0)) * (1.0-pow(1.0-ndotv, 5.0));\n"
//    "v_color = vec3(0.006*ndotl + 0.04*pow(ndoth, e));\n"
    "v_color = vec3((Cd0 + Cs0) * El*ndotl);\n"
#else
// TODO: tsone: This color must be linearized
    "v_color = vec3(0.0);\n"
#endif
    "gl_Position = u_mvp * a_0;\n"
    "}\n";
static const char* screen_frag_src =
"precision highp float;\n"
DBG_PROLOG()
DEFINE(M_PI)
"uniform sampler2D u_stretchTex;\n"
// TODO: tsone: for debug
"uniform float u_convergence;\n"
"uniform vec3 u_sharpenKernel[5];\n"
"varying vec2 v_uv[5];\n"
"varying vec3 v_color;\n"
"varying vec3 v_norm;\n"
"varying vec3 v_pos;\n"

//
// From 'Improved texture interpolation' by Inigo Quilez (2009):
// http://iquilezles.org/www/articles/texture/texture.htm
//
// Modifies linear interpolation by applying a smoothstep function to
// texture image coordinate fractional part. This causes derivatives of
// the interpolation slope to be zero, avoiding discontinuities.
// Quintic smoothstep makes 1st & 2nd order derivates zero, and regular
// (cubic) smoothstep makes only 1st order derivate zero.
//
// Not only it is useful for normal and bump map texture intepolation,
// it also gives appearance slightly similar to nearest interpolation,
// but smoother. This is useful for surfaces that should appear "pixelated",
// for example LCD and TV screens.
//
"vec3 texture2DSmooth( const sampler2D tex, const vec2 uv, const vec2 texResolution ) {\n"
    "vec2 p = uv * texResolution + 0.5;\n"
    "vec2 i = floor( p );\n"
    "vec2 f = p - i;\n"
    "f = ( ( 6.0 * f - 15.0 ) * f + 10.0 ) * f*f*f;\n" // Quintic smoothstep.
//    "f = ( 3.0 - 2.0 * f ) * f*f;\n" // Cubic smoothstep.
    "p = ( i + f - 0.5 ) / texResolution;\n"
    "return texture2D( tex, p ).rgb;\n"
"}\n"

// TODO: tsone: duplicate code (screen & tv)
//
// Optimized cubic texture interpolation. Uses 4 texture samples for the 4x4 texel area.
//
// http://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl
//
"vec4 cubic( const float v ) {\n"
    "vec4 n = vec4( 1.0, 2.0, 3.0, 4.0 ) - v;\n"
    "vec4 s = n * n * n;\n"
    "float x = s.x;\n"
    "float y = s.y - 4.0 * s.x;\n"
    "float z = s.z - 4.0 * s.y + 6.0 * s.x;\n"
    "float w = 6.0 - x - y - z;\n"
    "return vec4( x, y, z, w );\n"
"}\n"
"vec3 texture2DCubic( const sampler2D tex, in vec2 uv, const vec2 reso ) {\n"
    "uv = uv * reso - 0.5;\n"
    "float fx = fract( uv.x );\n"
    "float fy = fract( uv.y );\n"
    "uv.x -= fx;\n"
    "uv.y -= fy;\n"
    "vec4 xcub = cubic( fx );\n"
    "vec4 ycub = cubic( fy );\n"
    "vec4 c = uv.xxyy + vec4( - 0.5, 1.5, - 0.5, 1.5 );\n"
    "vec4 s = vec4( xcub.xz, ycub.xz ) + vec4( xcub.yw, ycub.yw );\n"
    "vec4 offs = c + vec4( xcub.yw, ycub.yw ) / s;\n"
    "vec3 s0 = texture2D( tex, offs.xz / reso ).rgb;\n"
    "vec3 s1 = texture2D( tex, offs.yz / reso ).rgb;\n"
    "vec3 s2 = texture2D( tex, offs.xw / reso ).rgb;\n"
    "vec3 s3 = texture2D( tex, offs.yw / reso ).rgb;\n"
    "float sx = s.x / (s.x + s.y);\n"
    "float sy = s.z / (s.z + s.w);\n"
    "return mix( mix( s3, s2, sx ), mix( s1, s0, sx ), sy );\n"
"}\n"

"void main(void) {\n"
    "vec3 color = vec3(0.0);\n"
    "for (int i = 0; i < 5; i++) {\n"
        "vec3 tmp = texture2D(u_stretchTex, v_uv[i]).rgb;\n"
        "color += u_sharpenKernel[i] * tmp*tmp;\n"
    "}\n"
    "color = clamp(color, 0.0, 1.0);\n"
// TODO: tsone: test adding dust scatter
    "vec3 n = normalize(v_norm);\n"
    "color += 0.018*texture2D(u_stretchTex, v_uv[2] - 0.021*n.xy).rgb;\n"

#if 1
    "const vec3 view_pos = vec3(0.0, 0.0, 2.5);\n"
    "const vec3 light_pos = vec3(-1.0, 6.0, 3.0);\n"
    "vec3 p = v_pos;\n"
    "vec3 v = normalize(view_pos - p);\n"
    "vec3 p2l = light_pos - p;\n"
    "float p2lL = length(p2l);\n"
    "vec3 l = p2l / p2lL;\n"
    "vec3 h = normalize(l + v);\n"
    "float ndotl = max(dot(n, l), 0.0);\n"
    "float ndoth = max(dot(n, h), 0.0);\n"
    "float ndotv = max(dot(n, v), 0.0);\n"
    "float albedo = 0.13;\n"
    "float Cd = albedo;\n"
    "float Cs = 0.0;\n"
    "float m = 11.0;\n"
    "float Cd0 = Cd / M_PI;\n"
    "float Cs0 = (Cs * (m + 8.0) / (8.0 * M_PI)) * pow(ndoth, m);\n"
    "float R = 10.0;\n"
    "float El = R / (dot(p2l, p2l));\n"
//    "float Rf0 = 0.03;\n"
//    "float Rf = Rf0 + (1.0-Rf0) * pow(1.0-ndotl, 5.0);\n"
//    "float Cd2 = (21.0/(20.0*M_PI * (1.0-Rf0))) * (1.0-pow(1.0-ndotl, 5.0)) * (1.0-pow(1.0-ndotv, 5.0));\n"
//    "color += vec3((Cd0 + Cs0) * El*ndotl);\n"
#else
    "vec3 n = normalize(v_norm);\n"
//    "vec3 p = v_pos;\n"
//    "color = v_color;\n"
//    "color = vec3(0.0, u_convergence/2.0+p.z-n.y, 0.0);\n"
    "color = vec3(n.y, -n.y, 0.0);\n"
#endif

    // Use gamma encoding as we may output smooth color changes here.
    "gl_FragColor = vec4(sqrt(color), 1.0);\n"
"}\n";

static const char* tv_vert_src =
    "precision highp float;\n"
DEFINE(NOISE_W)
DEFINE(NOISE_H)
DBG_PROLOG()
    "attribute vec4 a_0;\n"
    "attribute vec3 a_1;\n"
    "attribute vec3 a_2;\n"
    "attribute vec3 a_3;\n"
    "uniform mat4 u_mvp;\n"
    "varying vec3 v_radiance;\n"
    "varying vec2 v_noiseUVs[2];\n"
    "varying vec2 v_uv;\n"
    "varying float v_blend;\n"

    "void main() {\n"
        "vec4 p = a_0;\n"
        "vec4 tp = u_mvp * p;\n"
        "gl_Position = tp;\n"

        "vec3 n = normalize(a_1);\n"
        "vec3 dl = a_2;\n"

// TODO: tsone: lighting from ceiling lamp
#if 0
        "vec3 view_pos = vec3(0.0, 0.0, 2.5);\n"
        "vec3 light_pos = vec3(-1.0, 6.0, 3.0);\n"
        "vec3 v = normalize(view_pos - a_0.xyz);\n"
        "vec3 l = normalize(light_pos - a_0.xyz);\n"
        "vec3 h = normalize(l + v);\n"
        "float ndotl = max(dot(n, l), 0.0);\n"
        "float ndoth = max(dot(n, h), 0.0);\n"
        "v_color = vec3(0.000 + 0.004*ndotl + 0.00*pow(ndoth, 19.0));\n"
#endif

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

// TODO: tsone: use output resolution appropriately
        "v_noiseUVs[0] = 0.5 * vec2(1120.0/NOISE_W, 896.0/NOISE_H) * (tp.xy/tp.w);\n"
        "v_noiseUVs[1] = 0.5 * v_noiseUVs[0];\n"

        "v_blend = min(0.28*70.0 * andy, 1.0);\n"
    "}\n";
static const char* tv_frag_src =
    "precision highp float;\n"
DBG_PROLOG()
    DEFINE(M_PI)
    "uniform sampler2D u_downsample1Tex;\n"
    "uniform sampler2D u_downsample3Tex;\n"
    "uniform sampler2D u_downsample5Tex;\n"
    "uniform sampler2D u_noiseTex;\n"
    "varying vec3 v_radiance;\n"
    "varying vec2 v_noiseUVs[2];\n"
    "varying vec2 v_uv;\n"
    "varying float v_blend;\n"

#if 0
// TODO: tsone: duplicate code (screen & tv)
//
// Optimized cubic texture interpolation. Uses 4 texture samples for the 4x4 texel area.
//
// http://stackoverflow.com/questions/13501081/efficient-bicubic-filtering-code-in-glsl
//
"vec4 cubic( const float v ) {\n"
    "vec4 n = vec4( 1.0, 2.0, 3.0, 4.0 ) - v;\n"
    "vec4 s = n * n * n;\n"
    "float x = s.x;\n"
    "float y = s.y - 4.0 * s.x;\n"
    "float z = s.z - 4.0 * s.y + 6.0 * s.x;\n"
    "float w = 6.0 - x - y - z;\n"
    "return vec4( x, y, z, w );\n"
"}\n"
"vec3 texture2DCubic( const sampler2D tex, in vec2 uv, const vec2 reso ) {\n"
    "uv = uv * reso - 0.5;\n"
    "float fx = fract( uv.x );\n"
    "float fy = fract( uv.y );\n"
    "uv.x -= fx;\n"
    "uv.y -= fy;\n"
    "vec4 xcub = cubic( fx );\n"
    "vec4 ycub = cubic( fy );\n"
    "vec4 c = uv.xxyy + vec4( - 0.5, 1.5, - 0.5, 1.5 );\n"
    "vec4 s = vec4( xcub.xz, ycub.xz ) + vec4( xcub.yw, ycub.yw );\n"
    "vec4 offs = c + vec4( xcub.yw, ycub.yw ) / s;\n"
    "vec3 s0 = texture2D( tex, offs.xz / reso ).rgb;\n"
    "vec3 s1 = texture2D( tex, offs.yz / reso ).rgb;\n"
    "vec3 s2 = texture2D( tex, offs.xw / reso ).rgb;\n"
    "vec3 s3 = texture2D( tex, offs.yw / reso ).rgb;\n"
    "float sx = s.x / (s.x + s.y);\n"
    "float sy = s.z / (s.z + s.w);\n"
    "return mix( mix( s3, s2, sx ), mix( s1, s0, sx ), sy );\n"
"}\n"

    "const vec2 c_uvMult = vec2(0.892, 0.827);\n"
    "const vec3 c_center = vec3(0.0, 0.000135, -0.04427);\n"
    "const vec3 c_size = vec3(0.95584, 0.703985, 0.04986);\n"
//    "const vec3 c_size = vec3(0.95584-0.011, 0.703985-0.011, 0.04986);\n"
#endif

"void main(void) {\n"
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

    // Add noise for rough plastic (and to hide interpolation artifacts).
    "float noise = 2.0*0.4336 * (mix(noiseLF, noiseHF, 0.7366 * v_blend) - 0.5);\n"
    "color *= 1.0 + noise;\n"

// TODO: tsone: tone-mapping?

// TODO: tsone: vignette?
//    "color = vec3(0.0);\n"
//    "vec2 nuv = 2.0*v_uv - 1.0;\n"
//    "float vignette = max(1.0 - length(nuv), 0.0);\n"

    // Gamma encode color w/ sqrt().
    "gl_FragColor = vec4(sqrt(color), 1.0);\n"
"}\n";

// Downsample shader.
static const char* downsample_vert_src =
    "precision highp float;\n"
DBG_PROLOG()
    "uniform vec2 u_offsets[8];\n"
    "attribute vec4 a_0;\n"
    "attribute vec2 a_2;\n"
    "varying vec2 v_uv[8];\n"

    "void main() {\n"
    "gl_Position = a_0;\n"
    "for (int i = 0; i < 8; i++) {\n"
        "v_uv[i] = a_2 + u_offsets[i];\n"
    "}\n"
    "}\n";
static const char* downsample_frag_src =
    "precision highp float;\n"
DBG_PROLOG()
    "uniform sampler2D u_downsampleTex;\n"
    "uniform float u_weights[8];\n"
    "varying vec2 v_uv[8];\n"

    "void main(void) {\n"
    "vec3 result = vec3(0.0);\n"
    "for (int i = 0; i < 8; i++) {\n"
        "vec3 color = texture2D(u_downsampleTex, v_uv[i]).rgb;\n"
        "result += u_weights[i] * color*color;\n"
    "}\n"
    "gl_FragColor = vec4(sqrt(result), 1.0);\n"
    "}\n";

// Combine shader.
static const char* combine_vert_src =
    "precision highp float;\n"
DBG_PROLOG()
    "attribute vec4 a_0;\n"
    "attribute vec2 a_2;\n"
    "varying vec2 v_uv;\n"
    "void main() {\n"
        "gl_Position = a_0;\n"
        "v_uv = a_2;\n"
    "}\n";
static const char* combine_frag_src =
    "precision highp float;\n"
DBG_PROLOG()
    "uniform sampler2D u_tvTex;\n"
    "uniform sampler2D u_downsample3Tex;\n"
    "uniform sampler2D u_downsample5Tex;\n"
    "uniform float u_glow;\n"
    "varying vec2 v_uv;\n"

    "void main(void) {\n"
        // Sample screen/tv and downsampled (blurry) textures for glow.
        "vec3 color = texture2D(u_tvTex, v_uv).rgb;\n"
        "vec3 ds3 = texture2D(u_downsample3Tex, v_uv).rgb;\n"
        "vec3 ds5 = texture2D(u_downsample5Tex, v_uv).rgb;\n"
        // Linearize color values.
        "color *= color;\n"
        "ds3 *= ds3;\n"
        "ds5 *= ds5;\n"
        // Blend in glow as blurry highlight allowing slight bleeding on higher u_glow values.
        "float g2 = u_glow * u_glow;\n"
        "color = (color + u_glow*ds3 + g2*ds5) / (1.0 + g2);\n"
        // Gamma encode w/ sqrt() to something similar to sRGB space.
        "gl_FragColor = vec4(sqrt(color), 1.0);\n"
    "}\n";
