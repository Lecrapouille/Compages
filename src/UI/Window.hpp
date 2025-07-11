//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_UI_WINDOW_HPP
#define OPENGLCPPWRAPPER_UI_WINDOW_HPP

// *****************************************************************************
//! \file GLWindow.hpp manages a window and its i/o for drawing OpenGL scenes.
// *****************************************************************************

#include "Math/Vector.hpp"
#include "OpenGL/Context/OpenGL.hpp"
#include <mutex>

class Layer;

// ***************************************************************************
//! \class GLWindow Window.hpp
//! \ingroup UI
//!
//! \brief GLWindow is an abstract window class for displaying OpenGL
//! application. This class wrappes the glfw3 window creation which initializes
//! OpenGL context. This class offers callbacks for the setup, update and manage
//! windows events (resize, mouse, keyboard). These callbacks have to be
//! implemented in derived class.
//!
//! GLWindow needs to be created by GLApplication. From GLApplication you can
//! create several independent GLWindow at the same time. If you want a single
//! windows, please use instead StandAloneGLWindow it does not need
//! GLApplication.
// ***************************************************************************
class GLWindow
{
public:

    //--------------------------------------------------------------------------
    //! \brief Structure holding mouse states used by GLApplication callbacks.
    //--------------------------------------------------------------------------
    struct Mouse
    {
        //----------------------------------------------------------------------
        //! \brief Button type
        //----------------------------------------------------------------------
        enum Button
        {
            LEFT = GLFW_MOUSE_BUTTON_LEFT,
            RIGHT = GLFW_MOUSE_BUTTON_RIGHT,
            MIDDLE = GLFW_MOUSE_BUTTON_MIDDLE,
            ONE = GLFW_MOUSE_BUTTON_1,
            TWO,
            THREE,
            FOUR,
            FIVE,
            SIX,
            SEVEN,
            EIGHT,
            NONE = GLFW_MOUSE_BUTTON_LAST + 1u,
        };

        //! \brief position (x and y coordinate)
        Vector2g position = { 0.0 };
        //! \brief displacement (delta position)
        Vector2g displacement = { 0.0 };
        //! \brief scrolling
        Vector2g scroll = { 0.0 };
        //! \brief is cursor visible ?
        bool visible = true;
        //! \brief button pressed or released ?
        bool pressed = false;
        //! \brief which button was pressed or released.
        GLWindow::Mouse::Button button = GLWindow::Mouse::Button::NONE;
    };

    //--------------------------------------------------------------------------
    //! \brief Define window events that you can enable. Default is None.
    //! \note these flags have operators | and &.
    //--------------------------------------------------------------------------
    enum class Event : unsigned
    {
        //! \brief No callback
        None = 0x0,
        //! \brief Keyboard event (pressed and released)
        Keyboard = 0x01,
        //! \brief Mouse button event (pressed and released)
        MouseButton = 0x2,
        //! \brief Mouse motion event (X and Y axis)
        MouseMove = 0x4,
        //! \brief Mouse scroll event
        MouseScroll = 0x8,
        //! \brief All events
        All = 0xFFFF
    };

    //--------------------------------------------------------------------------
    //! \brief Create a glfw3 window with given initial size and title.
    //--------------------------------------------------------------------------
    GLWindow(uint32_t const width, uint32_t const height, const char* title);

    //--------------------------------------------------------------------------
    //! \brief Destroy the glfw3 window.
    //--------------------------------------------------------------------------
    virtual ~GLWindow();

    //--------------------------------------------------------------------------
    //! \brief Start the inifinite loop running the window calling the setup()
    //! method and the update() method at the given FPS (TODO).
    //--------------------------------------------------------------------------
    bool run();

    //--------------------------------------------------------------------------
    //! \brief Start the inifinite loop running the window calling the setup()
    //! method and the update() method at the given FPS (TODO).
    //--------------------------------------------------------------------------
    inline void halt()
    {
        m_should_halt = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Enable event reactions from mouse boutton, mouse scroll, mouse
    //! motion, keyboard key pressed or released).
    //--------------------------------------------------------------------------
    void reactTo(GLWindow::Event const events = GLWindow::Event::All);

    //--------------------------------------------------------------------------
    //! \brief Hide and grab the mouse cursor.
    //--------------------------------------------------------------------------
    void hideCursor();

    //--------------------------------------------------------------------------
    //! \brief Make the cursor visible.
    //--------------------------------------------------------------------------
    void showCursor();

    //--------------------------------------------------------------------------
    //! \brief Return the delta time (in s) from the previous update() call.
    //--------------------------------------------------------------------------
    inline float dt() const
    {
        return m_deltaTime;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the number of frame per seconds from the previous update()
    //! call. The value is averaged along 1 second.
    //--------------------------------------------------------------------------
    inline uint32_t fps() const
    {
        return m_fps;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the OpenGL context linked to this window.
    //--------------------------------------------------------------------------
    inline GL::Context::Window& context()
    {
        assert(m_context != nullptr && "Context has not been created");
        return *m_context;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the structure holding mouse states. To be used from
    //! mouse callback functions onMouseButtonPressed(), onMouseScrolled(),
    //! onMouseMoved().
    //--------------------------------------------------------------------------
    static GLWindow::Mouse& mouse()
    {
        return m_mouse;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the current width of the window. To be used from mouse
    //! callback functions onResized().
    //--------------------------------------------------------------------------
    template <typename T>
    static T width()
    {
        return static_cast<T>(staticWidth());
    }

    //--------------------------------------------------------------------------
    //! \brief Return the current height of the window.
    //--------------------------------------------------------------------------
    template <typename T>
    static T height()
    {
        return static_cast<T>(staticHeight());
    }

    //--------------------------------------------------------------------------
    //! \brief Change the size of the window. Do not call this method if you
    //! are resizing the windows manually: this is already done internally.
    //--------------------------------------------------------------------------
    void resize(uint32_t const width, uint32_t const height);

    //--------------------------------------------------------------------------
    //! \brief Is the window is on full screen or not.
    //! \return true if the windows is in full screen, return false else.
    //--------------------------------------------------------------------------
    inline bool isFullScreen() const
    {
        return (m_context != nullptr) &&
               (glfwGetWindowMonitor(m_context) != nullptr);
    }

    //--------------------------------------------------------------------------
    //! \brief Is the key is currently pressed ?
    //! \return true if pressed, false else.
    //--------------------------------------------------------------------------
    static bool isKeyDown(size_t const key)
    {
        return GLWindow::KEY_PRESS == m_currentKeys[key];
    }

    //--------------------------------------------------------------------------
    //! \brief Is the key is currently released ?
    //! \return true if released, false else.
    //--------------------------------------------------------------------------
    static bool isKeyUp(size_t const key)
    {
        return GLWindow::KEY_RELEASE == m_currentKeys[key];
    }

    //--------------------------------------------------------------------------
    //! \brief Has the key been pressed ?
    //! \return true if currently pressed and was released the state before.
    //--------------------------------------------------------------------------
    static bool wasKeyPressed(size_t const key)
    {
        return GLWindow::KEY_PRESS == m_currentKeys[key] &&
               GLWindow::KEY_RELEASE == m_lastKeys[key];
    }

    //--------------------------------------------------------------------------
    //! \brief Has the key been pressed ?
    //! \return true if currently released and was pressed the state before.
    //--------------------------------------------------------------------------
    static bool wasKeyReleased(size_t const key)
    {
        return GLWindow::KEY_RELEASE == m_currentKeys[key] &&
               GLWindow::KEY_PRESS == m_lastKeys[key];
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Compute and display in in the window title the number of
    //! frame per seconds (FPS). Needed for knowing the delta time between
    //! two draw cycles.
    //--------------------------------------------------------------------------
    void computeFPS();

    //--------------------------------------------------------------------------
    //! \brief Get the GPU Memory usage. This method returns an estimated usage
    //! for the current application. Trigger the method onGPUMemoryChanged()
    //! that needs to be implemented on derived class.
    //--------------------------------------------------------------------------
    void monitorGPUMemory();

    //--------------------------------------------------------------------------
    //! \brief Callback when the GPU received new data. Default behavior
    //! is to do nothing.
    //--------------------------------------------------------------------------
    virtual void onGPUMemoryChanged(size_t /*bytes*/) {}

    //--------------------------------------------------------------------------
    //! \brief Callback when a key of the keyboard has been pressed or released.
    //! Enable event reaction throught makeReactOn();
    //--------------------------------------------------------------------------
    virtual void onKeyboardEvent() {}

    //--------------------------------------------------------------------------
    //! \brief Callback when the mouse has been moved. Default behavior
    //! is to do nothing. Call mouse() to get mouse states.
    //! Enable event reaction throught makeReactOn();
    //--------------------------------------------------------------------------
    virtual void onMouseMoved() {}

    //--------------------------------------------------------------------------
    //! \brief Callback when the mouse has been scrolled. Default behavior
    //! is to do nothing. Call mouse() to get mouse states.
    //! Enable event reaction throught makeReactOn();
    //--------------------------------------------------------------------------
    virtual void onMouseScrolled() {}

    //--------------------------------------------------------------------------
    //! \brief Callback when the mouse has been pressed. Default behavior
    //! is to do nothing. Call mouse() to get mouse states.
    //! Enable event reaction throught makeReactOn();
    //--------------------------------------------------------------------------
    virtual void onMouseButtonPressed() {}

    //--------------------------------------------------------------------------
    //! \brief Callback when the window has its size changed.
    //! Use template mtthods width<T>(), height<T>() to get new size.
    //--------------------------------------------------------------------------
    virtual void onWindowResized() {}

    //--------------------------------------------------------------------------
    //! \brief Graphics setup. This method will call onSetup() on success or
    //! call onSetupFailed() on failure.
    //! \return true on success, false on failure.
    //--------------------------------------------------------------------------
    bool setup();

    //--------------------------------------------------------------------------
    //! \brief Add here all stuffs concerning the init of your 3D game.
    //! \return false for aborting start(), else true for continuing.
    //--------------------------------------------------------------------------
    virtual bool onSetup() = 0;

    //--------------------------------------------------------------------------
    //! \brief Callback triggered when the method setup() failed. By default
    //! this method does nothing.
    //--------------------------------------------------------------------------
    virtual void onSetupFailed(std::string const& reason) = 0;

    //--------------------------------------------------------------------------
    //! \brief Paint, manage window events. This method will call
    //! onPaintFailed() on failure.
    //! \return true on success, false on failure.
    //--------------------------------------------------------------------------
    bool update();

    //--------------------------------------------------------------------------
    //! \brief Condition for halting the window loop started by run(). By
    //! default when the ESC key has been pressed or when the user has clicked
    //! on the X button. Override this function to change the defalut behavior.
    //! \return true when the application shall halt the run() method.. Return
    //! false to continue the run() method.
    //--------------------------------------------------------------------------
    virtual bool haltCondition();

    //--------------------------------------------------------------------------
    //! \brief Add here all stuffs painting your 3D world to be
    //! displayed. This method is called by the start() method.
    //! \return false for halting start(), else return true for continuing.
    //--------------------------------------------------------------------------
    virtual bool onPaint() = 0;

    //--------------------------------------------------------------------------
    //! \brief Callback triggered when the method draw() failed. By default this
    //! method does nothing.
    //--------------------------------------------------------------------------
    virtual void onPaintFailed(std::string const& reason) = 0;

    //! \brief Windows current height. This method is only used to avoid using
    //! directly static member variable which need to init on the cpp file.
    static uint32_t& staticHeight()
    {
        static uint32_t h;
        return h;
    }

    //! \brief Windows current width. This method is only used to avoid using
    //! directly static member variable which need to init on the cpp file.
    static uint32_t& staticWidth()
    {
        static uint32_t w;
        return w;
    }

protected:

    //! \brief
    bool m_should_halt = false;
    //! \brief
    std::vector<std::unique_ptr<Layer>> m_layers;

private:

    //
    static bool const KEY_PRESS = true;
    static bool const KEY_RELEASE = false;

    //! \brief OpenGL context associated to this window.
    GL::Context::Window* m_context = nullptr;
    //! \brief Windows title.
    const char* m_title = nullptr;
    //! \brief Used for computing m_deltaTime.
    double m_lastTime = 0.0;
    //! \brief Used for computing m_deltaTime.
    double m_lastFrameTime = 0.0;
    //! \brief  Used for computing m_fps.
    uint32_t nbFrames = 0u;
    //! \brief Frames Per Seconds.
    uint32_t m_fps = 0;
    //! \brief The time (in seconds) spent from the previous call of update()
    //! method.
    float m_deltaTime = 0.0f;
    //! \brief GLWindow::Mouse states.
    static GLWindow::Mouse m_mouse;
    //! \brief Previous keyboard map.
    static std::vector<char> m_lastKeys;
    //! \brief Current keyboard map.
    static std::vector<char> m_currentKeys;
    //! \brief Memorize the GPU consumption to call onGPUMemoryChanged() only
    //! when the memory changed.
    size_t previous_gpu_mem = 0_z;
    //! \brief Protect the keyboard against concurrent accesses.
    mutable std::mutex m_mutex_keyboard;
};

//--------------------------------------------------------------------------
//! \brief Allow to do OR operations on Events to enable several GLWindow
//! events.
//--------------------------------------------------------------------------
constexpr GLWindow::Event operator|(GLWindow::Event lhs, GLWindow::Event rhs)
{
    return static_cast<GLWindow::Event>(
        static_cast<std::underlying_type<GLWindow::Event>::type>(lhs) |
        static_cast<std::underlying_type<GLWindow::Event>::type>(rhs));
}

constexpr GLWindow::Event operator&(GLWindow::Event lhs, GLWindow::Event rhs)
{
    return static_cast<GLWindow::Event>(
        static_cast<std::underlying_type<GLWindow::Event>::type>(lhs) &
        static_cast<std::underlying_type<GLWindow::Event>::type>(rhs));
}

constexpr bool operator==(GLWindow::Event lhs, GLWindow::Event rhs)
{
    return static_cast<std::underlying_type<GLWindow::Event>::type>(lhs) ==
           static_cast<std::underlying_type<GLWindow::Event>::type>(rhs);
}

#endif // OPENGLCPPWRAPPER_UI_WINDOW_HPP
