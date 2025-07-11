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

#include "debug.hpp"
#include <iostream>

//------------------------------------------------------------------------------
void showUnloadedTextures(GLVAO const& vao)
{
    std::vector<std::string> list;
    size_t count = vao.getUnloadedTextures(list);
    std::cerr << "Failed loading " << count << " textures:" << std::endl;
    for (auto& it : list)
    {
        std::cerr << " " << it;
    }
    std::cerr << std::endl;
}

//------------------------------------------------------------------------------
void debug(GLVertexShader const& vertex, GLFragmentShader const& fragment)
{
    std::cout << "Shader program " << vertex.name() << ":" << std::endl;
    std::cout << vertex << std::endl << std::endl;
    std::cout << "Fragment program " << fragment.name() << ":" << std::endl;
    std::cout << fragment << std::endl << std::endl;
}

//------------------------------------------------------------------------------
void debug(GLProgram const& prog)
{
    std::cout << "Debug GLProgram " << prog.name() << ":" << std::endl;

    // Display the list of failed shaders
    {
        std::vector<std::string> names;
        size_t count = prog.getFailedShaders(names);
        std::cout << "  Has " << count << " failed shaders: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }

    // Display the list of Uniforms hold by the program
    {
        std::vector<std::string> names;
        size_t count = prog.getUniformNames(names);
        std::cout << "  Has " << count << " uniforms: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }

    // Display the list of Attributes hold by the program
    {
        std::vector<std::string> names;
        size_t count = prog.getAttributeNames(names);
        std::cout << "  Has " << count << " attributes: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }

    // Display the list of Samplers hold by the program
    {
        std::vector<std::string> names;
        size_t count = prog.getSamplerNames(names);
        std::cout << "  Has " << count << " samplers: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }
}

//------------------------------------------------------------------------------
void debug(GLVAO const& vao)
{
    std::cout << "Debug GLVAO " << vao.name() << ":" << std::endl;

    // Display the list of VBOs hold by the VAO
    {
        std::vector<std::string> names;
        size_t count = vao.getVBONames(names);
        std::cout << "  Has " << count << " VBO: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }

    // Display the list of textures hold by the VAO
    {
        std::vector<std::string> names;
        size_t count = vao.getTexturesNames(names);
        std::cout << "  Has " << count << " textures: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }

    // Display the list of textures hold by the VAO
    {
        std::vector<std::string> names;
        size_t count = vao.getUnloadedTextures(names);
        std::cout << "  Has " << count << " unloaded textures: " << std::endl;
        for (auto& it : names)
        {
            std::cout << "    '" << it << "'" << std::endl;
        }
    }
}
