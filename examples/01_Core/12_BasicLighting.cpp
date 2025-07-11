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

#include "12_BasicLighting.hpp"
#include <cstdlib>
#include <iostream>

// -----------------------------------------------------------------------------
BasicLighting::BasicLighting(uint32_t const width,
                             uint32_t const height,
                             const char* title)
    : GLWindow(width, height, title),
      m_cube("VAO_cube"),
      m_lamp("VAO_lamp"),
      m_prog_cube("Prog_cube"),
      m_prog_lamp("Prog_lamp")
{
    std::cout << "Hello BasicLighting: " << info() << std::endl;
}

// -----------------------------------------------------------------------------
BasicLighting::~BasicLighting()
{
    std::cout << "Bye BasicLighting" << std::endl;
}

// -----------------------------------------------------------------------------
void BasicLighting::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    Matrix44f const& proj = matrix::perspective(
        60.0_deg, width<float>() / height<float>(), 0.1f, 100.0f);
    m_prog_cube.matrix44f("projection") = proj;
    m_prog_lamp.matrix44f("projection") = proj;
}

static Vector3f lightPos = Vector3f(1.2f, 1.0f, 2.0f);
static Vector3f lookat = Vector3f(1, 1, 1);
static Matrix44f view =
    matrix::lookAt(Vector3f(5, 5, 5), lookat, Vector3f(0, 1, 0));

//------------------------------------------------------------------------------
//! \brief Create a cube for the lamp.
//------------------------------------------------------------------------------
bool BasicLighting::createLamp()
{
    m_vs1.read("01_Core/shaders/12_BasicLighting_lamp.vs");
    m_fs1.read("01_Core/shaders/12_BasicLighting_lamp.fs");

    if (!m_prog_lamp.compile(m_vs1, m_fs1))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog_lamp.strerror() << "'" << std::endl;
        return false;
    }

    // Draw a cube on the light position
    if (!m_prog_lamp.bind(m_lamp))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog_lamp.strerror()
                  << "'" << std::endl;
        return false;
    }
    m_lamp.vector3f("position") = {
#include "geometry/cube_position.txt"
    };

    Transformable<float, 3U> transform;
    transform.reset();
    transform.position(lightPos);
    // transform.scale(Vector3f(0.05f)); // a smaller cube
    m_prog_lamp.matrix44f("model") = transform.matrix();
    m_prog_lamp.matrix44f("view") = view;

    return true;
}

//------------------------------------------------------------------------------
//! \brief Create a cube with material.
//------------------------------------------------------------------------------
bool BasicLighting::createCube()
{
    // Load vertex and fragment shaders with GLSL code.
    m_vs2.read("01_Core/shaders/12_BasicLighting_material.vs");
    m_fs2.read("01_Core/shaders/12_BasicLighting_material.fs");

    // Compile the shader program
    if (!m_prog_cube.compile(m_vs2, m_fs2))
    {
        std::cerr << "failed compiling OpenGL program. Reason was '"
                  << m_prog_cube.strerror() << "'" << std::endl;
        return false;
    }

    // Create a cube.
    if (!m_prog_cube.bind(m_cube))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog_cube.strerror()
                  << "'" << std::endl;
        return false;
    }
    m_cube.vector3f("position") = {
#include "geometry/cube_position.txt"
    };
    m_cube.vector3f("normal") = {
#include "geometry/cube_normals.txt"
    };

    Transformable<float, 3U> transform;
    transform.reset();
    m_prog_cube.matrix44f("model") = transform.matrix();
    m_prog_cube.matrix44f("view") = view;
    m_prog_cube.vector3f("viewPos") = lightPos;

    // Material properties
    // Note: specular lighting doesn't have full effect on this object's
    // material
    m_prog_cube.vector3f("material.ambient") = Vector3f(1.0f, 0.5f, 0.31f);
    m_prog_cube.vector3f("material.diffuse") = Vector3f(1.0f, 0.5f, 0.31f);
    m_prog_cube.vector3f("material.specular") = Vector3f(0.5f, 0.5f, 0.5f);
    m_prog_cube.scalarf("material.shininess") = 32.0f;

    return true;
}

//------------------------------------------------------------------------------
//! \brief Change the light settings.
//------------------------------------------------------------------------------
void BasicLighting::changeLightProperties(float const random)
{
    Vector3f lightColor;
    lightColor.x = std::sin(random * 2.0f);
    lightColor.y = std::sin(random * 0.7f);
    lightColor.z = std::sin(random * 1.3f);

    Vector3f diffuseColor = vector::componentProduct(
        lightColor, Vector3f(0.5f)); // decrease the influence
    Vector3f ambientColor =
        vector::componentProduct(diffuseColor, Vector3f(0.2f)); // low influence
    m_prog_cube.vector3f("light.ambient") = ambientColor;
    m_prog_cube.vector3f("light.diffuse") = diffuseColor;
    m_prog_cube.vector3f("light.specular") = Vector3f(1.0f, 1.0f, 1.0f);
    m_prog_cube.vector3f("light.position") = lightPos;
}

//------------------------------------------------------------------------------
//! \brief Initialize shader and create a quad.
//------------------------------------------------------------------------------
bool BasicLighting::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));

    if (!createLamp())
        return false;
    if (!createCube())
        return false;

    changeLightProperties(float(std::rand() % 10));

    return true;
}

//------------------------------------------------------------------------------
//! \brief Paint our scene.
//------------------------------------------------------------------------------
bool BasicLighting::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    if (!m_cube.draw(Mode::TRIANGLES, 0u, 36u))
    {
        std::cerr << "Cube not renderered" << std::endl;
        return false;
    }
    if (!m_lamp.draw(Mode::TRIANGLES, 0u, 36u))
    {
        std::cerr << "Lamp not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void BasicLighting::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void BasicLighting::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
