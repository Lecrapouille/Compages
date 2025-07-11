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

#ifndef EXAMPLE_03_MULTI_TEXTURED_SQUARE_HPP
#define EXAMPLE_03_MULTI_TEXTURED_SQUARE_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief This example shows how to create a simple multi-textured triangle.
//! This method maybe be used to create terrain textures.
//!
//! \note This example takes its inspiration from the Youtube video
//! "OpenGL 3D Game Tutorial 17: Multitexturing" made by ThinMatrix
//! https://youtu.be/-kbal7aGUpk
//------------------------------------------------------------------------------
class MultiTexturedSquare: public GLWindow
{
public:

    MultiTexturedSquare(uint32_t const width,
                        uint32_t const height,
                        const char* title);
    ~MultiTexturedSquare();

    static std::string info()
    {
        return "Render a textured triangle";
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
    GLVAO m_square;
};

#endif // EXAMPLE_03_MULTI_TEXTURED_SQUARE_HPP
