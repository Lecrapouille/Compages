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

#include "00_ColorfulTriangle.hpp"
#include <iostream>

//------------------------------------------------------------------------------
//! \brief m_prog, m_triangle: GLObject instances have a name. These names are
//! used for two things:
//!  - for log and debug purpose (GLShader, GLVAO, GLProgram ...) in this case
//!    names are public.
//!  - for key in hash maps (GLVBO, GLAttributes, GLSamplers) in this case names
//!    are private and are internally managed automatically .
//------------------------------------------------------------------------------
ColorfulTriangle::ColorfulTriangle(uint32_t const width,
                                   uint32_t const height,
                                   const char* title)
    : GLWindow(width, height, title), m_prog("Prog"), m_triangle("triangle")
// Possible alternative for VAO:
//   m_triangle(<name>, BufferUsage::<usage>, <reserve size for VBO>)
// Where:
//   - <usage> is STREAM_DRAW or STATIC_DRAW or DYNAMIC_DRAW (default). See
//     documentation of glBufferData() for more details.
//   - <reserve size for VBO> number of elements to allocate when creating VBO.
{
    std::cout << "Hello ColorfulTriangle: " << info() << std::endl;
}

//------------------------------------------------------------------------------
ColorfulTriangle::~ColorfulTriangle()
{
    std::cout << "Bye ColorfulTriangle" << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Callback called when the window has been resized. To get windows size
//! you can call width<T>() and height<T>() where T is the desired type int,
//! float ...
//------------------------------------------------------------------------------
void ColorfulTriangle::onWindowResized()
{
    // Note: you can wrap glCheck() around OpenGL functions to check potential
    // errors.
    glCheck(glViewport(0, 0, width<int>(), height<int>()));
}

//------------------------------------------------------------------------------
//! \brief Callback called after OpenGL context has been created. Add in this
//! method all your application setup.
//!
//! In this example, we load a vertex shader and a fragment shader from strings
//! (GLVertexShader and GLFragmentShader). Compile them inside a GLProgram
//! instance and bind this GLProgram instance to a GLVAO instance.
//!
//! Shaders allow projecting vertices on the screen and applying colors.
//! Program allows to extract and memorize shaders variables (uniforms,
//! attributes ...). GLVAO is your 3D shape and allows to store textures and
//! GLVBOs and is the instance to draw. GLVBO holds information of your model
//! (vertex positions, normals ...). They are created when binding a VAO to a
//! Program.
//!
//! As developper, you just have to give shaders GLSL code and fill these VBOs
//! with data (vertex positions and vertex colors) and texture to the path to
//! the desired jpeg file.
//------------------------------------------------------------------------------
bool ColorfulTriangle::onSetup()
{
    // In next examples we will see how to load shaders GLSL code from files but
    // for now we will see how to load vertex and fragment shaders from
    // strings. There are two type of strings: -- classic string but you need to
    // add explicitly chars such as '"' '\n' -- C++11 raw string litterals avoid
    // to add '\n' chars. In this function, we will show the two ways.

    // Shaders have operators such as << and >> to insert text.
    std::string v2f("struct v2f_s {                       \n"
                    "  vec3      color;                   \n"
                    "} v2f;                               \n");

    // Load vertex shader from a classic string.
    m_vertex_shader << "in vec2      position;                   \n"
                       "in vec3      color;                      \n"
                       "out "
                    << v2f
                    << "void main() {                            \n"
                       "  v2f.color = color;                     \n"
                       "  gl_Position = vec4(position, 0.0, 1.0);\n"
                       "}";

    // Example of how to insert string from the begining
    "#version 330 core\n" >> m_vertex_shader;

    // Load fragment shader from C++11 raw string.
    m_fragment_shader <<
        R"GLSL(#version 330 core
                   in struct v2f_s {
                     vec3 color;
                   } v2f;
                   out vec4 fragColor;
                   void main() {
                       fragColor = vec4(v2f.color, 1);
                   })GLSL";

    // Optional. For debug, shaders can be displayed with:
    std::cout << "Shader program: " << m_vertex_shader.name() << ":"
              << std::endl;
    std::cout << m_vertex_shader << std::endl << std::endl;
    std::cout << "Fragment program: " << m_fragment_shader.name() << ":"
              << std::endl;
    std::cout << m_fragment_shader << std::endl << std::endl;

    // You have to compile shaders into the class GLProgram. If shaders
    // contained syntax errors the compile() will return false and errors can be
    // shown with strerror(). If your OpenGL has been compiled with success, the
    // GLProgram will create its own list of shader uniforms, attributes, and
    // samplers from shaders. In this example, shader only use the attribute
    // position and color.
    if (!m_prog.compile(m_vertex_shader, m_fragment_shader))
    {
        std::cerr << "Failed compiling OpenGL program. Reason was '"
                  << m_prog.strerror() << "'" << std::endl;
        return false;
    }

    // Optional. Helper for debugging states of your GLProgram. See
    // examples/debug.cpp
    debug(m_prog);

    // To initialize your 3D model, you have to use a VAO and bind it to the
    // desired GLProgram. This last, helped with its internal list of shader
    // attributes and samplers, will populate VBOs and textures inside the VAO
    // instance. Shader attributes and samplers give their name to VBOs and
    // textures. In our example, the VAO will have a single VBO named position.
    if (!m_prog.bind(m_triangle))
    {
        std::cerr << "Failed binding. Reason was '" << m_prog.strerror() << "'"
                  << std::endl;
        return false;
    }

    // Fill VBOs of the VAO: init triangle vertex positions. Note "position"
    // shall refer to the variable position inside the GLSL code. If you rename
    // it in the shader you will have to rename here too. Beware name are case
    // sensitive.
    m_triangle.vector2f("position") = { Vector2f(-1.0f, -1.0f), // X, Y
                                        Vector2f(1.0f, -1.0f),
                                        Vector2f(0.0f, 1.0f) };

    // Fill VBOs of the VAO: init triangle vertex colors. Note "color" shall
    // refer to the variable color inside the GLSL code. If you rename it in the
    // shader you will have to rename here too. Beware name are case sensitive.
    m_triangle.vector3f(
        "color") = { Vector3f(1.0f, 0.0f, 0.0f), // Red, Green, Blue
                     Vector3f(0.0f, 1.0f, 0.0f),
                     Vector3f(0.0f, 0.0f, 1.0f) };

    // Optional. Helper for debugging states of your GLVAO. See
    // examples/debug.cpp
    debug(m_triangle);

    return true;
}

//------------------------------------------------------------------------------
//! \brief Callback called each time for rendering the scene.
//------------------------------------------------------------------------------
bool ColorfulTriangle::onPaint()
{
    //
    glCheck(glClearColor(0.0f, 0.0f, 0.4f, 0.0f));
    glCheck(glClear(GL_COLOR_BUFFER_BIT));

    // Render 3 vertices (starting from the first indice 0) as a triangle.
    // You can simply call m_triangle.draw(Mode::TRIANGLES) or call
    // m_triangle.draw().
    if (!m_triangle.draw(Mode::TRIANGLES, 0u, 3u))
    {
        std::cerr << "Triangle not renderered" << std::endl;
        return false;
    }

    // Note: the double buffer is automatically managed
    return true;
}

//------------------------------------------------------------------------------
//! \brief Callback called when ColorfulTriangle::onSetup() failed.
//------------------------------------------------------------------------------
void ColorfulTriangle::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
//! \brief Callback called when ColorfulTriangle::onPaint() failed.
//------------------------------------------------------------------------------
void ColorfulTriangle::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
