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

#include "07_MultipleObjects.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

//------------------------------------------------------------------------------
MultipleObjects::MultipleObjects(uint32_t const width,
                                 uint32_t const height,
                                 const char* title)
    : GLWindow(width, height, title),
      m_cube1("cube1"),
      m_cube2("cube2"),
      m_floor("floor"),
      m_prog("prog")
{
    std::cout << "Hello MultipleObjects" << std::endl;
}

//------------------------------------------------------------------------------
MultipleObjects::~MultipleObjects()
{
    std::cout << "Bye MultipleObjects" << std::endl;
}

//------------------------------------------------------------------------------
void MultipleObjects::onWindowResized()
{
    float ratio = width<float>() / height<float>();
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
    m_prog.matrix44f("projection") =
        matrix::perspective(60.0_deg, ratio, 0.1f, 100.0f);
}

//------------------------------------------------------------------------------
//! \brief Create a cube.
//------------------------------------------------------------------------------
bool MultipleObjects::initCube(Shape& cube, const char* texture_path)
{
    m_prog.bind(cube.vao);

    // Fill VBOs of the VAO: init vertex positions.
    cube.vao.vector3f("position") = {
#include "geometry/cube_position.txt"
    };

    // Fill VBOs of the VAO: init texture positions.
    cube.vao.vector2f("UV") = {
#include "geometry/cube_texture.txt"
    };

    // Apply a texture
    GLTexture2D& texture = cube.vao.texture2D("texID");
    static int i = 0; // Randomize texture settings
    if (++i % 2 == 0)
    {
        texture.interpolation(GLTexture::Minification::LINEAR,
                              GLTexture::Magnification::LINEAR);
        texture.wrap(GLTexture::Wrap::CLAMP_TO_EDGE);
    }
    else
    {
        texture.interpolation(GLTexture::Minification::NEAREST,
                              GLTexture::Magnification::NEAREST);
        texture.wrap(GLTexture::Wrap::MIRRORED_REPEAT);
    }
    return texture.load<SOIL>(texture_path);
}

//------------------------------------------------------------------------------
//! \brief Create a floor.
//------------------------------------------------------------------------------
bool MultipleObjects::initFloor(Shape& floor, const char* texture_path)
{
    // Create VBOs of the VAO.
    m_prog.bind(floor.vao);

    // Fill VBOs of the VAO: init vertex positions.
    floor.vao.vector3f("position") = {
#include "geometry/floor_position.txt"
    };

    // Fill VBOs of the VAO: init texture positions.
    floor.vao.vector2f("UV") = {
#include "geometry/floor_texture.txt"
    };

    // Repeat the texture pattern
    floor.vao.vector2f("UV") *= 4.0f;

    GLTexture2D& texture = floor.vao.texture2D("texID");
    texture.interpolation(GLTexture::Minification::LINEAR,
                          GLTexture::Magnification::LINEAR);
    texture.wrap(GLTexture::Wrap::MIRRORED_REPEAT);
    return texture.load<SOIL>(texture_path);
}

//------------------------------------------------------------------------------
//! \brief Load vertex and fragment shaders. Create two VAOs and fill their VBOs
//! and textures. Note that examples of EBO (aka vertice index) are shown in
//! next examples.
//------------------------------------------------------------------------------
bool MultipleObjects::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    m_vertex_shader.read("01_Core/shaders/07_MultipleObjects.vs");
    m_fragment_shader.read("01_Core/shaders/07_MultipleObjects.fs");

    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }
    debug(m_prog);

    // Init shader uniforms
    m_prog.scalarf("scale") = 1.0f;
    m_prog.matrix44f("model") = Matrix44f(matrix::Identity);
    m_prog.matrix44f("view") =
        matrix::lookAt(Vector3f(3, 3, 3), Vector3f(0, 0, 0), Vector3f(0, 1, 0));

    // Create 2 VAOs from the same shader program: one cube and one plane.
    bool res = initFloor(m_floor, "external/assets/wooden-crate.jpg") &&
               initCube(m_cube1, "external/assets/hazard.png") &&
               initCube(m_cube2, "external/assets/path.png");

    // Helper for debugging states of your program
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_floor.vao);
    debug(m_cube1.vao);
    debug(m_cube2.vao);

    return res;
}

//------------------------------------------------------------------------------
//! \brief Paint our scene. We are using the delta time to spin one cube. We
//! show different ways to draw VAO.
//------------------------------------------------------------------------------
bool MultipleObjects::onPaint()
{
    static float time = 0.0f;
    time += dt();

    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Draw the first cube from the first VAO. Make it spining around its
    // Y-axis. Apply a "pinkished" coloration.
    m_prog.vector4f("color") = Vector4f(0.8f, 0.2f, 0.8f, 0.8f);
    m_cube1.transform.reset();
    m_cube1.transform.yaw(
        units::angle::radian_t(cosf(time))); // Apply a rotation around Y-axis
    m_cube1.transform.position(
        Vector3f(-1.0f, 0.0f, -1.0f)); // Apply a translation
    m_prog.matrix44f("model") =
        m_cube1.transform.matrix();           // Rotate and translate the cube
    m_cube1.vao.draw(Mode::TRIANGLES, 0, 36); // Style 01: pass all parameters

    // Draw the same cube (same model => same VAO) at different and fixed
    // position. Apply a "darkished" coloration.
    m_prog.vector4f("color") = Vector4f(0.2f, 0.2f, 0.2f, 0.2f);
    m_cube1.transform.reset();
    m_cube1.transform.position(
        Vector3f(-1.0f, 0.0f, 2.0f)); // Apply a translation
    m_prog.matrix44f("model") = m_cube1.transform.matrix();
    m_cube1.vao.draw(); // Style 02: do not pass first and count vertices

    // Draw a third cube from the second VAO.
    // Apply a "darkished" coloration and let this cube static (no motion).
    m_prog.vector4f("color") = Vector4f(0.4f, 0.4f, 0.5f, 0.2f);
    m_cube2.transform.reset();
    m_cube2.transform.position(
        Vector3f(2.0f, 0.0f, -2.0f)); // Apply a translation
    m_prog.matrix44f("model") = m_cube2.transform.matrix();
    m_cube2.vao.draw(); // Style 02: do not pass first and count vertices

    // Draw the floor (second model => third VAO).
    m_prog.vector4f("color") = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    m_floor.transform.reset();
    m_floor.transform.position(
        Vector3f(0.0f, 0.0f, 0.0f)); // Apply a translation
    m_prog.matrix44f("model") = m_floor.transform.matrix();
    m_floor.vao.draw(
        Mode::TRIANGLES,
        0,
        6); // Style 03: do not pass implict bound VAO and no vertices count

    return true;
}

//------------------------------------------------------------------------------
void MultipleObjects::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void MultipleObjects::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
