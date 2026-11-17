// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Common/Window.hpp"

#include <GLFW/glfw3.h>

#if defined(__EMSCRIPTEN__)
#    include <emscripten.h>
#endif

#if defined(__EMSCRIPTEN__)
// clang-format off
//! \brief Suspend the program (ASYNCIFY) until the browser draws its next
//! frame, so that the gallery runs at the refresh rate of the screen.
EM_ASYNC_JS(void, waitForScreenRefresh, (), {
    await new Promise((resolve) => requestAnimationFrame(resolve));
});
// clang-format on
#endif

namespace examples
{

//! \brief What GLFW says when it cannot do something, which is more use than
//! anything this file could invent.
static std::string glfwReason()
{
#if defined(__EMSCRIPTEN__)
    // The GLFW of Emscripten has no glfwGetError: see the browser console.
    return "see the console of the browser";
#else
    const char* why = nullptr;
    glfwGetError(&why);
    return (why == nullptr) ? "no reason given" : why;
#endif
}

// ----------------------------------------------------------------------------
compages::Status
Window::open(std::string const& p_title, int p_width, int p_height)
{
    if (glfwInit() == GLFW_FALSE)
    {
        return compages::failure(
            "no window can be opened: " + glfwReason() +
            ". On a machine without a display, try running "
            "under Xvfb");
    }
    m_glfw_ready = true;

    // The context the backend compiled in the library needs (GPU_BACKEND of
    // Compages.mk, seen here as COMPAGES_GPU_BACKEND_...).
#if defined(COMPAGES_GPU_BACKEND_GLES3)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    const char* const context = "OpenGL ES 3.0";
#elif defined(COMPAGES_GPU_BACKEND_GL41)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // macOS gives a core profile only to a forward compatible context.
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    const char* const context = "OpenGL 4.1 core profile";
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    const char* const context = "OpenGL 4.5 core profile";
#endif
    // Asked for so that the driver reports its own mistakes to us rather than
    // waiting to be interrogated after every call.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
#if defined(__EMSCRIPTEN__)
    // In a web page, draw one pixel per pixel of the screen: without this the
    // canvas has one pixel per CSS pixel, blurred on a screen that scales.
    glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
#endif

    m_window =
        glfwCreateWindow(p_width, p_height, p_title.c_str(), nullptr, nullptr);
    if (m_window == nullptr)
    {
        return compages::failure(
            std::string("no ") + context + " context is available: " + glfwReason() +
            ". macOS never went past 4.1: build with GPU_BACKEND=GL41 there");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetScrollCallback(m_window, &Window::onScroll);
    // One frame per refresh of the screen. Without it the loop runs as fast as
    // it can, which heats the machine and makes the frame rate meaningless.
    // A web page has no swap interval: endFrame waits for the browser.
#if !defined(__EMSCRIPTEN__)
    glfwSwapInterval(1);
#endif

    // The library gets a loader, never a window. This one line is the whole of
    // what ties src/GPU to a windowing library, and it is on this side of the
    // boundary.
    COMPAGES_TRY(compages::gpu::init(
        reinterpret_cast<compages::gpu::LoadProc>(glfwGetProcAddress)));
    m_device_ready = true;

    m_last_time = glfwGetTime();
    m_second_started = m_last_time;

    return compages::success();
}

// ----------------------------------------------------------------------------
Window::~Window()
{
    // In this order, and it matters: the resources have to be given back while
    // the context they live on is still current.
    if (m_device_ready)
    {
        compages::gpu::shutdown();
    }
    if (m_window != nullptr)
    {
        glfwDestroyWindow(m_window);
    }
    if (m_glfw_ready)
    {
        glfwTerminate();
    }
}

// ----------------------------------------------------------------------------
bool Window::closing() const
{
    return (m_window == nullptr) ||
           (glfwWindowShouldClose(m_window) != GLFW_FALSE);
}

// ----------------------------------------------------------------------------
void Window::close()
{
    if (m_window != nullptr)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

// ----------------------------------------------------------------------------
void Window::beginFrame()
{
    glfwPollEvents();
    // Taken after the events: in a web page they arrive while endFrame gives
    // the browser its turn, so before this function, never inside
    // glfwPollEvents.
    m_scroll = m_scroll_pending;
    m_scroll_pending = 0.0f;
#if defined(__EMSCRIPTEN__)
    followBrowserWindow();
#endif

    int width = 0;
    int height = 0;
    // The drawable size, which on a screen that scales its contents is not the
    // size the window was asked for.
    glfwGetFramebufferSize(m_window, &width, &height);
    m_width = static_cast<std::uint32_t>(width < 0 ? 0 : width);
    m_height = static_cast<std::uint32_t>(height < 0 ? 0 : height);

    const double now = glfwGetTime();
    m_elapsed = static_cast<float>(now - m_last_time);
    m_last_time = now;

    ++m_frames_this_second;
    const double since = now - m_second_started;
    if (since >= 1.0)
    {
        m_frames_per_second = static_cast<float>(
            static_cast<double>(m_frames_this_second) / since);
        m_frames_this_second = 0;
        m_second_started = now;
    }

    compages::gpu::resetFrameStatistics();

    const compages::Vector2f now_mouse = mouse();
    if (m_mouse_seen)
    {
        m_mouse_delta = now_mouse - m_mouse;
    }
    else
    {
        m_mouse_delta = compages::Vector2f(0.0f, 0.0f);
        m_mouse_seen = true;
    }
    m_mouse = now_mouse;

    m_mouse_left =
        glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    m_mouse_right =
        glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    m_mouse_left_pressed = m_mouse_left && !m_mouse_left_was;
    m_mouse_left_was = m_mouse_left;
}

// ----------------------------------------------------------------------------
void Window::endFrame()
{
    glfwSwapBuffers(m_window);
#if defined(__EMSCRIPTEN__)
    // In a web page the frame is shown when the browser gets its turn back:
    // ASYNCIFY suspends the loop here and resumes it at the next frame. With
    // vsync, the next frame is the next refresh of the screen
    // (requestAnimationFrame); without, as soon as the browser is free.
    if (m_vsync)
    {
        waitForScreenRefresh();
    }
    else
    {
        emscripten_sleep(0);
    }
#endif
}

#if defined(__EMSCRIPTEN__)
// ----------------------------------------------------------------------------
void Window::followBrowserWindow()
{
    // The page has no window to resize: the canvas takes the whole tab, so the
    // GLFW window follows the size of the tab (in CSS pixels). GLFW then gives
    // the canvas that size times the pixel ratio of the screen.
    // clang-format off
    const int width = EM_ASM_INT({ return window.innerWidth; });
    const int height = EM_ASM_INT({ return window.innerHeight; });
    // clang-format on
    int current_width = 0;
    int current_height = 0;
    glfwGetWindowSize(m_window, &current_width, &current_height);
    if ((width > 0) && (height > 0) &&
        ((width != current_width) || (height != current_height)))
    {
        glfwSetWindowSize(m_window, width, height);
    }
}
#endif

// ----------------------------------------------------------------------------
compages::Vector2f Window::mouse() const
{
    if (m_window == nullptr)
    {
        return compages::Vector2f(0.0f, 0.0f);
    }
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);

    // GLFW gives the cursor in screen coordinates, and the framebuffer has
    // more pixels than that on a screen that scales its contents (a Retina
    // screen, a web page with a pixel ratio of 2): turned into pixels here.
    int window_width = 0;
    int window_height = 0;
    glfwGetWindowSize(m_window, &window_width, &window_height);
    const float scale_x = (window_width > 0)
        ? static_cast<float>(m_width) / static_cast<float>(window_width) : 1.0f;
    const float scale_y = (window_height > 0)
        ? static_cast<float>(m_height) / static_cast<float>(window_height) : 1.0f;

    // GLFW counts from the top of the window and the graphics API from the
    // bottom, so the one place the two disagree is turned over here rather than
    // in every example.
    return compages::Vector2f(static_cast<float>(x) * scale_x,
                                    static_cast<float>(m_height) -
                                        static_cast<float>(y) * scale_y);
}

// ----------------------------------------------------------------------------
bool Window::keyDown(int p_glfw_key) const
{
    return (m_window != nullptr) &&
           (glfwGetKey(m_window, p_glfw_key) == GLFW_PRESS);
}

// ----------------------------------------------------------------------------
void Window::captureMouse(bool p_captured)
{
    if ((m_window == nullptr) || (p_captured == m_mouse_captured))
    {
        return;
    }
    m_mouse_captured = p_captured;
    glfwSetInputMode(m_window,
                     GLFW_CURSOR,
                     p_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (p_captured)
    {
        glfwSetCursor(m_window, nullptr);
    }
    if (glfwRawMouseMotionSupported() == GLFW_TRUE)
    {
        glfwSetInputMode(m_window,
                         GLFW_RAW_MOUSE_MOTION,
                         p_captured ? GLFW_TRUE : GLFW_FALSE);
    }
    // The pointer jumps when it is hidden or shown: that is not a motion.
    m_mouse_seen = false;
}

// ----------------------------------------------------------------------------
void Window::vsync(bool p_enabled)
{
    m_vsync = p_enabled;
#if !defined(__EMSCRIPTEN__)
    if (m_window != nullptr)
    {
        glfwSwapInterval(p_enabled ? 1 : 0);
    }
#endif
}

// ----------------------------------------------------------------------------
void Window::onScroll(GLFWwindow* p_window, double /*p_x*/, double p_y)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(p_window));
    if (self != nullptr)
    {
        self->m_scroll_pending += static_cast<float>(p_y);
    }
}

} // namespace examples
