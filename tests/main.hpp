//==============================================================================
// SimForth: A Forth for SimTaDyn.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of SimForth.
//
// SimForth is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with SimForth.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#ifndef MAIN_HPP
#define MAIN_HPP

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "OpenGL/Context/OpenGL.hpp"
#include <GL/glew.h>
#include <GLFW/glfw3.h>

using namespace ::testing;

class OpenGLContext
{
    typedef std::function<void()> Callback;

public:

    OpenGLContext(Callback const& cb)
    {
        if (!glfwInit())
            throw GL::Exception("Failed to initialize GLFW");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Mac OS X
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
        m_context = glfwCreateWindow(1, 1, "", nullptr, nullptr);
        glfwMakeContextCurrent(m_context);
        glfwSwapInterval(1); // Enable vsync
        GL::Context::setCreated();
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
            throw GL::Exception("Failed to initialize GLFW");
        if (!GLEW_VERSION_3_3)
            throw GL::Exception("OpenGL 3.3 API is not available!");
        cb();
    }

    ~OpenGLContext()
    {
        glfwDestroyWindow(m_context);
        glfwTerminate();
        GL::Context::setCreated(false);
    }

private:

    GLFWwindow* m_context = nullptr;
};

#endif // MAIN_HPP
