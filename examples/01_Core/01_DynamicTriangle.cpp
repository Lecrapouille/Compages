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

#include "01_DynamicTriangle.hpp"
#include <iostream>

//------------------------------------------------------------------------------
DynamicTriangle::DynamicTriangle(uint32_t const width,
                                 uint32_t const height,
                                 const char* title)
    : GLWindow(width, height, title), m_prog("Prog"), m_triangle("triangle")
{
    std::cout << "Hello DynamicTriangle: " << info() << std::endl;
}

//------------------------------------------------------------------------------
DynamicTriangle::~DynamicTriangle()
{
    std::cout << "Bye DynamicTriangle" << std::endl;
}

//------------------------------------------------------------------------------
void DynamicTriangle::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
}

//------------------------------------------------------------------------------
//! \brief Vertices of the triangle.
//------------------------------------------------------------------------------
static std::vector<Vector2f> initial_position = { Vector2f(-1.0f, -1.0f),
                                                  Vector2f(1.0f, -1.0f),
                                                  Vector2f(0.0f, 1.0f) };

//------------------------------------------------------------------------------
bool DynamicTriangle::onSetup()
{
    m_vertex_shader << "#version 330 core                        \n"
                       "in vec2      position;                   \n"
                       "void main() {                            \n"
                       "  gl_Position = vec4(position, 0.0, 1.0);\n"
                       "}";

    m_fragment_shader << "#version 330 core                      \n"
                         "uniform vec3 color;                    \n"
                         "out vec4 fragColor;                    \n"
                         "void main() {                          \n"
                         "  fragColor = vec4(color, 1.0);        \n"
                         "}";

    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    if (!m_prog.bind(m_triangle))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    // Fill program uniform with a RGB color. Note "color" shall refer to the
    // variable color inside the GLSL code. If you rename it in the shader you
    // will have to rename here too. Beware name are case sensitive.
    m_prog.vector3f("color") = Vector3f(1.0f, 0.0f, 0.0f);

    // Fill VBOs of the VAO: init triangle vertex positions.
    m_triangle.vector2f("position") = initial_position;

    // debug
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_triangle);

    return true;
}

//------------------------------------------------------------------------------
//! \brief Paint our scene. Here we are using the delta time to modify positions
//! of the triangle CPU side. Dirty data are automaticaly transfered to the GPU.
//------------------------------------------------------------------------------
bool DynamicTriangle::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    // Use time for dynamicaly change values
    static float time = 0.0f;
    time += dt();
    const float c = cosf(time);
    const float ct = c * 0.5f + 0.5f;
    const float st = sinf(time) * 0.5f + 0.5f;

    // Change vertices position over time. Dirty CPU data are transmitted to
    // GPU.
    GLVertexBuffer<Vector2f>& position = m_triangle.vector2f("position");
    position = initial_position;
    position *= c;

    // Change color over time. Dirty CPU data are transmitted to GPU.
    m_prog.vector3f("color") = Vector3f(st, ct, 0.0f);

    // Draw the VAO bound to the shader program.
    if (!m_triangle.draw()) // implicit arguments: Mode::TRIANGLES, 0u, 3u
    {
        std::cerr << "Triangle not renderered" << std::endl;
    }

    return true;
}

//------------------------------------------------------------------------------
void DynamicTriangle::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void DynamicTriangle::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
