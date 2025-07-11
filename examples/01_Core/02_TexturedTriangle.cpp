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

#include "02_TexturedTriangle.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

//------------------------------------------------------------------------------
TexturedTriangle::TexturedTriangle(uint32_t const width,
                                   uint32_t const height,
                                   const char* title)
    : GLWindow(width, height, title), m_prog("Prog"), m_triangle("triangle")
{
    std::cout << "Hello TexturedTriangle: " << info() << std::endl;
}

//------------------------------------------------------------------------------
TexturedTriangle::~TexturedTriangle()
{
    std::cout << "Bye TexturedTriangle" << std::endl;
}

//------------------------------------------------------------------------------
void TexturedTriangle::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
}

//------------------------------------------------------------------------------
//! \brief Load a texture
//------------------------------------------------------------------------------
bool TexturedTriangle::loadTextures()
{
    // Get the texture from its sampler name.
    GLTexture2D& texture = m_triangle.texture2D("texID");

    // Configurate the texture
    texture.interpolation(GLTexture::Minification::LINEAR,
                          GLTexture::Magnification::LINEAR);
    texture.wrap(GLTexture::Wrap::MIRRORED_REPEAT);

    // Read the png file and load data on the texture (CPU side). Dirty data
    // will be automaticaly transfered to the GPU. Note that the loader is by
    // default SOIL but you can pass it your own load as 2nd parameter.
    return texture.load<SOIL>("external/assets/hazard.png");
}

//------------------------------------------------------------------------------
bool TexturedTriangle::onSetup()
{
    // Load vertex and fragment shaders with GLSL code.
    m_vertex_shader.read("01_Core/shaders/02_TexturedTriangle.vs");
    m_fragment_shader.read("01_Core/shaders/02_TexturedTriangle.fs");

    // Compile the shader program
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    // Create VBOs of the VAO.
    if (!m_prog.bind(m_triangle))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    // Fill VBOs of the VAO: init triangle vertex positions.
    m_triangle.vector3f("position") = { Vector3f(-1.0f, -1.0f, 0.0f),
                                        Vector3f(1.0f, -1.0f, 0.0f),
                                        Vector3f(0.0f, 1.0f, 0.0f) };

    // Fill VBOs of the VAO: init triangle texture positions.
    m_triangle.vector2f("UV") = { Vector2f(0.0f, 0.0f),
                                  Vector2f(1.0f, 0.0f),
                                  Vector2f(0.5f, 1.0f) };

    // Repeat the texture motif (if wanted)
    m_triangle.vector2f("UV") *= 4.0f;

    // Load textures
    if (!loadTextures())
    {
        showUnloadedTextures(m_triangle);
        return false;
    }

    // Helper for debugging states of your program
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_triangle);

    return true;
}

//------------------------------------------------------------------------------
bool TexturedTriangle::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    // implicit arguments: Mode::TRIANGLES, 0u, 3u
    if (!m_triangle.draw())
    {
        std::cerr << "Triangle not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void TexturedTriangle::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void TexturedTriangle::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
