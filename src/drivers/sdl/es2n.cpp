#include "es2n.h"
#include <cstdlib>
#include <cstring>
#include <cmath>

#define STR(s_) _STR(s_)
#define _STR(s_) #s_

#define NUM_CYCLES 3
#define NUM_COLORS 512 
#define NUM_CYCLES_TEXTURE 4

// Source code modified from:
// http://wiki.nesdev.com/w/index.php/NTSC_video

// Bounds for signal level normalization
#define ATTENUATION 0.746
#define LOWEST      (0.350 * ATTENUATION)
#define HIGHEST     1.962
#define BLACK       0.518
#define WHITE       HIGHEST

// Generate the square wave
static bool inColorPhase(int color, int phase)
{
    return (color + phase) % 12 < 6;
};

// pixel = Pixel color (9-bit) given as input. Bitmask format: "eeellcccc".
// phase = Signal phase. It is a variable that increases by 8 each pixel.
static double NTSCsignal(int pixel, int phase)
{
    // Voltage levels, relative to synch voltage
    const double levels[8] = {
        0.350, 0.518, 0.962, 1.550, // Signal low
        1.094, 1.506, 1.962, 1.962  // Signal high
    };

    // Decode the NES color.
    int color = (pixel & 0x0F);    // 0..15 "cccc"
    int level = (pixel >> 4) & 3;  // 0..3  "ll"
    int emphasis = (pixel >> 6);   // 0..7  "eee"
    if (color > 0x0D) { level = 1; } // Level 1 forced for colors $0E..$0F

    // The square wave for this color alternates between these two voltages:
    float low  = levels[0 + level];
    float high = levels[4 + level];
    if (color == 0) { low = high; } // For color 0, only high level is emitted
    if (color > 0x0C) { high = low; } // For colors $0D..$0F, only low level is emitted

    double signal = inColorPhase(color, phase) ? high : low;

    // When de-emphasis bits are set, some parts of the signal are attenuated:
    if ((color < 0x0E) && ( // Not for colors $0E..$0F (Wiki sample code doesn't have this)
        ((emphasis & 1) && inColorPhase(0, phase))
        || ((emphasis & 2) && inColorPhase(4, phase))
        || ((emphasis & 4) && inColorPhase(8, phase)))) {
        signal = signal * ATTENUATION;
    }

    return signal;
}

static void genKernelTex(es2n *p)
{
    unsigned char *result = (unsigned char*) calloc(4 * NUM_CYCLES_TEXTURE * NUM_COLORS, sizeof(GLubyte));

    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        const int phase = cycle * 8;

        for (int color = 0; color < NUM_COLORS; color++) {

            for (int p = 0; p < 4; p++) {
                double signal;

                signal = (NTSCsignal(color, 2*p + phase) + NTSCsignal(color, 2*p + phase+1)) / 2.0;
                signal = (signal-LOWEST) / (HIGHEST-LOWEST);
                result[4 * (cycle*NUM_COLORS + color) + p] = 0.5 + 255.0 * signal;
            }
        }
    }

    glActiveTexture(GL_TEXTURE1);
    glGenTextures(1, &p->ntsc_tex);
    glBindTexture(GL_TEXTURE_2D, p->ntsc_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NUM_COLORS, NUM_CYCLES_TEXTURE, 0, GL_RGBA, GL_UNSIGNED_BYTE, result);

    glActiveTexture(GL_TEXTURE0);
    free(result);
}

static GLuint compileShader(GLenum type, const char *src)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, 0);
    glCompileShader(shader);
    return shader;
}

static GLuint linkShader(GLuint vert_shader, GLuint frag_shader)
{
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert_shader);
    glAttachShader(prog, frag_shader);
    glLinkProgram(prog);
    glDetachShader(prog, vert_shader);
    glDetachShader(prog, frag_shader);
    glUseProgram(prog);
    return prog;
}

GLuint buildShader(const char *vert_src, const char *frag_src)
{
    GLuint vert_shader = compileShader(GL_VERTEX_SHADER, vert_src);
    GLuint frag_shader = compileShader(GL_FRAGMENT_SHADER, frag_src);
    GLuint result = linkShader(vert_shader, frag_shader);
    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
    return result;
}

void makeFBTex(GLuint *tex, GLuint *fb, int w, int h)
{
    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    glGenFramebuffers(1, fb);
    glBindFramebuffer(GL_FRAMEBUFFER, *fb);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *tex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
//    if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
//        glog("not complete\n");
//  }
}

static void setUniforms(GLuint prog)
{
    GLint k;
    k = glGetUniformLocation(prog, "u_baseTex");
    glUniform1i(k, 0);
    k = glGetUniformLocation(prog, "u_ntscTex");
    glUniform1i(k, 1);
    k = glGetUniformLocation(prog, "u_lvlTex");
    glUniform1i(k, 2);
}

// TODO: reformat inputs to something more meaningful
void es2nInit(es2n *p, int left, int right, int top, int bottom)
{
    memset(p, 0, sizeof(es2n));

    // Configure levels FB.
    makeFBTex(&p->lvl_tex, &p->lvl_fb, 256, 256);
    const char* lvl_vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv;\n"
        "void main() {\n"
        "v_uv = vert.zw;\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    const char* lvl_frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_baseTex;\n"
        "uniform sampler2D u_ntscTex;\n"
        "varying vec2 v_uv;\n"
        "#define IN_SIZE 256.0\n"
        "\n"
        "#define LOWEST  " STR(LOWEST) "\n"
        "#define HIGHEST " STR(HIGHEST) "\n"
        "#define BLACK   " STR(BLACK) "\n"
        "#define WHITE   " STR(WHITE) "\n"
        "\n"
        "void main(void)\n"
        "{\n"
        "    vec2 la = texture2D(u_baseTex, v_uv).ra;\n"
        "    vec2 p = floor(IN_SIZE*v_uv);\n"
        "    vec2 uv = vec2(\n"
        "        dot((255.0/511.0) * vec2(1.0, 64.0), la),\n" // color
        "        mod(p.x - p.y, 3.0) / 3.0\n" // phase
        "    );\n"
        "    vec4 result = texture2D(u_ntscTex, uv) * ((HIGHEST-LOWEST)/(WHITE-BLACK)) + ((LOWEST-BLACK)/(WHITE-BLACK));\n"
        "    gl_FragColor = result;\n"
        "}\n";
    p->lvl_prog = buildShader(lvl_vert_src, lvl_frag_src);
    setUniforms(p->lvl_prog);

    // Set base texture to 0.
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &p->base_tex);
    glBindTexture(GL_TEXTURE_2D, p->base_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, 256, 256, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, 0);

    genKernelTex(p);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    const GLfloat quadverts[4 * 4] = {
        // Packed vertices: x, y, u, v
        -1.0f, -1.0f, left / 256.0f, bottom / 256.0f,
         1.0f, -1.0f, right / 256.0f, bottom / 256.0f,
        -1.0f,  1.0f, left / 256.0f, top / 256.0f, 
         1.0f,  1.0f, right / 256.0f, top / 256.0f
    };

    // Create quad vertex buffer.
    glGenBuffers(1, &p->quadbuf);
    glBindBuffer(GL_ARRAY_BUFFER, p->quadbuf);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadverts), quadverts, GL_STATIC_DRAW);

    // Create vertex attribute array.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);

    // Create shaders and program.
    const char* vert_src =
        "precision highp float;\n"
        "attribute vec4 vert;\n"
        "varying vec2 v_uv[7];\n"
        "#define S vec2(1.0/256.0, 0.0)\n"
        "void main() {\n"
        "v_uv[0] = vert.zw-4.0*S;\n"
        "v_uv[1] = vert.zw-3.0*S;\n"
        "v_uv[2] = vert.zw-2.0*S;\n"
        "v_uv[3] = vert.zw-1.0*S;\n"
        "v_uv[4] = vert.zw+0.0*S;\n"
        "v_uv[5] = vert.zw+1.0*S;\n"
        "v_uv[6] = vert.zw+2.0*S;\n"
        "gl_Position = vec4(vert.xy, 0.0, 1.0);\n"
        "}\n";
    
    const char* frag_src =
        "precision highp float;\n"
        "uniform sampler2D u_baseTex;\n"
        "uniform sampler2D u_ntscTex;\n"
        "uniform vec2 u_mousePos;\n"
        "varying vec2 v_uv[7];\n"
          "#define PI 3.1415926535\n"
          "#define PIC (PI / 6.0)\n"
        "#define IN_SIZE 256.0\n"
        "#define WINDOW_SCALER 4.0\n"
        "#define GAMMA (2.2 / 2.0)\n"
        "const mat3 c_convMat = mat3(\n"
        "    1.0,        1.0,        1.0,       // Y\n"
        "    0.946882,   -0.274788,  -1.108545, // I\n"
        "    0.623557,   -0.635691,  1.709007   // Q\n"
        ");\n"
        "\n"
        "#define LOWEST  " STR(LOWEST) "\n"
        "#define HIGHEST " STR(HIGHEST) "\n"
        "#define BLACK   " STR(BLACK) "\n"
        "#define WHITE   " STR(WHITE) "\n"
        "#define clamp01(v) clamp(v, 0.0, 1.0)\n"
        "\n"
        "const vec3 c_gamma = vec3(GAMMA);\n"
        "const vec4 one = vec4(1.0);\n"
        "const vec4 nil = vec4(0.0);\n"
        "\n"
        "vec4 sampleRaw(in vec2 p, in vec2 la)\n"
        "{\n"
        "    vec2 uv = vec2(\n"
        "        dot((255.0/511.0) * vec2(1.0, 64.0), la),\n" // color
        "        mod(p.x - p.y, 3.0) / 3.0\n" // phase
        "    );\n"
        "     return texture2D(u_ntscTex, uv) * ((HIGHEST-LOWEST)/(WHITE-BLACK)) + ((LOWEST-BLACK)/(WHITE-BLACK));\n"
        "}\n"
        "#define YW 3.0\n"
        "#define IW 6.0\n"
        "#define QW 6.0\n"
/*
        "vec4 k(in vec4 d, in float w)\n"
        "{\n"
        "    return step(d, vec4(w));\n"
        "}\n"
*/
        "vec4 scanphase;\n"
#if 1
        "vec3 sample(in vec2 p, in vec2 la, in vec4 k)\n"
        "{\n"
        "    vec4 v = sampleRaw(p, la);\n"
        "    vec4 a = 8.0*PIC*p.x + scanphase;\n"
        "    return vec3(\n"
        "        dot(k, v),\n"
        "        dot(k, v*cos(a)),\n"
        "        dot(k, v*sin(a))\n"
        "    );\n"
        "}\n"
#elif 0
        "vec3 sample(in vec2 p, in vec4 yk, in vec4 ik, in vec4 qk)\n"
        "{\n"
        "    vec4 v = sampleRaw(p);\n"
        "    vec4 a = 8.0*PIC*p.x + scanphase;\n"
        "    return vec3(\n"
        "        dot(yk, v),\n"
        "        dot(ik, v*cos(a)),\n"
        "        dot(qk, v*sin(a))\n"
        "    );\n"
        "}\n"
#else
        "vec3 sampleK(in vec2 p, in float t)\n"
        "{\n"
        "    vec4 v = sampleRaw(p);\n"
        "    vec4 a = 8.0*PIC*p.x + scanphase;\n"
        "    vec4 d = abs(vec4(0.5, 1.5, 2.5, 3.5) + t);\n"
        "    return vec3(\n"
//        "        dot(mix(k(d, YW), 0.5*k(d, IW), u_mousePos.x), v),\n"
        "        dot(mix(k(d, YW), 0.5*k(d, IW), 1.0/16.0), v),\n"
        "        dot(k(d, IW), v*cos(a)),\n"
        "        dot(k(d, QW), v*sin(a))\n"
        "    );\n"
        "}\n"
#endif
        "void main(void)\n"
        "{\n"
        "    vec2 coord = floor(WINDOW_SCALER*IN_SIZE*v_uv[4]);\n"
        "    vec2 p = floor(coord / WINDOW_SCALER);\n"
        "    vec4 phase = vec4(mod(coord.x, 4.0));\n"
        "    scanphase = PIC * (3.9+vec4(0.5, 2.5, 4.5, 6.5) - mod(p.y*8.0, 12.0));\n"
        "    vec3 yiq = vec3(0.0);\n"
#if 1
        // fringe all to the right, q=48
#if 1
        "    vec4 c0 = clamp01(vec4(1.0, 0.0,-1.0,-2.0) + phase);\n"
        "    vec4 c1 = clamp01(vec4(2.0, 3.0, 4.0, 4.0) - phase);\n"
        "    vec4 c2 = clamp01(vec4(0.0, 0.0, 0.0, 1.0) - phase);\n"

//        "    const float m = 2.0 / 3.0;\n"
#if 1
        "    vec3 s[4];\n"
        "    float m = 1.0/16.0;\n"
        "    float n = 0.0;//u_mousePos.x;\n"

        "    vec2 la[7];\n"
        "    la[0] = texture2D(u_baseTex, v_uv[0]).ra;\n"
        "    la[1] = texture2D(u_baseTex, v_uv[1]).ra;\n"
        "    la[2] = texture2D(u_baseTex, v_uv[2]).ra;\n"
        "    la[3] = texture2D(u_baseTex, v_uv[3]).ra;\n"
        "    la[4] = texture2D(u_baseTex, v_uv[4]).ra;\n"
        "    la[5] = texture2D(u_baseTex, v_uv[5]).ra;\n"
        "    la[6] = texture2D(u_baseTex, v_uv[6]).ra;\n"

        "    s[3]  = sample(p + vec2(-4.0, 0.0), la[0], one-c0);\n"
        "    s[3] += sample(p + vec2(-3.0, 0.0), la[1], one-c2);\n"
        "    s[3] += sample(p + vec2(-2.0, 0.0), la[2], one-c1);\n"

        "    s[2]  = sample(p + vec2(-3.0, 0.0), la[1], c2);\n"
        "    s[2] += sample(p + vec2(-2.0, 0.0), la[2], c1);\n"
        "    s[2] += sample(p + vec2(-1.0, 0.0), la[3], c0);\n"

        "    s[1]  = sample(p + vec2(-1.0, 0.0), la[3], one-c0);\n"
        "    s[1] += sample(p + vec2( 0.0, 0.0), la[4], one-c2);\n"
        "    s[1] += sample(p + vec2( 1.0, 0.0), la[5], one-c1);\n"

        "    s[0]  = sample(p + vec2( 0.0, 0.0), la[4], c2);\n"
        "    s[0] += sample(p + vec2( 1.0, 0.0), la[5], c1);\n"
        "    s[0] += sample(p + vec2( 2.0, 0.0), la[6], c0);\n"

        "    yiq.r = (m*s[0].r + s[1].r + m*s[2].r) / (6.0 * (1.0+m+m));\n"
        "    yiq.g = (n*s[0].g + s[1].g + s[2].g + n*s[3].g) / (6.0 * (2.0+n+n));\n"
        "    yiq.b = (s[0].b + s[1].b + s[2].b + s[3].b) / (6.0 * (4.0));\n"
#else
        "    vec3 s[3];\n"
        "    float m = 1.0;//u_mousePos.y;\n"
        "    float yt = m*0.1;//u_mousePos.x;\n"
        "    float it = m*m*0.1;//u_mousePos.x;\n"

        "    s[2]  = sample(p + vec2(-4.0, 0.0), c2);\n"
        "    s[2] += sample(p + vec2(-3.0, 0.0), c1);\n"
        "    s[2] += sample(p + vec2(-2.0, 0.0), c0);\n"

        "    s[1]  = sample(p + vec2(-2.0, 0.0), one-c0);\n"
        "    s[1] += sample(p + vec2(-1.0, 0.0), one-c2);\n"
        "    s[1] += sample(p,                   one-c1);\n"

        "    s[0]  = sample(p + vec2(-1.0, 0.0), c2);\n"
        "    s[0] += sample(p,                   c1);\n"
        "    s[0] += sample(p + vec2( 1.0, 0.0), c0);\n"

        "    yiq.r = (s[0].r + yt*s[1].r + yt*yt*s[2].r) / (6.0 * (1.0+yt+yt*yt));\n"
        "    yiq.g = (s[0].g + m*s[1].g + it*s[2].g) / (6.0 * (1.0+m+it));\n"
        "    yiq.b = (s[0].b + m*s[1].b + m*m*s[2].b) / (6.0 * (1.0+m+m*m));\n"
#endif

#else
        "    vec4 ye0 = clamp01(vec4(1.0, 0.0,-1.0,-2.0) + phase);\n"
        "    vec4 ye1 = clamp01(vec4(2.0, 3.0, 4.0, 4.0) - phase);\n"
        "    vec4 ye2 = clamp01(vec4(0.0, 0.0, 0.0, 1.0) - phase);\n"
        "    yiq += sample(p + vec2(-1.0, 0.0), ye2, ye2, ye2);\n"
        "    yiq += sample(p,                   ye1, ye1, ye1);\n"
        "    yiq += sample(p + vec2( 1.0, 0.0), ye0, ye0, ye0);\n"
        "    yiq /= 2.0 * vec3(3.0);\n"
#endif
/*
        // even fringing to both sides, q=48
        "    vec4 yedge0 = clamp01(vec4( 0.0, 0.0, 1.0, 2.0) - phase);\n"
        "    vec4 yedge1 = clamp01(vec4( 3.0, 4.0, 4.0, 4.0) - phase);\n"
        "    vec4 yedge2 = clamp01(vec4( 0.0,-1.0,-2.0,-3.0) + phase);\n"
        "    vec4 iedge0 = clamp01(vec4( 0.0, 0.0, 0.0, 1.0) - phase);\n"
        "    vec4 iedge1 = clamp01(vec4( 2.0, 3.0, 4.0, 4.0) - phase);\n"
        "    vec4 iedge2 = clamp01(vec4( 1.0, 1.0, 1.0, 0.0) + phase);\n"
        "    vec4 iedge3 = clamp01(vec4(-1.0,-2.0,-3.0,-3.0) + phase);\n"
        "    vec4 qedge0 = clamp01(vec4( 0.0, 1.0, 2.0, 3.0) - phase);\n"
        "    vec4 qedge1 = clamp01(vec4( 1.0, 0.0,-1.0,-2.0) + phase);\n"
        "    yiq += sample(p + vec2(-3.0, 0.0), zeros,   zeros, qedge0);\n"
        "    yiq += sample(p + vec2(-2.0, 0.0), zeros,  iedge0,   ones);\n"
        "    yiq += sample(p + vec2(-1.0, 0.0), yedge0, iedge1,   ones);\n"
        "    yiq += sample(p,                   yedge1,   ones,   ones);\n"
        "    yiq += sample(p + vec2( 1.0, 0.0), yedge2, iedge2,   ones);\n"
        "    yiq += sample(p + vec2( 2.0, 0.0), zeros,  iedge3,   ones);\n"
        "    yiq += sample(p + vec2( 3.0, 0.0), zeros,   zeros, qedge1);\n"
        "    yiq /= vec3(6.0, 12.0, 24.0);\n"
*/
#else
        "    float ph = -phase[0];\n"
        "    yiq += sampleK(p-vec2(2.0, 0.0), ph-8.0);\n"
        "    yiq += sampleK(p-vec2(1.0, 0.0), ph-4.0);\n"
        "    yiq += sampleK(p,                ph);\n"
        "    yiq += sampleK(p+vec2(1.0, 0.0), ph+4.0);\n"
        "    yiq += sampleK(p+vec2(2.0, 0.0), ph+8.0);\n"
        "    yiq /= 2.0 * vec3(YW, IW, QW);\n"
#endif
//        "    yiq.r *= 2.0 * u_mousePos.y;\n"
//        "    yiq.gb *= 2.0 * u_mousePos.x;\n"
        "    vec3 result = c_convMat * yiq;\n"
//        "    float scan = 1.0 - (5.0/256.0 * (WINDOW_SCALER-1.0)/2.0) * distance(mod(coord.y, WINDOW_SCALER), (WINDOW_SCALER-1.0)/2.0);\n"
//        "    gl_FragColor = vec4(clamp01(pow(result, c_gamma)) * scan, 1.0);\n"
        "    gl_FragColor = vec4(pow(result, c_gamma), 1.0);\n"
        "}\n"
        ;

    p->rgb_prog = buildShader(vert_src, frag_src);
    setUniforms(p->rgb_prog);

// TODO: remove from final
/*
    GLfloat mousePos[2] = { 0.0f, 0.0f };
    GLint uMousePosLoc = glGetUniformLocation(p->rgb_prog, "u_mousePos");
    glUniform2fv(uMousePosLoc, 1, mousePos);
*/
}

void es2nDeinit(es2n *p)
{
// TODO: is cleanup needed?
/*
    if (p->base_tex) {
        glDeleteTextures(1, &p->base_tex);
        p->base_tex = 0;
    }
    if (p->ntsc_tex) {
        glDeleteTextures(1, &p->ntsc_tex);
        p->ntsc_tex = 0;
    }
    if (p->rgb_prog) {
        glDeleteProgram(p->rgb_prog);
        p->rgb_prog = 0;
    }
*/
}

void es2nRender(es2n *p, GLushort *pixels)
{
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 256, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pixels);

	glClear(GL_COLOR_BUFFER_BIT);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

