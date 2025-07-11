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

#include "10_SkyBoxShape.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

//------------------------------------------------------------------------------
SkyBoxShape::SkyBoxShape(uint32_t const width,
                         uint32_t const height,
                         const char* title)
    : GLWindow(width, height, title),
      m_shape("Shape"),
      m_skybox("SkyBox"),
      m_progShape("progShape"),
      m_progSkyBox("progSkyBox")
{
    std::cout << "Hello SkyBoxShape: " << info() << std::endl;
}

//------------------------------------------------------------------------------
SkyBoxShape::~SkyBoxShape()
{
    std::cout << "Bye SkyBoxShape" << std::endl;
}

//------------------------------------------------------------------------------
void SkyBoxShape::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    Matrix44f const& proj = matrix::perspective(
        60.0_deg, width<float>() / height<float>(), 0.1f, 100.0f);
    m_progShape.matrix44f("projection") = proj;
    m_progSkyBox.matrix44f("projection") = proj;
}

//------------------------------------------------------------------------------
//! \brief Create a skybox.
//------------------------------------------------------------------------------
bool SkyBoxShape::createSkyBox()
{
    vs1.read("01_Core/shaders/09_SkyBoxTextureCube.vs");
    fs1.read("01_Core/shaders/09_SkyBoxTextureCube.fs");

    if (!m_progSkyBox.compile(vs1, fs1))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_progSkyBox.strerror() << "'" << std::endl;
        return false;
    }

    if (!m_progSkyBox.bind(m_skybox))
    {
        std::cerr << "Failed binding. Reason was '" << m_progSkyBox.strerror()
                  << "'" << std::endl;
        return false;
    }

    // Now we have to fill VBOs with data: here vertices. Because in
    // vertex shader a_position is vect3 we have to cast to Vector3f.
    m_skybox.vector3f("position") = {
#include "geometry/cube_position.txt"
    };

    // Add 6 textures to the sky box
    if (!m_skybox.textureCube("skybox").load<SOIL>(
            GLTextureCube::Map::POSITIVE_X, "external/assets/right.jpg"))
        return false;
    if (!m_skybox.textureCube("skybox").load<SOIL>(
            GLTextureCube::Map::NEGATIVE_X, "external/assets/left.jpg"))
        return false;
    if (!m_skybox.textureCube("skybox").load<SOIL>(
            GLTextureCube::Map::POSITIVE_Y, "external/assets/top.jpg"))
        return false;
    if (!m_skybox.textureCube("skybox").load<SOIL>(
            GLTextureCube::Map::NEGATIVE_Y, "external/assets/bottom.jpg"))
        return false;
    if (!m_skybox.textureCube("skybox").load<SOIL>(
            GLTextureCube::Map::POSITIVE_Z, "external/assets/front.jpg"))
        return false;
    if (!m_skybox.textureCube("skybox").load<SOIL>(
            GLTextureCube::Map::NEGATIVE_Z, "external/assets/back.jpg"))
        return false;

    return true;
}

//------------------------------------------------------------------------------
//! \brief Create a 3D shape (Cone, Pyramid, Cylinder, Tube). See
//! these class like factories for instanciating VAO.
//------------------------------------------------------------------------------
bool SkyBoxShape::createShape()
{
    vs2.read("01_Core/shaders/07_MultipleObjects.vs");
    fs2.read("01_Core/shaders/07_MultipleObjects.fs");

    // Compile shader as OpenGL program. This one will instanciate all OpenGL
    // objects for you.
    if (!m_progShape.compile(vs2, fs2))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_progShape.strerror() << "'" << std::endl;
        return false;
    }

    m_progShape.scalarf("scale") = 1.0f;
    m_progShape.vector4f("color") = Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
    if (!m_progShape.bind(m_shape))
    {
        std::cerr << "Failed binding. Reason was '" << m_progShape.strerror()
                  << "'" << std::endl;
        return false;
    }

    m_shape.vector3f("position") = {
#include "geometry/cube_position.txt"
    };
    m_shape.vector3f("position") *= 0.5f;

    m_shape.vector2f("UV") = {
#include "geometry/cube_texture.txt"
    };

    GLTexture2D& texture = m_shape.texture2D("texID");
    texture.interpolation(GLTexture::Minification::LINEAR,
                          GLTexture::Magnification::LINEAR);
    texture.wrap(GLTexture::Wrap::MIRRORED_REPEAT);
    return texture.load<SOIL>("external/assets/wooden-crate.jpg");
}

//------------------------------------------------------------------------------
//! \brief Init your scene.
//------------------------------------------------------------------------------
bool SkyBoxShape::onSetup()
{
    // Enable some OpenGL states
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDepthFunc(GL_LESS));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    return createShape() && createSkyBox();
}

static Vector3f lookat = Vector3f(2, 2, 2);
static Matrix44f view =
    matrix::lookAt(Vector3f(5, 5, 5), lookat, Vector3f(0, 1, 0));

// --------------------------------------------------------------
//! \brief Draw the shape.
// --------------------------------------------------------------
bool SkyBoxShape::drawShape()
{
    static float time = 0.0f;
    time += dt();

    // Apply a rotation to the box around the Y-axis
    Transformable<float, 3U> transform;
    transform.reset(); // restore to identity matrix
    transform.yaw(units::angle::radian_t(4.0f * time));
    transform.translate(Vector3f(2.0f, 2.0f, 2.0f));
    m_progShape.scalarf("scale") = cosf(time) + 0.5f;
    m_progShape.matrix44f("model") = transform.matrix();
    m_progShape.matrix44f("view") = view;

    // Set depth function back to default
    glCheck(glDepthFunc(GL_LESS));
    if (!m_shape.draw(Mode::TRIANGLES, 0u, 36u))
    {
        std::cerr << "Shape not renderered" << std::endl;
        return false;
    }

    return true;
}

// --------------------------------------------------------------
//! \brief Draw skybox. Should be draw as last.
// --------------------------------------------------------------
bool SkyBoxShape::drawSkyBox()
{
    // Remove translation from the view matrix
    m_progSkyBox.matrix44f("view") = Matrix44f(Matrix33f(view));

    // Change depth function so depth test passes when values are equal
    // to depth buffer's content
    glCheck(glDepthFunc(GL_LEQUAL));
    if (!m_skybox.draw(Mode::TRIANGLES, 0u, 36u))
    {
        std::cerr << "SkyBox not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
//! \brief Paint our scene.
//------------------------------------------------------------------------------
bool SkyBoxShape::onPaint()
{
    // Clear OpenGL color and depth buffers.
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Draw scene as normal.
    bool res1 = drawShape();
    // Draw skybox as last. Set depth function back to default
    bool res2 = drawSkyBox();

    return res1 && res2;
}

//------------------------------------------------------------------------------
void SkyBoxShape::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the onSetup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void SkyBoxShape::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
