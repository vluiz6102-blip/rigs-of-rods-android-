#include <android/log.h>
#include <android/input.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include "vehicle_controls.h"

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "RoRPortProbe", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "RoRPortProbe", __VA_ARGS__)

struct Graphics {
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    int32_t width = 0;
    int32_t height = 0;
    float steer = 0.0f;
    float throttle = 0.0f;
    float brake = 0.0f;
    int joystickPointer = -1;
    int throttlePointer = -1;
    int brakePointer = -1;
    GLuint program = 0;
    GLuint vbo = 0;
    GLint colorLocation = -1;
    GLint positionLocation = -1;
    bool running = true;
};

static bool initEgl(android_app* app, Graphics& g) {
    g.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (g.display == EGL_NO_DISPLAY || !eglInitialize(g.display, nullptr, nullptr)) {
        LOGE("eglInitialize failed");
        return false;
    }
    const EGLint attrs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16, EGL_NONE
    };
    EGLConfig config{};
    EGLint count = 0;
    if (!eglChooseConfig(g.display, attrs, &config, 1, &count) || count < 1) {
        LOGE("No EGL config for OpenGL ES 3");
        return false;
    }
    const EGLint ctxAttrs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    g.context = eglCreateContext(g.display, config, EGL_NO_CONTEXT, ctxAttrs);
    g.surface = eglCreateWindowSurface(g.display, config, app->window, nullptr);
    if (g.context == EGL_NO_CONTEXT || g.surface == EGL_NO_SURFACE ||
        !eglMakeCurrent(g.display, g.surface, g.surface, g.context)) {
        LOGE("Failed to create EGL context/surface");
        return false;
    }
    eglQuerySurface(g.display, g.surface, EGL_WIDTH, &g.width);
    eglQuerySurface(g.display, g.surface, EGL_HEIGHT, &g.height);
    glViewport(0, 0, g.width, g.height);
    LOGI("OpenGL ES diagnostic surface: %dx%d", g.width, g.height);
    return true;
}

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint ok = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (ok != GL_TRUE) {
        GLint length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char log[1024] = {};
        glGetShaderInfoLog(shader, std::min(length, 1023), nullptr, log);
        LOGE("Shader compilation failed: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static bool initHud(Graphics& g) {
    const char* vertexSource =
        "#version 300 es\n"
        "layout(location=0) in vec2 aPosition;\n"
        "void main(){ gl_Position=vec4(aPosition,0.0,1.0); }\n";
    const char* fragmentSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform vec4 uColor;\n"
        "out vec4 outColor;\n"
        "void main(){ outColor=uColor; }\n";
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);
    if (!vs || !fs) {
        if (vs) glDeleteShader(vs);
        if (fs) glDeleteShader(fs);
        return false;
    }
    g.program = glCreateProgram();
    glAttachShader(g.program, vs);
    glAttachShader(g.program, fs);
    glLinkProgram(g.program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    GLint linked = GL_FALSE;
    glGetProgramiv(g.program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        char log[1024] = {};
        glGetProgramInfoLog(g.program, sizeof(log), nullptr, log);
        LOGE("HUD shader link failed: %s", log);
        glDeleteProgram(g.program);
        g.program = 0;
        return false;
    }
    g.colorLocation = glGetUniformLocation(g.program, "uColor");
    g.positionLocation = 0;
    glGenBuffers(1, &g.vbo);
    return g.vbo != 0;
}

static void shutdownEgl(Graphics& g) {
    if (g.display != EGL_NO_DISPLAY) {
        eglMakeCurrent(g.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (g.surface != EGL_NO_SURFACE) eglDestroySurface(g.display, g.surface);
        if (g.context != EGL_NO_CONTEXT) eglDestroyContext(g.display, g.context);
        eglTerminate(g.display);
    }
    g.display = EGL_NO_DISPLAY;
    g.surface = EGL_NO_SURFACE;
    g.context = EGL_NO_CONTEXT;
    g.program = 0;
    g.vbo = 0;
}

static void drawShape(Graphics& g, const GLfloat* vertices, GLsizei count,
                      GLenum mode, float r, float green, float b, float alpha) {
    glUseProgram(g.program);
    glUniform4f(g.colorLocation, r, green, b, alpha);
    glBindBuffer(GL_ARRAY_BUFFER, g.vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(count * 2 * sizeof(GLfloat)),
                 vertices, GL_STREAM_DRAW);
    glEnableVertexAttribArray(static_cast<GLuint>(g.positionLocation));
    glVertexAttribPointer(static_cast<GLuint>(g.positionLocation), 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(mode, 0, count);
    glDisableVertexAttribArray(static_cast<GLuint>(g.positionLocation));
}

static void drawCircle(Graphics& g, float x, float y, float radius,
                       float r, float green, float b, float alpha = 0.65f) {
    constexpr int segments = 40;
    GLfloat vertices[(segments + 2) * 2];
    vertices[0] = x;
    vertices[1] = y;
    for (int i = 0; i <= segments; ++i) {
        const float angle = static_cast<float>(i) * 6.28318530718f / segments;
        vertices[(i + 1) * 2] = x + std::cos(angle) * radius;
        vertices[(i + 1) * 2 + 1] = y + std::sin(angle) * radius;
    }
    drawShape(g, vertices, segments + 2, GL_TRIANGLE_FAN, r, green, b, alpha);
}

static void render(Graphics& g) {
    const float background = 0.035f + std::min(0.10f, std::abs(g.throttle) * 0.04f);
    glViewport(0, 0, g.width, g.height);
    glClearColor(background, 0.065f + g.brake * 0.035f, 0.09f + std::abs(g.steer) * 0.035f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (!g.program || !g.vbo) {
        if (!initHud(g)) {
            LOGE("HUD renderer initialization failed");
            return;
        }
    }

    // Virtual steering wheel: outer pad plus an indicator that follows the finger.
    drawCircle(g, -0.72f, -0.52f, 0.30f, 0.15f, 0.19f, 0.23f, 0.70f);
    drawCircle(g, -0.72f, -0.52f, 0.24f, 0.35f, 0.40f, 0.44f, 0.75f);
    drawCircle(g, -0.72f + g.steer * 0.18f, -0.52f, 0.075f, 0.25f, 0.78f, 0.96f, 0.95f);

    // Accelerator and brake pedals with fill levels that mirror current touch values.
    drawCircle(g, 0.76f, 0.48f, 0.20f, 0.12f, 0.27f, 0.17f, 0.75f);
    drawCircle(g, 0.76f, 0.48f, 0.055f + 0.11f * g.throttle, 0.20f, 0.88f, 0.34f, 0.95f);
    drawCircle(g, 0.76f, -0.48f, 0.20f, 0.30f, 0.13f, 0.13f, 0.75f);
    drawCircle(g, 0.76f, -0.48f, 0.055f + 0.11f * g.brake, 0.98f, 0.25f, 0.20f, 0.95f);

    // Placeholder chassis only: proves the render loop and control-state visualization,
    // not the Rigs of Rods vehicle renderer or simulation.
    const GLfloat chassis[] = {
        -0.20f, -0.10f, 0.20f, -0.10f, 0.15f, 0.10f,
        -0.20f, -0.10f, 0.15f, 0.10f, -0.15f, 0.10f
    };
    drawShape(g, chassis, 6, GL_TRIANGLES, 0.72f, 0.74f, 0.76f, 1.0f);
    if (!eglSwapBuffers(g.display, g.surface)) LOGE("eglSwapBuffers failed: 0x%x", eglGetError());
}

static int32_t handleInput(android_app*, AInputEvent* event, Graphics* g) {
    if (!g) return 0;

    // Physical controllers report normalized joystick axes through Android's
    // motion-event API. Keep this mapping separate from the touch overlay so
    // either input source can drive the same normalized control contract.
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION &&
        (AInputEvent_getSource(event) & AINPUT_SOURCE_JOYSTICK) == AINPUT_SOURCE_JOYSTICK) {
        const float x = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_X, 0);
        const float y = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Y, 0);
        const float rightTrigger = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RTRIGGER, 0);
        const float brakeTrigger = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_LTRIGGER, 0);
        const float z = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_Z, 0);
        const float rz = AMotionEvent_getAxisValue(event, AMOTION_EVENT_AXIS_RZ, 0);

        ror_android::VehicleControls controls = ror_android::neutralControls();
        controls.steering = std::abs(x) > 0.08f ? x : 0.0f;
        // Prefer trigger axes when present; fall back to the right stick's Y
        // axis for devices that expose no analog trigger range.
        controls.throttle = std::max(rightTrigger, std::max(0.0f, -y));
        controls.brake = std::max(brakeTrigger, std::max(0.0f, y));
        if (rightTrigger == 0.0f && brakeTrigger == 0.0f && (z != 0.0f || rz != 0.0f)) {
            controls.throttle = std::max(0.0f, (z + 1.0f) * 0.5f);
            controls.brake = std::max(0.0f, (rz + 1.0f) * 0.5f);
        }
        controls = ror_android::normalizeControls(controls.steering, controls.throttle, controls.brake);
        g->steer = controls.steering;
        g->throttle = controls.throttle;
        g->brake = controls.brake;
        return 1;
    }

    if (AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
    const int action = AMotionEvent_getAction(event);
    const int masked = action & AMOTION_EVENT_ACTION_MASK;
    const int index = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >>
                      AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
    const int pointerId = AMotionEvent_getPointerId(event, index);
    const float x = AMotionEvent_getX(event, index);
    const float y = AMotionEvent_getY(event, index);
    const float w = static_cast<float>(std::max(1, g->width));
    const float h = static_cast<float>(std::max(1, g->height));

    if (masked == AMOTION_EVENT_ACTION_DOWN || masked == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if (x < w * 0.48f && g->joystickPointer < 0) g->joystickPointer = pointerId;
        else if (x >= w * 0.72f && y < h * 0.62f && g->throttlePointer < 0) g->throttlePointer = pointerId;
        else if (x >= w * 0.72f && y >= h * 0.62f && g->brakePointer < 0) g->brakePointer = pointerId;
    }

    if (masked == AMOTION_EVENT_ACTION_UP || masked == AMOTION_EVENT_ACTION_POINTER_UP ||
        masked == AMOTION_EVENT_ACTION_CANCEL) {
        if (pointerId == g->joystickPointer) { g->joystickPointer = -1; g->steer = 0.0f; }
        if (pointerId == g->throttlePointer) { g->throttlePointer = -1; g->throttle = 0.0f; }
        if (pointerId == g->brakePointer) { g->brakePointer = -1; g->brake = 0.0f; }
    } else {
        const size_t count = AMotionEvent_getPointerCount(event);
        for (size_t i = 0; i < count; ++i) {
            const int id = AMotionEvent_getPointerId(event, i);
            const float px = AMotionEvent_getX(event, i);
            const float py = AMotionEvent_getY(event, i);
            if (id == g->joystickPointer) g->steer = std::clamp((px / w - 0.24f) / 0.22f, -1.0f, 1.0f);
            if (id == g->throttlePointer) g->throttle = std::clamp(1.0f - py / (h * 0.62f), 0.0f, 1.0f);
            if (id == g->brakePointer) g->brake = std::clamp((py / h - 0.62f) / 0.38f, 0.0f, 1.0f);
        }
    }
    const ror_android::VehicleControls controls = ror_android::normalizeControls(
        g->steer, g->throttle, g->brake);
    g->steer = controls.steering;
    g->throttle = controls.throttle;
    g->brake = controls.brake;
    return 1;
}

static void resetControls(Graphics& g) {
    const ror_android::VehicleControls neutral = ror_android::neutralControls();
    g.steer = neutral.steering;
    g.throttle = neutral.throttle;
    g.brake = neutral.brake;
    g.joystickPointer = -1;
    g.throttlePointer = -1;
    g.brakePointer = -1;
}

static void handleCmd(android_app* app, int32_t cmd) {
    auto* g = static_cast<Graphics*>(app->userData);
    if (!g) return;
    if (cmd == APP_CMD_INIT_WINDOW && app->window) {
        resetControls(*g);
        if (g->display != EGL_NO_DISPLAY) shutdownEgl(*g);
        if (initEgl(app, *g)) { if (initHud(*g)) render(*g); else LOGE("Failed to initialize HUD"); }
    } else if (cmd == APP_CMD_TERM_WINDOW) {
        resetControls(*g);
        shutdownEgl(*g);
    } else if (cmd == APP_CMD_PAUSE || cmd == APP_CMD_LOST_FOCUS || cmd == APP_CMD_STOP) {
        resetControls(*g);
    } else if (cmd == APP_CMD_WINDOW_RESIZED && g->display != EGL_NO_DISPLAY) {
        eglQuerySurface(g->display, g->surface, EGL_WIDTH, &g->width);
        eglQuerySurface(g->display, g->surface, EGL_HEIGHT, &g->height);
    }
}

void android_main(android_app* app) {
    app_dummy();
    Graphics graphics;
    app->userData = &graphics;
    app->onAppCmd = handleCmd;
    app->onInputEvent = [](android_app* state, AInputEvent* event) -> int32_t {
        return handleInput(state, event, static_cast<Graphics*>(state->userData));
    };
    LOGI("Started native Android OpenGL ES diagnostic. Not the Rigs of Rods game.");
    while (graphics.running) {
        int events = 0;
        android_poll_source* source = nullptr;
        while (ALooper_pollOnce(graphics.display == EGL_NO_DISPLAY ? -1 : 0, nullptr, &events,
                                reinterpret_cast<void**>(&source)) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                graphics.running = false;
                break;
            }
        }
        if (graphics.display != EGL_NO_DISPLAY && graphics.surface != EGL_NO_SURFACE) render(graphics);
    }
    shutdownEgl(graphics);
}
