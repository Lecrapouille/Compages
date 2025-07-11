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

#include "01_BasicWindowIOEvents.hpp"
#include <iostream>

//------------------------------------------------------------------------------
BasicWindowIOEvents::BasicWindowIOEvents(uint32_t const width,
                                         uint32_t const height,
                                         const char* title)
    : GLWindow(width, height, title)
{
    std::cout << "Hello BasicWindowIOEvents: " << info() << std::endl;
}

//------------------------------------------------------------------------------
BasicWindowIOEvents::~BasicWindowIOEvents()
{
    std::cout << std::endl; // flush the \r char
    std::cout << "Bye BasicWindowIOEvents" << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Callback when the window has been resized. Use template methods
//! width<T>(), height<T>() to get windows inforamtion where T is the
//! destination type (float, double, int). Note that height is never zero !
//------------------------------------------------------------------------------
void BasicWindowIOEvents::onWindowResized()
{
    // Make the viewport matches the new window dimensions.
    glCheck(glViewport(0, 0, GLWindow::width<int>(), GLWindow::height<int>()));

    // Note: height is never zero !
    float ratio = GLWindow::width<float>() / GLWindow::height<float>();

    std::cout << "New Window dimension " << width<int>() << " x "
              << height<int>() << ". Ratio is " << ratio << std::endl;
}

// -----------------------------------------------------------------------------
//! \brief Keyboard pressed or released events. Note the escape key is already
//! map to kill the application but you can override shouldHalt() to reuse this
//! key).
// -----------------------------------------------------------------------------
void BasicWindowIOEvents::onKeyboardEvent()
{
    if (GLWindow::isKeyDown(GLFW_KEY_F1))
        std::cout << std::endl << "Key pressed F1" << std::endl;
    if (GLWindow::isKeyUp(GLFW_KEY_F1))
        std::cout << std::endl << "Key released F1" << std::endl;
    if (GLWindow::wasKeyReleased(GLFW_KEY_F1))
        std::cout << std::endl << "Key was released F1" << std::endl;
    if (GLWindow::wasKeyPressed(GLFW_KEY_F1))
        std::cout << std::endl << "Key was pressed F1" << std::endl;

    if (GLWindow::isKeyDown(GLFW_KEY_A))
        std::cout << std::endl << "Key pressed A" << std::endl;
    if (GLWindow::isKeyUp(GLFW_KEY_A))
        std::cout << std::endl << "Key released A" << std::endl;
    if (GLWindow::wasKeyReleased(GLFW_KEY_A))
        std::cout << std::endl << "Key was released A" << std::endl;
    if (GLWindow::wasKeyPressed(GLFW_KEY_A))
        std::cout << std::endl << "Key was pressed A" << std::endl;
}

// -----------------------------------------------------------------------------
//! \brief Callback on mouse pressed or release event. In this example show the
//! mouse cursor when the mouse is pressed or hide the cursor when the mouse is
//! released.
// -----------------------------------------------------------------------------
void BasicWindowIOEvents::onMouseButtonPressed()
{
    std::cout << "Mouse button " << static_cast<int>(mouse().button) << " was "
              << (mouse().pressed ? "pressed" : "released") << std::endl;

    static bool state = true;
    state ^= true;
    (false == state) ? showCursor() : hideCursor();
}

// -----------------------------------------------------------------------------
//! \brief Callback on mouse movement event.
// -----------------------------------------------------------------------------
void BasicWindowIOEvents::onMouseMoved()
{
    std::cout << "Mouse moved to: "
              << static_cast<int>(GLWindow::mouse().position.x) << " x "
              << static_cast<int>(GLWindow::mouse().position.y) << std::endl;

    std::cout << "Mouse delta movement: " << mouse().displacement.x << " x "
              << mouse().displacement.y << std::endl;
}

// -----------------------------------------------------------------------------
//! \brief Callback on mouse scroll event.
// -----------------------------------------------------------------------------
void BasicWindowIOEvents::onMouseScrolled()
{
    std::cout << "Mouse scrolled: " << mouse().scroll.x << " x "
              << mouse().scroll.y << std::endl;
}

//------------------------------------------------------------------
//! \brief Display the GPU memory usage. triggered when new data
//! are sent to the GPU.
//------------------------------------------------------------------
void BasicWindowIOEvents::onGPUMemoryChanged(size_t size)
{
    std::cout << "Estimated GPU memory usage: " << size << " bytes"
              << std::endl;
}

//------------------------------------------------------------------
bool BasicWindowIOEvents::onSetup()
{
    // Enable IO callbacks. Without this method: no events are triggered.
    reactTo(Event::MouseMove | Event::MouseScroll | Event::MouseButton |
            Event::Keyboard);

    // Do not show the mouse cursor.
    hideCursor();

    // Success
    return true;
}

//------------------------------------------------------------------
bool BasicWindowIOEvents::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    // Success
    return true;
}

//------------------------------------------------------------------------------
void BasicWindowIOEvents::onSetupFailed(std::string const& reason)
{
    // To reach this code make setup() return false;
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void BasicWindowIOEvents::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
