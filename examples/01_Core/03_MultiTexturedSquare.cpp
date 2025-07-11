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

#include "03_MultiTexturedSquare.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

//------------------------------------------------------------------------------
MultiTexturedSquare::MultiTexturedSquare(uint32_t const width,
                                         uint32_t const height,
                                         const char* title)
    : GLWindow(width, height, title), m_prog("Prog"), m_square("square")
{
    std::cout << "Hello MultiTexturedSquare: " << info() << std::endl;
}

//------------------------------------------------------------------------------
MultiTexturedSquare::~MultiTexturedSquare()
{
    std::cout << "Bye MultiTexturedSquare" << std::endl;
}

//------------------------------------------------------------------------------
void MultiTexturedSquare::onWindowResized()
{
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
}

//------------------------------------------------------------------------------
bool MultiTexturedSquare::loadTextures()
{
    if (!m_square.texture2D("blendMap")
             .load<SOIL>("external/assets/blendMap.png"))
        return false;
    if (!m_square.texture2D("backgroundTexture")
             .load<SOIL>("external/assets/grassy2.png"))
        return false;
    if (!m_square.texture2D("rTexture").load<SOIL>("external/assets/mud.png"))
        return false;
    if (!m_square.texture2D("gTexture")
             .load<SOIL>("external/assets/grassFlowers.png"))
        return false;
    if (!m_square.texture2D("bTexture").load<SOIL>("external/assets/path.png"))
        return false;

    return true;
}

//------------------------------------------------------------------------------
bool MultiTexturedSquare::onSetup()
{
    // Load vertex and fragment shaders with GLSL code.
    m_vertex_shader.read("01_Core/shaders/03_MultiTexturedTriangle.vs");
    m_fragment_shader.read("01_Core/shaders/03_MultiTexturedTriangle.fs");

    // Compile the shader program
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    // Create VBOs of the VAO.
    if (!m_prog.bind(m_square))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    // Fill VBOs of the VAO: init square vertex positions.
    m_square.vector3f("position") = {
        // Triangle 1
        Vector3f(1.0f, 1.0f, 0.0f),
        Vector3f(1.0f, -1.0f, 0.0f),
        Vector3f(-1.0f, 1.0f, 0.0f),

        // Triangle 2
        Vector3f(1.0f, -1.0f, 0.0f),
        Vector3f(-1.0f, -1.0f, 0.0f),
        Vector3f(-1.0f, 1.0f, 0.0f),
    };

    // Fill VBOs of the VAO: init square texture positions.
    m_square.vector2f("UV") = {
        // Triangle 1
        Vector2f(1.0f, 1.0f),
        Vector2f(1.0f, 0.0f),
        Vector2f(0.0f, 1.0f),

        // Triangle 2
        Vector2f(1.0f, 0.0f),
        Vector2f(0.0f, 0.0f),
        Vector2f(0.0f, 1.0f),
    };

    // Fill Load texture files
    if (!loadTextures())
    {
        showUnloadedTextures(m_square);
        return false;
    }

    // Helper for debugging states of your program
    debug(m_vertex_shader, m_fragment_shader);
    debug(m_prog);
    debug(m_square);

    return true;
}

//------------------------------------------------------------------------------
bool MultiTexturedSquare::onPaint()
{
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    // implicit arguments: Mode::TRIANGLES, 0u, 6u
    if (!m_square.draw())
    {
        std::cerr << "Square not renderered" << std::endl;
        return false;
    }

    return true;
}

//------------------------------------------------------------------------------
void MultiTexturedSquare::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void MultiTexturedSquare::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
