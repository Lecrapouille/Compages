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

#ifndef EXAMPLE_09_SKYBOX_TEXTURECUBE_HPP
#define EXAMPLE_09_SKYBOX_TEXTURECUBE_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \brief this example renders a skybox. A skybox is cubic texture simuling a
//! infinite landscape.
//!
//! \note This example takes its inspiration from:
//! https://learnopengl.com/Advanced-OpenGL/Cubemaps
//------------------------------------------------------------------------------
class SkyBoxTextureCube: public GLWindow
{
public:

    SkyBoxTextureCube(uint32_t const width,
                      uint32_t const height,
                      const char* title);
    ~SkyBoxTextureCube();

    static std::string info()
    {
        return "Render a basic skybox (texture cube)";
    }

private:

    bool createSkyBox();
    bool drawSkyBox();
    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader vs1;
    GLFragmentShader fs1;
    GLVAO m_skybox;
    GLProgram m_prog;
};

#endif // EXAMPLE_09_SKYBOX_TEXTURECUBE_HPP
