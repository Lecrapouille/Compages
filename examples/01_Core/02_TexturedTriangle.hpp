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

#ifndef EXAMPLE_02_TEXTURED_TRIANGLE_HPP
#define EXAMPLE_02_TEXTURED_TRIANGLE_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief This example shows how to create a simple textured triangle.
//------------------------------------------------------------------------------
class TexturedTriangle: public GLWindow
{
public:

    TexturedTriangle(uint32_t const width,
                     uint32_t const height,
                     const char* title);
    ~TexturedTriangle();

    static std::string info()
    {
        return "Render a multi-textured triangle";
    }

private:

    bool loadTextures();
    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;
    GLProgram m_prog;
    GLVAO m_triangle;
};

#endif // EXAMPLE_02_TEXTURED_TRIANGLE_HPP
