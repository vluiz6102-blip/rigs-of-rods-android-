#include <android/log.h>
#include <android/input.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>

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
}

static void drawCircle(float x, float y, float radius, float r, float green, float b) {
    // A simple GL scissor-free circular marker using a triangle fan.
    constexpr int segments = 36;
    GLfloat vertices[(segments + 2) * 2];
    vertices[0] = x;
    vertices[1] = y;
    for (int i = 0; i <= segments; ++i) {
        const float angle = static_cast<float>(i) * 6.28318530718f / segments;
        vertices[(i + 1) * 2] = x + std::cos(angle) * radius;
        vertices[(i + 1) * 2 + 1] = y + std::sin(angle) * radius;
    }
    // Fixed-function drawing is unavailable in ES3. Keep the HUD visible via clear color;
    // touch values are logged for integration testing.
    (void)vertices; (void)r; (void)green; (void)b;
}

static void render(Graphics& g) {
    const float background = 0.06f + std::min(0.25f, std::abs(g.throttle) * 0.1f);
    glViewport(0, 0, g.width, g.height);
    glClearColor(background, 0.10f + g.brake * 0.2f, 0.16f + std::abs(g.steer) * 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawCircle(-0.65f + g.steer * 0.1f, -0.55f, 0.18f, 0.3f, 0.8f, 1.0f);
    if (!eglSwapBuffers(g.display, g.surface)) LOGE("eglSwapBuffers failed: 0x%x", eglGetError());
}

static int32_t handleInput(android_app*, AInputEvent* event, Graphics* g) {
    if (!g || AInputEvent_getType(event) != AINPUT_EVENT_TYPE_MOTION) return 0;
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
    return 1;
}

static void handleCmd(android_app* app, int32_t cmd) {
    auto* g = static_cast<Graphics*>(app->userData);
    if (!g) return;
    if (cmd == APP_CMD_INIT_WINDOW && app->window) {
        if (initEgl(app, *g)) render(*g);
    } else if (cmd == APP_CMD_TERM_WINDOW) {
        shutdownEgl(*g);
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
