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

#include "04_IndexedQuad.hpp"
#include <iostream>

//------------------------------------------------------------------------------
IndexedQuad::IndexedQuad(uint32_t const width,
                         uint32_t const height,
                         const char* title)
    : GLWindow(width, height, title), m_prog("Prog"), m_box("Box")
{
    std::cout << "Hello IndexedQuad " << info() << std::endl;
}

//------------------------------------------------------------------------------
IndexedQuad::~IndexedQuad()
{
    std::cout << "Bye IndexedQuad" << std::endl;
}

//------------------------------------------------------------------------------
void IndexedQuad::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
}

//------------------------------------------------------------------------------
bool IndexedQuad::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_vertex_shader << "#version 330 core                        \n"
                       "in vec3      position;                   \n"
                       "in vec3      color;                      \n"
                       "out struct v2f_s {                       \n"
                       "  vec3      color;                       \n"
                       "} v2f;                                   \n"
                       "void main() {                            \n"
                       "  v2f.color = color;                     \n"
                       "  gl_PointSize = 10.0;                   \n"
                       "  gl_Position = vec4(position, 1.0);     \n"
                       "}";

    m_fragment_shader << "#version 330 core                        \n"
                         "in struct v2f_s {                        \n"
                         "  vec3 color;                            \n"
                         "} v2f;                                   \n"
                         "out vec4 fragColor;                      \n"
                         "void main() {                            \n"
                         "    fragColor = vec4(v2f.color, 1);      \n"
                         "}";

    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    if (!m_prog.bind(m_box))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    m_box.vector3f("position") = {
        Vector3f(-0.5f, 0.5f, 0.5f),   // Front Top Left            - Red   - 0
        Vector3f(0.5f, 0.5f, 0.5f),    // Front Top Right           - Green - 1
        Vector3f(0.5f, -0.5f, 0.5f),   // Front Bottom Right        - Blue  - 2
        Vector3f(-0.5f, -0.5f, 0.5f),  // Front Bottom Left         - Cyan  - 3
        Vector3f(-0.5f, 0.5f, -0.5f),  // Back Top Left             - Pink  - 4
        Vector3f(0.5f, 0.5f, -0.5f),   // Back Top Right            - Yellow- 5
        Vector3f(0.5f, -0.5f, -0.5f),  // Back Bottom Right         - White - 6
        Vector3f(-0.5f, -0.5f, -0.5f), // Back Bottom Left          - Gray  - 7
    };

    m_box.vector3f("color") = {
        Vector3f(1.0f, 0.0f, 0.0f), // Front Top Left               - Red   - 0
        Vector3f(0.0f, 1.0f, 0.0f), // Front Top Right              - Green - 1
        Vector3f(0.0f, 0.0f, 1.0f), // Front Bottom Right           - Blue  - 2
        Vector3f(0.0f, 1.0f, 1.0f), // Front Bottom Left            - Cyan  - 3
        Vector3f(1.0f, 0.0f, 1.0f), // Back Top Left                - Pink  - 4
        Vector3f(1.0f, 1.0f, 0.0f), // Back Top Right               - Yellow- 5
        Vector3f(0.1f, 0.1f, 0.1f), // Back Bottom Right            - White - 6
        Vector3f(1.0f, 1.0f, 1.0f), // Back Bottom Left             - Gray  - 7
    };

    m_box.index() = {
        0u, 3u, 2u,             // Front
        2u, 1u, 0u, 1u, 5u, 6u, // Right
        6u, 2u, 1u, 5u, 4u, 7u, // Left
        7u, 6u, 5u, 4u, 7u, 3u, // Back
        3u, 0u, 4u, 4u, 5u, 1u, // Top
        1u, 0u, 4u, 3u, 2u, 6u, // Bottom
        6u, 7u, 3u,
    };

    // Helper for debugging states of your program
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_box);

    return true;
}

//------------------------------------------------------------------------------
bool IndexedQuad::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
    glCheck(glEnable(GL_PROGRAM_POINT_SIZE));

    // Draw the box using the EBO. Do not pass vertices count !!
    if (!m_box.draw())
    {
        std::cerr << "Square not renderered" << std::endl;
        return false;
    }

    // Draw the box using the EBO. Do not pass vertices count !!
    if (!m_box.draw(Mode::POINTS))
    {
        std::cerr << "Square not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void IndexedQuad::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void IndexedQuad::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
