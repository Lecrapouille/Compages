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

#include "05_RotatingQuad.hpp"
#include <iostream>

//------------------------------------------------------------------------------
RotatingQuad::RotatingQuad(uint32_t const width,
                           uint32_t const height,
                           const char* title)
    : GLWindow(width, height, title), m_prog("Prog")
{
    std::cout << "Hello RotatingQuad: " << info() << std::endl;
}

//------------------------------------------------------------------------------
RotatingQuad::~RotatingQuad()
{
    std::cout << "Bye RotatingQuad" << std::endl;
}

//------------------------------------------------------------------------------
void RotatingQuad::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    // Note: height is never zero !
    float ratio = width<float>() / height<float>();

    // Init the projection matrix (shader uniforms).
    m_prog.matrix44f("projection") =
        matrix::perspective(60.0_deg, ratio, 0.1f, 10.0f);
}

//------------------------------------------------------------------------------
bool RotatingQuad::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_vertex_shader << "#version 330 core                                      "
                       "            \n"
                       "uniform mat4   model;         // Model matrix          "
                       "            \n"
                       "uniform mat4   view;          // View matrix           "
                       "            \n"
                       "uniform mat4   projection;    // Projection matrix     "
                       "            \n"
                       "in vec3        position;      // Vertex position       "
                       "            \n"
                       "in vec3        color;                                  "
                       "            \n"
                       "out struct v2f_s {                                     "
                       "            \n"
                       "  vec3      color;                                     "
                       "            \n"
                       "} v2f;                                                 "
                       "            \n"
                       "                                                       "
                       "            \n"
                       "void main()                                            "
                       "            \n"
                       "{                                                      "
                       "            \n"
                       "    v2f.color = color;                                 "
                       "            \n"
                       "    gl_Position = projection * view * model * "
                       "vec4(position, 1.0); \n"
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

    if (!m_prog.bind(m_box.vao))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    m_box.vao.vector3f("position") = {
        Vector3f(-0.5f, 0.5f, 0.5f),   // Front Top Left            - Red   - 0
        Vector3f(0.5f, 0.5f, 0.5f),    // Front Top Right           - Green - 1
        Vector3f(0.5f, -0.5f, 0.5f),   // Front Bottom Right        - Blue  - 2
        Vector3f(-0.5f, -0.5f, 0.5f),  // Front Bottom Left         - Cyan  - 3
        Vector3f(-0.5f, 0.5f, -0.5f),  // Back Top Left             - Pink  - 4
        Vector3f(0.5f, 0.5f, -0.5f),   // Back Top Right            - Yellow- 5
        Vector3f(0.5f, -0.5f, -0.5f),  // Back Bottom Right         - White - 6
        Vector3f(-0.5f, -0.5f, -0.5f), // Back Bottom Left          - Gray  - 7
    };

    m_box.vao.vector3f("color") = {
        Vector3f(1.0f, 0.0f, 0.0f), // Front Top Left               - Red   - 0
        Vector3f(0.0f, 1.0f, 0.0f), // Front Top Right              - Green - 1
        Vector3f(0.0f, 0.0f, 1.0f), // Front Bottom Right           - Blue  - 2
        Vector3f(0.0f, 1.0f, 1.0f), // Front Bottom Left            - Cyan  - 3
        Vector3f(1.0f, 0.0f, 1.0f), // Back Top Left                - Pink  - 4
        Vector3f(1.0f, 1.0f, 0.0f), // Back Top Right               - Yellow- 5
        Vector3f(0.1f, 0.1f, 0.1f), // Back Bottom Right            - White - 6
        Vector3f(1.0f, 1.0f, 1.0f), // Back Bottom Left             - Gray  - 7
    };

    m_box.vao.index() = {
        0u, 3u, 2u,             // Front
        2u, 1u, 0u, 1u, 5u, 6u, // Right
        6u, 2u, 1u, 5u, 4u, 7u, // Left
        7u, 6u, 5u, 4u, 7u, 3u, // Back
        3u, 0u, 4u, 4u, 5u, 1u, // Top
        1u, 0u, 4u, 3u, 2u, 6u, // Bottom
        6u, 7u, 3u,
    };

    // Init Model-View matrices (shader uniforms). Note that projection matrix
    // is init inside onWindowResized() which is called just after this method.
    m_prog.matrix44f("model") = m_box.transform.matrix(); // Identity matrix

    // We do not have camera yet, so fix look
    m_prog.matrix44f("view") =
        matrix::lookAt(Vector3f(3, 3, 3), Vector3f(1, 1, 1), Vector3f(0, 1, 0));

    // Helper for debugging states of your program
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_box.vao);

    return true;
}

//------------------------------------------------------------------------------
bool RotatingQuad::onPaint()
{
    static float time = 0.0f;
    time += dt();

    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Apply a rotation to the box around the Y-axis
    m_box.transform.reset(); // restore to identity matrix
    m_box.transform.yaw(units::angle::radian_t(4.0f * cosf(time)));
    m_prog.matrix44f("model") = m_box.transform.matrix();

    // Draw the box using the EBO. Do not pass vertices count or indice !!
    if (!m_box.vao.draw())
    {
        std::cerr << "Box not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void RotatingQuad::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void RotatingQuad::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
