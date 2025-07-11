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

#ifndef OPENGLCPPWRAPPER_EDITOR_HPP
#define OPENGLCPPWRAPPER_EDITOR_HPP

#include <OpenGLCppWrapper/OpenGLCppWrapper.hpp>

//------------------------------------------------------------------------------
//! \brief This example shows how to add and use Dear ImGui widgets.
//! For more information see: https://github.com/ocornut/imgui
//------------------------------------------------------------------------------
class Editor: public GLWindow
{
public:

    Editor(uint32_t const width, uint32_t const height, const char* title);
    ~Editor();

private:

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;

private:

    //--------------------------------------------------------------------------
    //! \brief Sub class managing Dear ImGui context and allowing to draw
    //! widgets.
    //! \note this class can be place outside \c Editor
    //--------------------------------------------------------------------------
    class GUI: public DearImGuiLayer
    {
    public:

        GUI(Editor& window) : DearImGuiLayer(window, "GUI") {}

    private:

        virtual bool onImGuiRender() override;
    };

private:

    // \brief Background color changed through DearImGui buttons
    GLTexture2D m_texture;
    static float color[4];
};

#endif // OPENGLCPPWRAPPER_EDITOR_HPP
