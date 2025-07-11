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

#ifndef EXAMPLE_10_SKYBOX_SHAPE_HPP
#define EXAMPLE_10_SKYBOX_SHAPE_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------
//! \file this example paints a shape inside a skybox. A skybox is
//! cubic texture simuling a infinite landscape
//!
//! \note This example takes its inspiration from
//! https://learnopengl.com/Advanced-OpenGL/Cubemaps
//------------------------------------------------------------------
class SkyBoxShape: public GLWindow
{
public:

    SkyBoxShape(uint32_t const width, uint32_t const height, const char* title);
    ~SkyBoxShape();

    static std::string info()
    {
        return "Render a shape inside of a skybox";
    }

private:

    bool createShape();
    bool drawShape();
    bool createSkyBox();
    bool drawSkyBox();
    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader vs1, vs2;
    GLFragmentShader fs1, fs2;
    GLVAO m_shape, m_skybox;
    GLProgram m_progShape, m_progSkyBox;
};

#endif // EXAMPLE_10_SKYBOX_SHAPE_HPP
