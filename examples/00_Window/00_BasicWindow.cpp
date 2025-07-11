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

#include "00_BasicWindow.hpp"
#include <iostream>

//------------------------------------------------------------------------------
//! \brief Constructor. You need to pass the window size and title to base
//! Window.
//------------------------------------------------------------------------------
BasicWindow::BasicWindow(uint32_t const width,
                         uint32_t const height,
                         const char* title)
    : GLWindow(width, height, title)
{
    std::cout << "Hello BasicWindow: " << info() << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Destructor. Do nothing.
//------------------------------------------------------------------------------
BasicWindow::~BasicWindow()
{
    std::cout << std::endl; // flush the \r char
    std::cout << "Bye BasicWindow" << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Callback method triggered by GLWindow. Add in this method C++/OpenGL
//! code to initialize graphics and states for your application.
//------------------------------------------------------------------------------
bool BasicWindow::onSetup()
{
    std::cout << "Setup BasicWindow" << std::endl;

    // Three cases:
    // - setup ends with success => return true. In this case the function
    // onPaint()
    //   will be called periodically by GLWindow.
    // - setup reaches an error => return false. In case, the onPaint() function
    // will
    //   not be called and instead the function onSetupFailed() is called and
    //   the program immediatley exit (and release CPU and GPU memory before).
    // - Throw GL::Exception it will be catck by GLWindow and onSetupFailed()
    //   will be called before ending the application.
    return true;
}

//------------------------------------------------------------------------------
//! \brief Callback when onSetup() failed.
//------------------------------------------------------------------------------
void BasicWindow::onSetupFailed(std::string const& reason)
{
    // To reach this code make onSetup() return false or throw an GL::Exception.
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Callback method triggered by GLWindow. Add in this method C++/OpenGL
//! code to paint your scene. Note that swap buffer are called automaticaly by
//! GLProgram, so you do not have to call it.
//------------------------------------------------------------------------------
bool BasicWindow::onPaint()
{
    // The delta time (in seconds) from the previous onPaint() and frames per
    // seconds (FPS) are automatically computed.
    std::cout << "Delta time: " << uint32_t(dt() * 1000.0f) << " ms."
              << "   FPS: " << fps() << "        \r" << std::flush;

    // Change the background color over time

    // Cumulate the time
    static float time = 0.0f;
    time += dt();

    // Note: this is not mandatory but you can wrap each OpenGL function with
    // the glCheck() to verify if everything is alright.
    float ct = cosf(time) * 0.5f + 0.5f;
    float st = sinf(time) * 0.5f + 0.5f;
    glCheck(glClearColor(st, ct, 0.0f, 1.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    // Three cases:
    // - onPaint() ends with success => return true. In this case the function
    //   onPaint() will be keep be called periodically by GLWindow.
    // - onPaint() reaches an error => return false. In case, the next onPaint()
    //   function will not be called and instead the function onDrawFailed() is
    //   called and the program immediatley exit (and release CPU and GPU memory
    //   before).
    // - Throw GL::Exception it will be catck by GLWindow and onPaintFailed()
    //   will be called before ending the application.
    return true;
}

//------------------------------------------------------------------------------
//! \brief Callback when onPaint() failed.
//------------------------------------------------------------------------------
void BasicWindow::onPaintFailed(std::string const& reason)
{
    // To reach this code make onPaint() return false or throw an GL::Exception.
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
