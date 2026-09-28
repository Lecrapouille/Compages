//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "Common/Window.hpp"

#include <GLFW/glfw3.h>

namespace examples
{

//------------------------------------------------------------------------------
//! \brief What GLFW says when it cannot do something, which is more use than
//! anything this file could invent.
//------------------------------------------------------------------------------
static std::string glfwReason()
{
    const char* why = nullptr;
    glfwGetError(&why);
    return (why == nullptr) ? "no reason given" : why;
}

//------------------------------------------------------------------------------
compages::gpu::Status Window::open(std::string const& p_title, int p_width, int p_height)
{
    if (glfwInit() == GLFW_FALSE)
    {
        return compages::gpu::failure("no window can be opened: " + glfwReason() +
                            ". On a machine without a display, try running "
                            "under Xvfb");
    }
    m_glfw_ready = true;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // Asked for so that the driver reports its own mistakes to us rather than
    // waiting to be interrogated after every call.
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
    glfwWindowHint(GLFW_DEPTH_BITS, 24);

    m_window =
        glfwCreateWindow(p_width, p_height, p_title.c_str(), nullptr, nullptr);
    if (m_window == nullptr)
    {
        return compages::gpu::failure(
            "no OpenGL 4.5 core profile context is available: " + glfwReason() +
            ". This backend needs 4.5 for Direct State Access; macOS never went "
            "past 4.1");
    }

    glfwMakeContextCurrent(m_window);
    glfwSetWindowUserPointer(m_window, this);
    glfwSetScrollCallback(m_window, &Window::onScroll);
    // One frame per refresh of the screen. Without it the loop runs as fast as it
    // can, which heats the machine and makes the frame rate meaningless.
    glfwSwapInterval(1);

    // The library gets a loader, never a window. This one line is the whole of
    // what ties src/GPU to a windowing library, and it is on this side of the
    // boundary.
    COMPAGES_TRY(compages::gpu::init(reinterpret_cast<compages::gpu::LoadProc>(glfwGetProcAddress)));
    m_device_ready = true;

    m_last_time = glfwGetTime();
    m_second_started = m_last_time;

    return compages::gpu::success();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
bool Window::closing() const
{
    return (m_window == nullptr) ||
           (glfwWindowShouldClose(m_window) != GLFW_FALSE);
}

//------------------------------------------------------------------------------
void Window::close()
{
    if (m_window != nullptr)
    {
        glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }
}

//------------------------------------------------------------------------------
void Window::beginFrame()
{
    m_scroll = 0.0f;
    glfwPollEvents();

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
        m_frames_per_second =
            static_cast<float>(static_cast<double>(m_frames_this_second) / since);
        m_frames_this_second = 0;
        m_second_started = now;
    }

    compages::gpu::resetFrameStatistics();

    const Vector2f now_mouse = mouse();
    if (m_mouse_seen)
    {
        m_mouse_delta = now_mouse - m_mouse;
    }
    else
    {
        m_mouse_delta = Vector2f(0.0f, 0.0f);
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

//------------------------------------------------------------------------------
void Window::endFrame()
{
    glfwSwapBuffers(m_window);
}

//------------------------------------------------------------------------------
Vector2f Window::mouse() const
{
    if (m_window == nullptr)
    {
        return Vector2f(0.0f, 0.0f);
    }
    double x = 0.0;
    double y = 0.0;
    glfwGetCursorPos(m_window, &x, &y);

    // GLFW counts from the top of the window and the graphics API from the bottom,
    // so the one place the two disagree is turned over here rather than in every
    // example.
    return Vector2f(static_cast<float>(x),
                    static_cast<float>(m_height) - static_cast<float>(y));
}

//------------------------------------------------------------------------------
bool Window::keyDown(int p_glfw_key) const
{
    return (m_window != nullptr) &&
           (glfwGetKey(m_window, p_glfw_key) == GLFW_PRESS);
}

//------------------------------------------------------------------------------
void Window::captureMouse(bool p_captured)
{
    if ((m_window == nullptr) || (p_captured == m_mouse_captured))
    {
        return;
    }
    m_mouse_captured = p_captured;
    glfwSetInputMode(m_window, GLFW_CURSOR,
                     p_captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    if (p_captured)
    {
        glfwSetCursor(m_window, nullptr);
    }
    if (glfwRawMouseMotionSupported() == GLFW_TRUE)
    {
        glfwSetInputMode(m_window, GLFW_RAW_MOUSE_MOTION, p_captured ? GLFW_TRUE : GLFW_FALSE);
    }
    // The pointer jumps when it is hidden or shown: that is not a motion.
    m_mouse_seen = false;
}

//------------------------------------------------------------------------------
void Window::vsync(bool p_enabled)
{
    m_vsync = p_enabled;
    if (m_window != nullptr)
    {
        glfwSwapInterval(p_enabled ? 1 : 0);
    }
}

//------------------------------------------------------------------------------
void Window::onScroll(GLFWwindow* p_window, double /*p_x*/, double p_y)
{
    auto* self = static_cast<Window*>(glfwGetWindowUserPointer(p_window));
    if (self != nullptr)
    {
        self->m_scroll += static_cast<float>(p_y);
    }
}

} // namespace examples
