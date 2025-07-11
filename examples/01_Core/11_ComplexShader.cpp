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

#include "11_ComplexShader.hpp"
#include <iostream>

//------------------------------------------------------------------------------
ComplexShader::ComplexShader(uint32_t const width,
                             uint32_t const height,
                             const char* title)
    : GLWindow(width, height, title), m_quad("VAO_quad"), m_prog("Prog")
{
    std::cout << "Hello ComplexShader:" << info() << std::endl;
}

//------------------------------------------------------------------------------
ComplexShader::~ComplexShader()
{
    std::cout << "Bye ComplexShader" << std::endl;
}

//------------------------------------------------------------------------------
void ComplexShader::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
}

//------------------------------------------------------------------------------
//! \brief Initialize shader and create a quad.
//------------------------------------------------------------------------------
bool ComplexShader::onSetup()
{
    // Load vertex and fragment shaders with GLSL code.
    m_vertex_shader.read("01_Core/shaders/11_ComplexShader.vs");
    m_fragment_shader.read("01_Core/shaders/11_ComplexShader.fs");

    // Compile the shader program
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    // Create a quad.
    if (!m_prog.bind(m_quad))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    // Fill VBOs of the VAO: init quad vertex positions.
    m_quad.vector3f("position") = { Vector3f(1.0f, 1.0f),
                                    Vector3f(1.0f, -1.0f),
                                    Vector3f(-1.0f, -1.0f),
                                    Vector3f(-1.0f, 1.0f) };

    // Fill VBOs of the VAO: init texture positions.
    m_quad.vector2f("uv") = { Vector2f(1.0f, 1.0f),
                              Vector2f(1.0f, 0.0f),
                              Vector2f(0.0f, 0.0f),
                              Vector2f(0.0f, 1.0f) };

    // Vertices index of the quad
    m_quad.index() = {
        0u, 1u, 3u, // first triangle
        1u, 2u, 3u  // second triangle
    };

    settings();

    return true;
}

//------------------------------------------------------------------------------
//! \brief Initialize seetings of the shader.
//------------------------------------------------------------------------------
void ComplexShader::settings()
{
    m_prog.vector3f("color") = Vector3f(1.0f, 1.0f, 1.0f);
    m_prog.scalarf("speed") = 0.0001f;
    m_prog.scalarf("brightness") = 0.0018f;
    m_prog.scalarf("distfading") = 0.7f;
    m_prog.scalarf("twinkleSpeed") = 200.0f;
}

//------------------------------------------------------------------------------
//! \brief Paint our scene. Here we are using the delta time to
//------------------------------------------------------------------------------
bool ComplexShader::onPaint()
{
    static float time = 0.0f;
    time += dt();

    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    m_prog.scalarf("time") = time;
    if (!m_quad.draw(Mode::TRIANGLES))
    {
        std::cerr << "Quad not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void ComplexShader::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the onSetup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void ComplexShader::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
