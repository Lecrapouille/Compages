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

#include "13_PostProdFrameBuffer.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

//------------------------------------------------------------------------------
PostProdFrameBuffer::PostProdFrameBuffer(uint32_t const width,
                                         uint32_t const height,
                                         const char* title)
    : GLWindow(width, height, title),
      m_fbo("FBO"),
      m_prog_scene("prog_cube"),
      m_cube("VAO_cube"),
      m_floor("VAO_floor"),
      m_prog_screen("prog_screen"),
      m_screen("VAO_screen")
{
    std::cout << "Hello PostProdFrameBuffer " << info() << std::endl;
}

//------------------------------------------------------------------------------
PostProdFrameBuffer::~PostProdFrameBuffer()
{
    std::cout << "Bye PostProdFrameBuffer" << std::endl;
}

//------------------------------------------------------------------------------
void PostProdFrameBuffer::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));

    m_prog_scene.matrix44f("projection") = matrix::perspective(
        50.0_deg, width<float>() / height<float>(), 0.1f, 10.0f);
    m_prog_screen.scalarf("screen_width") = width<float>();
    m_prog_screen.scalarf("screen_height") = height<float>();
}

//------------------------------------------------------------------------------
//! \brief Shader for rendering the scene
//------------------------------------------------------------------------------
bool PostProdFrameBuffer::firstProgram()
{
    // Compile the shader for the scene
    m_vs_scene.read("01_Core/shaders/13_PostProdFrameBuffer_cube.vs");
    m_fs_scene.read("01_Core/shaders/13_PostProdFrameBuffer_cube.fs");
    if (!m_prog_scene.compile(m_vs_scene, m_fs_scene))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog_scene.strerror() << "'" << std::endl;
        return false;
    }

    // Create the cube
    if (!m_prog_scene.bind(m_cube))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog_scene.strerror()
                  << "'" << std::endl;
        return false;
    }
    m_cube.vector3f("position") = {
#include "geometry/cube_position.txt"
    };
    m_cube.vector2f("UV") = {
#include "geometry/cube_texture.txt"
    };

    // Create the floor
    if (!m_prog_scene.bind(m_floor))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog_scene.strerror()
                  << "'" << std::endl;
        return false;
    }
    m_floor.vector3f("position") = {
#include "geometry/floor_position.txt"
    };
    m_floor.vector2f("UV") = {
#include "geometry/floor_texture.txt"
    };

    // Apply textures
    if (!m_cube.texture2D("texID").load<SOIL>(
            "external/assets/wooden-crate.jpg"))
        return false;
    if (!m_floor.texture2D("texID").load<SOIL>("external/assets/path.png"))
        return false;

    // Init Model-View-Project matrices (shader uniforms)
    float ratio = width<float>() / height<float>();
    m_prog_scene.matrix44f("projection") =
        matrix::perspective(50.0_deg, ratio, 0.1f, 10.0f);
    m_prog_scene.matrix44f("model") = Matrix44f(matrix::Identity);
    m_prog_scene.matrix44f("view") =
        matrix::lookAt(Vector3f(3, 3, 3), Vector3f(0, 0, 0), Vector3f(0, 1, 0));

    return true;
}

//------------------------------------------------------------------------------
//! \brief Shader for post producing the screen
//------------------------------------------------------------------------------
bool PostProdFrameBuffer::secondProgram()
{
    // Compile the shader for the screen
    m_vs_screen.read("01_Core/shaders/13_PostProdFrameBuffer_screen.vs");
    m_fs_screen.read("01_Core/shaders/13_PostProdFrameBuffer_screen.fs");
    if (!m_prog_screen.compile(m_vs_screen, m_fs_screen))
    {
        std::cerr << "failed compiling OpenGL program. Reason was '"
                  << m_prog_screen.strerror() << "'" << std::endl;
        return false;
    }

    // Create a quad
    if (!m_prog_screen.bind(m_screen))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog_screen.strerror()
                  << "'" << std::endl;
        return false;
    }
    m_screen.vector2f("position") = {
        Vector2f(-1.0f, 1.0f), Vector2f(-1.0f, -1.0f), Vector2f(1.0f, -1.0f),
        Vector2f(-1.0f, 1.0f), Vector2f(1.0f, -1.0f),  Vector2f(1.0f, 1.0f)
    };
    m_screen.vector2f("UV") = { Vector2f(0.0f, 1.0f), Vector2f(0.0f, 0.0f),
                                Vector2f(1.0f, 0.0f), Vector2f(0.0f, 1.0f),
                                Vector2f(1.0f, 0.0f), Vector2f(1.0f, 1.0f) };

    m_prog_screen.scalarf("screen_width") = width<float>();
    m_prog_screen.scalarf("screen_height") = height<float>();

    return true;
}

//------------------------------------------------------------------------------
bool PostProdFrameBuffer::onSetup()
{
    glCheck(glEnable(GL_DEPTH_TEST));
    glCheck(glDisable(GL_BLEND));
    glCheck(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

    if (!firstProgram())
        return false;
    if (!secondProgram())
        return false;

    // Framebuffer
    m_fbo.resize(width<uint32_t>(), height<uint32_t>());
    m_fbo.createColorTexture(m_screen.texture2D("texID"));
    m_fbo.createDepthBuffer();

    return true;
}

//------------------------------------------------------------------------------
bool PostProdFrameBuffer::onPaint()
{
    static float time = 0.0f;
    time += dt();

    // First pass: draw to the framebuffer texture
    m_fbo.render(0u,
                 0u,
                 width<uint32_t>(),
                 height<uint32_t>(),
                 [this]()
                 {
                     glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
                     glCheck(
                         glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
                     glEnable(GL_DEPTH_TEST);
                     if (!m_floor.draw(Mode::TRIANGLES, 0u, 6u))
                     {
                         std::cerr << "Floor not renderered" << std::endl;
                         return;
                     }
                     if (!m_cube.draw(Mode::TRIANGLES, 0u, 36u))
                     {
                         std::cerr << "Cube not renderered" << std::endl;
                         return;
                     }
                 });

    // Second pass: draw to the screen
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    m_prog_screen.scalarf("time") = time;
    if (!m_screen.draw(Mode::TRIANGLES, 0u, 6u))
    {
        std::cerr << "Screen not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void PostProdFrameBuffer::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the onSetup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void PostProdFrameBuffer::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
