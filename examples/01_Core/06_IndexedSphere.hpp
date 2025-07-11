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

#ifndef EXAMPLE_07_INDEXED_SPHERE_HPP
#define EXAMPLE_07_INDEXED_SPHERE_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \file this example paints a sphere made of points. For drawing the VAO we
//! pass an index of vertices to OpenGL.
//!
//! \note This example takes its inspiration from
//! https://github.com/invor/simplestGraphRendering
//------------------------------------------------------------------------------
class IndexedSphere: public GLWindow
{
public:

    IndexedSphere(uint32_t const width,
                  uint32_t const height,
                  const char* title);
    ~IndexedSphere();

    static std::string info()
    {
        return "Render an indexed sphere";
    }

private:

    bool createSphere();
    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader m_vertex_shader;
    GLFragmentShader m_fragment_shader;
    GLVAO32 m_sphere;
    GLProgram m_prog;
    Transformable<float, 3U> m_transformable;
};

#endif // EXAMPLE_07_INDEXED_SPHERE_HPP
