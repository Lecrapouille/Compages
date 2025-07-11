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

#ifndef EXAMPLE_02_EMPTY_WINDOW_WITH_DEAR_IMGUI_HPP
#define EXAMPLE_02_EMPTY_WINDOW_WITH_DEAR_IMGUI_HPP

#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief This example shows how to add and use Dear ImGui widgets.
//! For more information see: https://github.com/ocornut/imgui
//------------------------------------------------------------------------------
class BasicWindowImGui: public GLWindow
{
public:

    BasicWindowImGui(uint32_t const width,
                     uint32_t const height,
                     const char* title);
    ~BasicWindowImGui();

    static std::string info()
    {
        return "Window using Dear ImGui widgets";
    }

private:

    // Same from the initial example:

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;

    // Added to this new example:

private:

    //--------------------------------------------------------------------------
    //! \brief Sub class managing Dear ImGui context and allowing to draw
    //! widgets.
    //! \note this class can be place outside \c BasicWindowImGui
    //--------------------------------------------------------------------------
    class GUI: public DearImGuiLayer
    {
    public:

        GUI(BasicWindowImGui& window) : DearImGuiLayer(window, "DearIMGUI") {}

    private:

        virtual bool onImGuiRender() override;
    };

private:

    // \brief Background color changed through DearImGui buttons
    static float color[4];
};

#endif // EXAMPLE_02_EMPTY_WINDOW_WITH_DEAR_IMGUI_HPP
