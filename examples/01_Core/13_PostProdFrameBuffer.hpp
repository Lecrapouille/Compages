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

#ifndef EXAMPLE_10_POSTPROD_FRAMEBUFFER_HPP
#define EXAMPLE_10_POSTPROD_FRAMEBUFFER_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

//------------------------------------------------------------------------------
//! \file This examples show a simple cube draw in a framebuffer.
//------------------------------------------------------------------------------
class PostProdFrameBuffer: public GLWindow
{
public:

    PostProdFrameBuffer(uint32_t const width,
                        uint32_t const height,
                        const char* title);
    ~PostProdFrameBuffer();

    static std::string info()
    {
        return "Post Production with Framebuffer";
    }

private:

    bool firstProgram();
    bool secondProgram();

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLFrameBuffer m_fbo;

    // First program (scene)

    GLVertexShader m_vs_scene;
    GLFragmentShader m_fs_scene;
    GLProgram m_prog_scene;
    GLVAO m_cube;
    GLVAO m_floor;

    // Second program (screen)

    GLVertexShader m_vs_screen;
    GLFragmentShader m_fs_screen;
    GLProgram m_prog_screen;
    GLVAO m_screen;
};

#endif // EXAMPLE_13_POSTPROD_FRAMEBUFFER_HPP
