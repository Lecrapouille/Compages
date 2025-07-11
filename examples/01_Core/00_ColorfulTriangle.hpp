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

#ifndef EXAMPLE_00_COLORFUL_TRIANGLE_HPP
#define EXAMPLE_00_COLORFUL_TRIANGLE_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief This example shows how to create a simple colorful triangle and how
//! we can modify VBO data from CPU and how dirty data are automatically
//! transfered to GPU.
//------------------------------------------------------------------------------
class ColorfulTriangle: public GLWindow
{
public:

    ColorfulTriangle(uint32_t const width,
                     uint32_t const height,
                     const char* title);
    ~ColorfulTriangle();

    static std::string info()
    {
        return "Render a colorful triangle";
    }

private:

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    //! \brief A GL*x*Shader holds the GLSL code source and compile it (in
    //! future it will may offer macro processing features).
    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;

    //! \brief A GLProgram holds at least two GLxShaders (vertex and fragment)
    //! and links them. Once compiled, a GLProgram holds internally and in
    //! privacy locations of shader uniforms, shader attributes and texture
    //! samplers.
    GLProgram m_prog;

    //! \brief A GLVAO can be considered as the OpenGL handler to your 3D model:
    //! it is an interface between CPU data and GPU data by holding VBOs and
    //! textures. You can modify data of textures and data of VBOs: the API will
    //! automatically transfer modified values to your graphical card (VBOs,
    //! samplers).
    GLVAO m_triangle;
};

#endif // EXAMPLE_00_COLORFUL_TRIANGLE_HPP
