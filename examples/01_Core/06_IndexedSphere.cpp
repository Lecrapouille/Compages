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

#include "06_IndexedSphere.hpp"
#include <iostream>

//------------------------------------------------------------------------------
IndexedSphere::IndexedSphere(uint32_t const width,
                             uint32_t const height,
                             const char* title)
    : GLWindow(width, height, title), m_sphere("sphere"), m_prog("prog")
{
    std::cout << "Hello IndexedSphere: " << info() << std::endl;
}

//------------------------------------------------------------------------------
IndexedSphere::~IndexedSphere()
{
    std::cout << "Bye IndexedSphere" << std::endl;
}

//------------------------------------------------------------------------------
void IndexedSphere::onWindowResized()
{
    float ratio = width<float>() / height<float>();
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
    m_prog.matrix44f("projection") =
        matrix::perspective(60.0_deg, ratio, 0.1f, 10.0f);
}

//------------------------------------------------------------------------------
//! \brief Create a VBO and a EBO (Element Buffer Object).
//------------------------------------------------------------------------------
bool IndexedSphere::createSphere()
{
    constexpr float radius = 2.0f;
    constexpr uint32_t NbPointsLon = 100u;
    constexpr uint32_t NbPointsLat = 100u;
    constexpr float stepLon = 360.0f / static_cast<float>(NbPointsLon);
    constexpr float stepLat = 180.0f / static_cast<float>(NbPointsLat);
    constexpr float PI =
        3.141592653589793238462643383279502884197169399375105820f;
    float latitude = -90.0f;
    float longitude = -180.0f;

    // Bind the VAO to the program. This will populate VBOs.
    // Get the reference of the desired VBO because vector3f()
    // is not a faster method.
    if (!m_prog.bind(m_sphere))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }
    auto& positions = m_sphere.vector3f("position");

    positions.reserve(NbPointsLon * NbPointsLat);
    for (uint32_t i = 0; i < NbPointsLon; ++i)
    {
        for (uint32_t j = 0; j < NbPointsLat; ++j)
        {
            float lat = (PI / 180.0f) * latitude;
            float lon = (PI / 180.0f) * longitude;
            float lat_sin = sinf(lat);
            float lon_sin = sinf(lon);
            float lat_cos = cosf(lat);
            float lon_cos = cosf(lon);

            positions.append(Vector3f(lon_sin * lat_cos * radius,
                                      lat_sin * radius,
                                      lat_cos * lon_cos * radius));

            longitude += stepLon;
        }
        latitude += stepLat;
    }

    // Create the EBO (index for vertices) and return its reference.
    // Fill the index with values.
    GLElementBuffer<uint32_t>& indices = m_sphere.index();
    indices.reserve(NbPointsLon * NbPointsLat);
    uint32_t i = NbPointsLon * NbPointsLat;
    while (i--)
    {
        indices.append(i);
    }

    return true;
}

//------------------------------------------------------------------------------
bool IndexedSphere::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));

    m_vertex_shader.read("01_Core/shaders/06_IndexedSphere.vs");
    m_fragment_shader.read("01_Core/shaders/06_IndexedSphere.fs");
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    m_prog.matrix44f("model") = m_transformable.matrix();
    m_prog.matrix44f("view") =
        matrix::lookAt(Vector3f(3, 3, 3), Vector3f(1, 1, 1), Vector3f(0, 1, 0));

    if (!createSphere())
        return false;

    // Helper for debugging states of your program
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_sphere);

    return true;
}

//------------------------------------------------------------------------------
bool IndexedSphere::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    if (!m_sphere.draw(Mode::POINTS))
    {
        std::cerr << "Sphere not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void IndexedSphere::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void IndexedSphere::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
