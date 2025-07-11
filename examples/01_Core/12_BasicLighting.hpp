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

#ifndef EXAMPLE_12_BASIC_LIGHTING_HPP
#define EXAMPLE_12_BASIC_LIGHTING_HPP

#include "../debug.hpp"
#include "OpenGLCppWrapper/OpenGLCppWrapper.hpp"

// *****************************************************************************
//! \brief This example shows how to create a basic light.
// *****************************************************************************
class BasicLighting: public GLWindow
{
public:

    BasicLighting(uint32_t const width,
                  uint32_t const height,
                  const char* title);
    ~BasicLighting();

    static std::string info()
    {
        return "Basic lighting";
    }

private:

    bool createCube();
    bool createLamp();
    void changeLightProperties(float const time);

    virtual bool onSetup() override;
    virtual bool onPaint() override;
    virtual void onSetupFailed(std::string const& reason) override;
    virtual void onPaintFailed(std::string const& reason) override;
    virtual void onWindowResized() override;

private:

    GLVertexShader m_vs1, m_vs2;
    GLFragmentShader m_fs1, m_fs2;
    GLVAO m_cube;
    GLVAO m_lamp;
    GLProgram m_prog_cube;
    GLProgram m_prog_lamp;
};

#endif // EXAMPLE_12_BASIC_LIGHTING_HPP
