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

#ifndef OPENGLCPPWRAPPER_UI_DEARIMGUI_HPP
#define OPENGLCPPWRAPPER_UI_DEARIMGUI_HPP

// ********************************************************************************************
//! \file GLImGUI.hpp wraps function calls of the ImGUI project.
// ********************************************************************************************

#include "UI/Layer.hpp"
#include "UI/Window.hpp"

#define IMGUI_IMPL_OPENGL_LOADER_GLEW
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wstrict-overflow"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Waggregate-return"
#pragma GCC diagnostic ignored "-Wsign-promo"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_opengl3.h"
#pragma GCC diagnostic pop

#include <iostream> // TODO temporary

// *****************************************************************************
//! \brief Class wrapper for the dear imgui library: an imediate
//! mode (im) graphical user interface (GUI) for OpenGL.
//! https://github.com/ocornut/imgui
// *****************************************************************************
class DearImGuiLayer: public Layer
{
public:

    enum class Theme
    {
        Classic,
        Dark
    };

    DearImGuiLayer(GLWindow& window, std::string const& name)
        : Layer(window, name)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Release Dear imgui allocated resources.
    //--------------------------------------------------------------------------
    ~DearImGuiLayer()
    {
        onRelease();
    }

    void setFont();
    void theme(Theme const style);
    void reactTo(GLWindow::Event const events);

private:

    //--------------------------------------------------------------------------
    //! \brief Start Dear imgui context. Use glfw routines.
    //! \param window the OpenGL window
    //--------------------------------------------------------------------------
    virtual bool onSetup() override;
    virtual bool onRelease() override;

    //--------------------------------------------------------------------------
    //! \brief Prepare the Dear imgui to draw the HMI. This will call the
    //! render() pure virtual method.
    //--------------------------------------------------------------------------
    virtual bool onPaint() override;

    void begin();
    void end();

    //--------------------------------------------------------------------------
    //! \brief Method for drawing the HMI. This has to be implemented by
    //! the derived class. Derived class shall override this method for
    //! drawing ImGUI objects.
    //! \return false if the rendering encountered an problem. As effect
    //! this will prevent GLWindow calling draw() which can react to
    //! this problem.
    //--------------------------------------------------------------------------
    virtual bool onImGuiRender() = 0;
};

#endif // OPENGLCPPWRAPPER_UI_DEARIMGUI_HPP
