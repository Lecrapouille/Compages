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

#ifndef EXAMPLE_01_EMPTY_WINDOW_WITH_IO_EVENTS_HPP
#define EXAMPLE_01_EMPTY_WINDOW_WITH_IO_EVENTS_HPP

#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief This example shows how to create a window for OpenGL reacting to
//! input/ouput (IO) events: keyboard pressed, mouse moved, button
//! pressed/released, scrolled, window size changed. No OpenGL objects are
//! rendered here for the moment.
//------------------------------------------------------------------------------
class BasicWindowIOEvents: public GLWindow
{
public:

    BasicWindowIOEvents(uint32_t const width,
                        uint32_t const height,
                        const char* title);
    ~BasicWindowIOEvents();

    static std::string info()
    {
        return "Dummy window reacting to window events.";
    }

private:

    // Added to this new example:

    virtual void onWindowResized() override;
    virtual void onMouseButtonPressed() override;
    virtual void onMouseMoved() override;
    virtual void onMouseScrolled() override;
    virtual void onKeyboardEvent() override;
    virtual void onGPUMemoryChanged(size_t size) override;

    // Same from the initial example:

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
};

#endif // EXAMPLE_01_EMPTY_WINDOW_WITH_IO_EVENTS_HPP
