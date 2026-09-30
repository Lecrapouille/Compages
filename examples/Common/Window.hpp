// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/GPU.hpp"

#include <string>

// ****************************************************************************
//! \file
//! \brief A window, which the library deliberately does not provide.
//!
//! This lives in the examples and not in src/GPU, and that is the point. The
//! library is handed a context that already exists and a function to resolve
//! driver symbols; it never creates a window, never reads a keyboard, and never
//! links against a windowing library. So it works with GLFW, with SDL, with Qt,
//! and inside an application that already has a window of its own.
//!
//! GLFW is what the examples happen to use. Nothing in src/GPU knows that.
// ****************************************************************************

struct GLFWwindow;

namespace examples
{

// ****************************************************************************
//! \brief A window with an OpenGL 4.5 context, and the device started on it.
// ****************************************************************************
class Window
{
public:

    Window() = default;
    ~Window();

    Window(Window const&) = delete;
    Window& operator=(Window const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Open the window and start the library on its context.
    //!
    //! \param[in] p_title what the title bar says.
    //! \param[in] p_width,p_height how large to open it.
    //! \return why it could not be opened, which on a machine without a
    //! suitable driver is worth reading rather than crashing on.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::Status
    open(std::string const& p_title, int p_width, int p_height);

    // ------------------------------------------------------------------------
    //! \brief Has the user asked to close the window?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool closing() const;

    // ------------------------------------------------------------------------
    //! \brief Ask to close the window, as a menu item would.
    // ------------------------------------------------------------------------
    void close();

    // ------------------------------------------------------------------------
    //! \brief Handle whatever the user did, and find out how large the window
    //! is now.
    // ------------------------------------------------------------------------
    void beginFrame();

    // ------------------------------------------------------------------------
    //! \brief Show what was drawn.
    // ------------------------------------------------------------------------
    void endFrame();

    // ------------------------------------------------------------------------
    //! \brief How wide the drawable part of the window is, in pixels.
    //!
    //! Not the same as the size the window was asked for on a screen that
    //! scales its contents, which is why it is read every frame from the driver
    //! rather than remembered from open().
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uint32_t width() const
    {
        return m_width;
    }

    //! \brief How tall the drawable part of the window is, in pixels.
    [[nodiscard]] std::uint32_t height() const
    {
        return m_height;
    }

    // ------------------------------------------------------------------------
    //! \brief How long the previous frame took, in seconds.
    // ------------------------------------------------------------------------
    [[nodiscard]] float elapsed() const
    {
        return m_elapsed;
    }

    // ------------------------------------------------------------------------
    //! \brief Frames per second, averaged over the last second so that the
    //! number is readable rather than flickering.
    // ------------------------------------------------------------------------
    [[nodiscard]] float framesPerSecond() const
    {
        return m_frames_per_second;
    }

    // ------------------------------------------------------------------------
    //! \brief Where the mouse is, in pixels from the bottom left of the window.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::core::Vector2f mouse() const;

    // ------------------------------------------------------------------------
    //! \brief How far the mouse moved since the previous \c beginFrame, y up.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::core::Vector2f mouseDelta() const
    {
        return m_mouse_delta;
    }

    // ------------------------------------------------------------------------
    //! \brief Scroll wheel this frame. Consumed: the next \c beginFrame
    //! resets it. Positive is away from the user.
    // ------------------------------------------------------------------------
    [[nodiscard]] float scroll() const
    {
        return m_scroll;
    }

    [[nodiscard]] bool mouseLeft() const
    {
        return m_mouse_left;
    }
    [[nodiscard]] bool mouseRight() const
    {
        return m_mouse_right;
    }
    //! \brief True on the frame the left button went down.
    [[nodiscard]] bool mouseLeftPressed() const
    {
        return m_mouse_left_pressed;
    }

    [[nodiscard]] bool keyDown(int p_glfw_key) const;

    // ------------------------------------------------------------------------
    //! \brief Hide the pointer and keep it in the window, so that the mouse
    //! can move forever in any direction: what a first person view needs.
    // ------------------------------------------------------------------------
    void captureMouse(bool p_captured);

    //! \brief Is the pointer hidden and held?
    [[nodiscard]] bool mouseCaptured() const
    {
        return m_mouse_captured;
    }

    // ------------------------------------------------------------------------
    //! \brief Wait for the screen before showing a frame (on by default), or
    //! show frames as fast as they are drawn, to measure what they cost.
    // ------------------------------------------------------------------------
    void vsync(bool p_enabled);

    //! \brief Is showing a frame waiting for the screen?
    [[nodiscard]] bool vsync() const
    {
        return m_vsync;
    }

    // ------------------------------------------------------------------------
    //! \brief The window itself, for the parts of the examples that need it,
    //! such as attaching a user interface to it.
    // ------------------------------------------------------------------------
    [[nodiscard]] GLFWwindow* handle() const
    {
        return m_window;
    }

private:

    static void onScroll(GLFWwindow* p_window, double p_x, double p_y);

    GLFWwindow* m_window = nullptr;
    bool m_glfw_ready = false;
    bool m_device_ready = false;

    std::uint32_t m_width = 0u;
    std::uint32_t m_height = 0u;

    double m_last_time = 0.0;
    float m_elapsed = 0.0f;

    //! \brief What the frame rate is averaged over, so that the number shown
    //! does not jump on every frame.
    double m_second_started = 0.0;
    int m_frames_this_second = 0;
    float m_frames_per_second = 0.0f;

    compages::core::Vector2f m_mouse{ 0.0f, 0.0f };
    compages::core::Vector2f m_mouse_delta{ 0.0f, 0.0f };
    bool m_mouse_seen = false;
    float m_scroll = 0.0f;
    bool m_mouse_left = false;
    bool m_mouse_right = false;
    bool m_mouse_left_was = false;
    bool m_mouse_left_pressed = false;
    bool m_mouse_captured = false;
    bool m_vsync = true;
};

} // namespace examples
