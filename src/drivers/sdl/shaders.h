
static const char* rgb_vert_src =
    "precision highp float;\n"
    DEFINE(NUM_TAPS)
    DEFINE(IDX_W)
    "attribute vec2 a_vert;\n"
    "varying vec2 v_uv[int(NUM_TAPS)];\n"
    "varying vec2 v_deemp_uv;\n"
    "#define UV_OUT(i_, o_) v_uv[i_] = vec2(uv.x + (o_)/IDX_W, uv.y)\n"
    "void main() {\n"
    "vec2 uv = 0.5 + vec2(0.5, 0.5*240.0/256.0) * sign(a_vert);\n"
    "UV_OUT(0,-2.0);\n"
    "UV_OUT(1,-1.0);\n"
    "UV_OUT(2, 0.0);\n"
    "UV_OUT(3, 1.0);\n"
    "UV_OUT(4, 2.0);\n"
    "v_deemp_uv = vec2(uv.y, 0.0);\n"
    "gl_Position = vec4(a_vert, 0.0, 1.0);\n"
    "}\n";
static const char* rgb_frag_src =
    "precision highp float;\n"
    DEFINE(NUM_SUBPS)
    DEFINE(NUM_TAPS)
    DEFINE(LOOKUP_W)
    DEFINE(IDX_W)
    DEFINE(YW2)
    DEFINE(CW2)
    "uniform sampler2D u_idxTex;\n"
    "uniform sampler2D u_deempTex;\n"
    "uniform sampler2D u_lookupTex;\n"
    "uniform vec3 u_mins;\n"
    "uniform vec3 u_maxs;\n"
    "uniform float u_brightness;\n"
    "uniform float u_contrast;\n"
    "uniform float u_color;\n"
    "uniform float u_gamma;\n"
    "uniform float u_rgbppu;\n"
    "varying vec2 v_uv[int(NUM_TAPS)];\n"
    "varying vec2 v_deemp_uv;\n"
    "const mat3 c_convMat = mat3(\n"
    "    1.0,        1.0,        1.0,       // Y\n"
    "    0.946882,   -0.274788,  -1.108545, // I\n"
    "    0.623557,   -0.635691,  1.709007   // Q\n"
    ");\n"
    "#define P(i_)  p = floor(IDX_W * v_uv[i_])\n"
    "#define U(i_)  (mod(p.x - p.y, 3.0)*NUM_SUBPS*NUM_TAPS + subp*NUM_TAPS + float(i_)) / (LOOKUP_W-1.0)\n"
    "#define V(i_)  ((255.0/511.0) * texture2D(u_idxTex, v_uv[i_]).r + deemp)\n"
    "#define UV(i_) uv = vec2(U(i_), V(i_))\n"
    "#define RESCALE(v_) ((v_) * (u_maxs-u_mins) + u_mins)\n"
    "#define SMP(i_) P(i_); UV(i_); yiq += RESCALE(texture2D(u_lookupTex, uv).rgb)\n"
    "\n"
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
// TODO: Working multiplier for filtered chroma to match PPU is 2/5 (for CW2=12).
// TODO: Is this because colors are blended together with composite?
    "yiq *= (8.0/2.0) / vec3(YW2, CW2-2.0, CW2-2.0);\n"
    "yiq = mix(yiq, rgbppu, u_rgbppu);\n"
    "yiq.gb *= u_color;\n"
    "vec3 result = c_convMat * yiq;\n"
    "gl_FragColor = vec4(u_contrast * pow(result, vec3(u_gamma)) + u_brightness, 1.0);\n"
    "}\n";

static const char* stretch_vert_src =
    "precision highp float;\n"
    "attribute vec2 a_vert;\n"
    "varying vec2 v_uv[2];\n"
    "void main() {\n"
    "vec2 uv = 0.5 + vec2(0.5, 0.5*240.0/256.0) * sign(a_vert);\n"
    "v_uv[0] = vec2(uv.x, 1.0 - uv.y);\n"
    "v_uv[1] = vec2(v_uv[0].x, v_uv[0].y - 0.25/256.0);\n"
    "gl_Position = vec4(a_vert, 0.0, 1.0);\n"
    "}\n";
static const char* stretch_frag_src =
    "precision highp float;\n"
    DEFINE(M_PI)
    "uniform float u_scanline;\n"
    "uniform sampler2D u_rgbTex;\n"
    "varying vec2 v_uv[2];\n"
    "void main(void) {\n"
    "vec3 color = 0.5 * (texture2D(u_rgbTex, v_uv[0]).rgb + texture2D(u_rgbTex, v_uv[1]).rgb);\n"
    "float scanline = u_scanline - u_scanline * abs(sin(M_PI*256.0 * v_uv[0].y - M_PI*0.125));\n"
    "gl_FragColor = vec4((color - scanline) * (1.0+scanline), 1.0);\n"
    "}\n";

static const char* disp_vert_src =
    "precision highp float;\n"
    DEFINE(RGB_W)
    DEFINE(M_PI)
    "attribute vec4 a_vert;\n"
    "attribute vec3 a_norm;\n"
    "attribute vec2 a_uv;\n"
    "uniform float u_convergence;\n"
    "uniform mat4 u_mvp;\n"
    "varying vec2 v_uv[5];\n"
    "varying vec3 v_color;\n"
    "#define TAP(i_, o_) v_uv[i_] = uv + vec2((o_) / RGB_W, 0.0)\n"
    "void main() {\n"
    "vec2 uv = vec2(1.0/280.0, 22.0/256.0) + vec2(278.0/280.0, 228.0/256.0)*a_uv;\n"
    "TAP(0,-4.0);\n"
    "TAP(1,-u_convergence);\n"
    "TAP(2, 0.0);\n"
    "TAP(3, u_convergence);\n"
    "TAP(4, 4.0);\n"
    "vec3 view_pos = vec3(0.0, 0.0, 2.5);\n"
    "vec3 light_pos = vec3(-1.0, 6.0, 3.0);\n"
    "vec3 n = normalize(a_norm);\n"
    "vec3 v = normalize(view_pos - a_vert.xyz);\n"
    "vec3 l = normalize(light_pos - a_vert.xyz);\n"
    "vec3 h = normalize(l + v);\n"
    "float ndotl = max(dot(n, l), 0.0);\n"
    "float ndoth = max(dot(n, h), 0.0);\n"
    "v_color = vec3(0.02*ndotl + 0.23*pow(ndoth, 25.0));\n"
    "gl_Position = u_mvp * a_vert;\n"
    "}\n";
static const char* disp_frag_src =
    "precision highp float;\n"
    "uniform sampler2D u_stretchTex;\n"
    "uniform mat3 u_sharpenKernel;\n"
    "varying vec2 v_uv[5];\n"
    "varying vec3 v_color;\n"
    "void main(void) {\n"
#if 1
    "#define SMP(i_, m_) color += (m_) * texture2D(u_stretchTex, v_uv[i_]).rgb\n"
    "vec3 color = vec3(0.0);\n"
    "SMP(0, u_sharpenKernel[0]);\n"
    "SMP(1, vec3(1.0, 0.0, 0.0));\n"
    "SMP(2, u_sharpenKernel[1]);\n"
    "SMP(3, vec3(0.0, 0.0, 1.0));\n"
    "SMP(4, u_sharpenKernel[2]);\n"
    "gl_FragColor = vec4(v_color + color, 1.0);\n"
#else
//        "gl_FragColor = texture2D(u_stretchTex, v_uv[2]);\n"
    "gl_FragColor = vec4(vec3(v_uv[2], 1.0), 1.0);\n"
#endif
    "}\n";

static const char* tv_vert_src =
    "precision highp float;\n"
    "attribute vec4 a_vert;\n"
    "attribute vec3 a_norm;\n"
    "uniform mat4 u_mvp;\n"
    "varying vec3 v_color;\n"
    "varying vec3 v_p;\n"
    "varying vec3 v_n;\n"
    "varying vec3 v_v;\n"

    "varying vec2 v_uv;\n"
    "varying float v_bias;\n"
    "varying float v_emis;\n"

    "const vec3 c_center = vec3(0.0, 0.00056, -0.0932);\n"
    "const vec3 c_size = vec3(0.956, 0.704, 0.0);\n"

    "void main() {\n"
    "vec3 view_pos = vec3(0.0, 0.0, 2.5);\n"
    "vec3 light_pos = vec3(-1.0, 6.0, 3.0);\n"
    "vec4 p = u_mvp * a_vert;\n"
    "vec3 n = normalize(a_norm);\n"
    "vec3 v = normalize(view_pos - a_vert.xyz);\n"
    "vec3 l = normalize(light_pos - a_vert.xyz);\n"
    "vec3 h = normalize(l + v);\n"
    "float ndotl = max(dot(n, l), 0.0);\n"
    "float ndoth = max(dot(n, h), 0.0);\n"
    "v_color = vec3(0.007 + 0.05*ndotl + 0.03*pow(ndoth, 19.0));\n"
    "v_n = n;\n"
    "v_v = -v;\n"
    "v_p = a_vert.xyz;\n"
    "gl_Position = p;\n"

    "vec3 vc = a_vert.xyz - c_center;\n"
    "vec3 q = max(abs(vc) - (c_size - vec3(0.05, 0.05, 0.0)), 0.0);\n"
    "float dxy = max(length(q.xy) - 0.05, 0.0);\n"
    "float dxyz = length(q);\n"
    "float erp = max(1.0 - 7.5*dxy, 0.0);\n"
    "v_uv = 0.5 + 0.5 * erp*erp * vec2(0.895, 0.825) * (vc.xy/c_size.xy);\n"
    "v_bias = 4.2 + 8.0*11.0 * dxy;\n"
    "vec3 p0 = normalize(c_center + vec3( c_size.x,  c_size.y, 0.0) - a_vert.xyz);\n"
    "vec3 p1 = normalize(c_center + vec3( c_size.x, -c_size.y, 0.0) - a_vert.xyz);\n"
    "vec3 p2 = normalize(c_center + vec3(-c_size.x,  c_size.y, 0.0) - a_vert.xyz);\n"
    "vec3 p3 = normalize(c_center + vec3(-c_size.x, -c_size.y, 0.0) - a_vert.xyz);\n"
    "float vola = max(max(max(dot(n, p0), dot(n, p1)), max(dot(n, p2), dot(n, p3))), 0.0);\n"
    "v_emis = min(0.0053 * vola*vola / (dxyz*dxyz), 1.0);\n"

    "}\n";
static const char* tv_frag_src =
    "precision highp float;\n"
    "uniform sampler2D u_stretchTex;\n"
    "varying vec3 v_color;\n"
    "varying vec3 v_p;\n"
    "varying vec3 v_n;\n"
    "varying vec3 v_v;\n"

    "varying vec2 v_uv;\n"
    "varying float v_bias;\n"
    "varying float v_emis;\n"

    "const vec3 c_center = vec3(0.0, 0.00056, -0.0932);\n"
//    "const vec3 c_size = vec3(0.97*0.956, 0.94*0.704, 0.0);\n"
    "const vec3 c_size = vec3(0.956, 0.704, 0.0);\n"
//    "const vec3 c_size = vec3(0.94531-0.00, 0.69515-0.05, 0.0);\n"
    "void main(void) {\n"
    "vec3 color;\n"
#if 0
    "vec3 r = reflect(normalize(v_v), normalize(vec3(1.0,1.0,0.4)*v_n));\n"
    "if (r.z <= 0.0) color = vec3(1.0);\n"
    "else color = vec3(0.3);\n"
#elif 0
//    "vec3 r = reflect(normalize(v_v), normalize(vec3(1.0,1.0,0.4)*v_n));\n"
    "vec3 v = v_p - c_center;\n"
//    "vec3 q = max(abs(v) - c_size, 0.0);\n"
    "vec3 q = max(abs(v) - (c_size - 0.05), 0.0);\n"
    "float dxy = max(length(q.xy) - 0.05, 0.0);\n"
//    "float dxy = length(q.xy);\n"
    "float dxyz = max(dxy, q.z);\n"
//    "vec3 p = -normalize(v_p);\n"
//    "vec3 r = normalize(v_n + 0.0*p);\n"
//    "float t = (c_center.z - v_p.z) / r.z;\n"
//    "vec2 uv = (0.5 + 0.5 * vec2(1.0, 1.2) * (v_p.xy + t*r.xy));\n"
    "float erp = max(1.0 - 9.0*dxy, 0.0);\n"
//    "vec2 uv = 0.5 + 0.5 * erp*erp * vec2(0.898, 0.818) * (v.xy/c_size.xy);\n"
    "vec2 uv = 0.5 + 0.5 * erp*erp * vec2(0.9, 0.825) * (v.xy/c_size.xy);\n"
    "vec3 mirror = texture2D(u_stretchTex, uv, 4.0 + 4.0*10.0*dxy).rgb;\n"
//    "mirror = vec3(uv, 1.0);\n"
//    "mirror = vec3(8.0*dxy);\n"
//    "if (r.z > 0.0) mirror = vec3(0.0);\n"
    "color = mirror;// * min(0.008 / (dxyz*dxyz), 1.0);\n"
#elif 1
    "color = v_color + v_emis * texture2D(u_stretchTex, v_uv, v_bias).rgb;\n"
#else
    "vec3 n = normalize(v_n);\n"
    "vec3 d = -normalize(v_p);\n"
    "float nDotD = max(dot(n, d), 0.0);\n"
    "float atten = 0.01 / dot(v_p, v_p);\n"
    "color = vec3(nDotD, atten, 0.2);\n"
#endif
    "gl_FragColor = vec4(color, 1.0);\n"
    "}\n";

// TODO: remove
#if 0 
// Gaussian blur.
static const char* blur_vert_src =
    "precision highp float;\n"
    "uniform vec2 u_shift;\n"
    "attribute vec2 a_vert;\n"
    "varying vec2 v_uv[11];\n"
    "#define UV(i_) v_uv[(i_)+5] = uv + float(i_)*u_shift;\n"
    "void main() {\n"
    "vec2 uv = 0.5 + 0.5*sign(a_vert);\n"
   	"UV(-5)\n"
	"UV(-4)\n"
	"UV(-3)\n"
	"UV(-2)\n"
	"UV(-1)\n"
	"UV( 0)\n"
	"UV( 1)\n"
	"UV( 2)\n"
	"UV( 3)\n"
	"UV( 4)\n"
	"UV( 5)\n"
    "gl_Position = vec4(a_vert, 0.0, 1.0);\n"
    "}\n";
static const char* blur_frag_src =
    "precision highp float;\n"
    "uniform sampler2D u_blurTex;\n"
    "varying vec2 v_uv[11];\n"
    "#define TEX(i_, m_) result += (m_) * texture2D(u_blurTex, v_uv[(i_)+5]).rgb;\n"
    "void main(void) {\n"
    "vec3 result = vec3(0.0);\n"
   	"TEX(-5, 1.0   / 1024.0)\n"
	"TEX(-4, 10.0  / 1024.0)\n"
	"TEX(-3, 45.0  / 1024.0)\n"
	"TEX(-2, 120.0 / 1024.0)\n"
	"TEX(-1, 210.0 / 1024.0)\n"
	"TEX( 0, 252.0 / 1024.0)\n"
	"TEX( 1, 210.0 / 1024.0)\n"
	"TEX( 2, 120.0 / 1024.0)\n"
	"TEX( 3, 45.0  / 1024.0)\n"
	"TEX( 4, 10.0  / 1024.0)\n"
	"TEX( 5, 1.0   / 1024.0)\n"
    "gl_FragColor = vec4(result, 1.0);\n"
    "}\n";
#endif


