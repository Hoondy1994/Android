#include "egl_renderer.h"

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <chrono>
#include <cmath>
#include <cstring>

#define LOG_TAG "FlingerEgl"
#define ALOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr const char* kVertSrc = R"(#version 300 es
layout(location = 0) in vec2 aPos;
out vec2 vUv;
void main() {
    vUv = aPos * 0.5 + 0.5;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

constexpr const char* kFragSrc = R"(#version 300 es
precision highp float;
in vec2 vUv;
uniform float uTime;
uniform vec2 uResolution;
uniform int uStress;
uniform int uMode;
out vec4 fragColor;

float wave(vec2 p, float t) {
    return sin(p.x * 8.0 + t) * cos(p.y * 6.0 - t * 1.3);
}

void main() {
    vec2 uv = vUv;
    vec2 p = (uv - 0.5) * uResolution / min(uResolution.x, uResolution.y);
    float t = uTime;

    float plasma = wave(p, t) + wave(p.yx * 1.7, t * 0.7);
    plasma += sin(length(p) * 12.0 - t * 2.5) * 0.35;

    vec3 base = vec3(
        0.45 + 0.5 * sin(plasma + t),
        0.35 + 0.5 * sin(plasma + 2.1 + t * 0.5),
        0.55 + 0.5 * cos(plasma * 1.3 - t * 0.8)
    );

    if (uMode == 1) {
        float grid = step(0.92, fract(p.x * 20.0 + t)) * step(0.92, fract(p.y * 20.0 - t));
        base = mix(base, vec3(1.0, 0.9, 0.2), grid);
    }

    float alpha = 1.0;
    if (uMode == 2) {
        alpha = 0.55 + 0.45 * sin(t * 3.0 + length(p) * 4.0);
    }

    if (uStress > 0) {
        for (int i = 0; i < uStress; ++i) {
            float fi = float(i);
            vec2 q = p + vec2(sin(t + fi), cos(t * 0.7 + fi)) * 0.15;
            float blob = smoothstep(0.18, 0.0, length(q));
            base += vec3(blob * 0.25, blob * 0.1, blob * 0.35);
        }
    }

    fragColor = vec4(base, alpha);
}
)";

struct EglState {
    ANativeWindow* window = nullptr;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    GLuint program = 0;
    GLuint vao = 0;
    GLuint vbo = 0;
    int width = 0;
    int height = 0;
    FlingerNativeStats stats{};
};

EglState g;

GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof log, nullptr, log);
        ALOGE("shader compile failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

GLuint linkProgram(const char* vert, const char* frag) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vert);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, frag);
    if (!vs || !fs) return 0;

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof log, nullptr, log);
        ALOGE("program link failed: %s", log);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}

bool setupGeometry() {
    const float quad[] = {
        -1.f, -1.f,
         1.f, -1.f,
        -1.f,  1.f,
         1.f,  1.f,
    };

    glGenVertexArrays(1, &g.vao);
    glGenBuffers(1, &g.vbo);
    glBindVertexArray(g.vao);
    glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof quad, quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
    return true;
}

bool chooseConfig(EGLConfig* outConfig) {
    const EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,
        EGL_STENCIL_SIZE, 0,
        EGL_NONE,
    };
    EGLint num = 0;
    if (!eglChooseConfig(g.display, attribs, outConfig, 1, &num) || num == 0) {
        ALOGE("eglChooseConfig failed");
        return false;
    }
    return true;
}

}  // namespace

bool flingerEglAttach(ANativeWindow* window) {
    if (!window) return false;
    flingerEglDetach();

    g.window = window;
    ANativeWindow_acquire(g.window);

    g.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g.display == EGL_NO_DISPLAY) {
        ALOGE("eglGetDisplay failed");
        flingerEglDetach();
        return false;
    }
    if (!eglInitialize(g.display, nullptr, nullptr)) {
        ALOGE("eglInitialize failed");
        flingerEglDetach();
        return false;
    }

    EGLConfig config = nullptr;
    if (!chooseConfig(&config)) {
        flingerEglDetach();
        return false;
    }

    const EGLint ctxAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    g.context = eglCreateContext(g.display, config, EGL_NO_CONTEXT, ctxAttribs);
    if (g.context == EGL_NO_CONTEXT) {
        ALOGE("eglCreateContext failed");
        flingerEglDetach();
        return false;
    }

    g.surface = eglCreateWindowSurface(g.display, config, g.window, nullptr);
    if (g.surface == EGL_NO_SURFACE) {
        ALOGE("eglCreateWindowSurface failed");
        flingerEglDetach();
        return false;
    }

    if (!eglMakeCurrent(g.display, g.surface, g.surface, g.context)) {
        ALOGE("eglMakeCurrent failed");
        flingerEglDetach();
        return false;
    }

    g.program = linkProgram(kVertSrc, kFragSrc);
    if (!g.program) {
        flingerEglDetach();
        return false;
    }
    setupGeometry();

    eglQuerySurface(g.display, g.surface, EGL_WIDTH, &g.width);
    eglQuerySurface(g.display, g.surface, EGL_HEIGHT, &g.height);
    g.stats.bufferWidth = g.width;
    g.stats.bufferHeight = g.height;
    ALOGI("EGL attached %dx%d", g.width, g.height);
    return true;
}

void flingerEglDetach() {
    if (g.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    }
    if (g.program) {
        glDeleteProgram(g.program);
        g.program = 0;
    }
    if (g.vbo) {
        glDeleteBuffers(1, &g.vbo);
        g.vbo = 0;
    }
    if (g.vao) {
        glDeleteVertexArrays(1, &g.vao);
        g.vao = 0;
    }
    if (g.context != EGL_NO_CONTEXT && g.display != EGL_NO_DISPLAY) {
        eglDestroyContext(g.display, g.context);
    }
    if (g.surface != EGL_NO_SURFACE && g.display != EGL_NO_DISPLAY) {
        eglDestroySurface(g.display, g.surface);
    }
    if (g.display != EGL_NO_DISPLAY) {
        eglTerminate(g.display);
    }
    if (g.window) {
        ANativeWindow_release(g.window);
        g.window = nullptr;
    }
    g.context = EGL_NO_CONTEXT;
    g.surface = EGL_NO_SURFACE;
    g.display = EGL_NO_DISPLAY;
    g.width = g.height = 0;
}

void flingerEglResize(int width, int height) {
    if (g.display == EGL_NO_DISPLAY || g.surface == EGL_NO_SURFACE) return;
    g.width = width;
    g.height = height;
    g.stats.bufferWidth = width;
    g.stats.bufferHeight = height;
    glViewport(0, 0, width, height);
}

void flingerEglDraw(float timeSec, int stressLevel, int mode) {
    if (g.display == EGL_NO_DISPLAY || g.surface == EGL_NO_SURFACE || !g.program) return;

    const auto t0 = std::chrono::steady_clock::now();
    if (!eglMakeCurrent(g.display, g.surface, g.surface, g.context)) {
        g.stats.swapOk = 0;
        return;
    }

    glViewport(0, 0, g.width > 0 ? g.width : 1, g.height > 0 ? g.height : 1);
    glClearColor(0.02f, 0.03f, 0.06f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(g.program);
    glUniform1f(glGetUniformLocation(g.program, "uTime"), timeSec);
    glUniform2f(glGetUniformLocation(g.program, "uResolution"),
                static_cast<float>(g.width), static_cast<float>(g.height));
    glUniform1i(glGetUniformLocation(g.program, "uStress"), stressLevel);
    glUniform1i(glGetUniformLocation(g.program, "uMode"), mode);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(g.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    const EGLBoolean swapped = eglSwapBuffers(g.display, g.surface);
    const auto t1 = std::chrono::steady_clock::now();

    g.stats.swapOk = swapped == EGL_TRUE ? 1 : 0;
    g.stats.lastSwapNs = std::chrono::duration_cast<std::chrono::nanoseconds>(t1.time_since_epoch()).count();
    g.stats.lastDrawUs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
}

void flingerEglGetStats(FlingerNativeStats* out) {
    if (!out) return;
    *out = g.stats;
}
