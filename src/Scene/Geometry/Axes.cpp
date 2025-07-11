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

#include "Scene/Geometry/Axes.hpp"

//------------------------------------------------------------------------------
bool Axes::generate(GLVAO32& vao, const bool clear)
{
    if (!vao.hasVBO<Vector3f>(shaders::name::position))
    {
        std::cerr << "VBO for vertices is needed" << std::endl;
        return false;
    }

    auto& position = vao.vector3f(shaders::name::position);

    if (clear)
    {
        position.clear();
    }
    position = {
        Vector3f(0.0f, 0.0f, 0.0f), Vector3f(config.size, 0.0f, 0.0f),
        Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, config.size, 0.0f),
        Vector3f(0.0f, 0.0f, 0.0f), Vector3f(0.0f, 0.0f, config.size)
    };

    if (vao.hasVBO<Vector4f>("colors"))
    {
        auto& color = vao.vector4f("colors");

        if (clear)
        {
            color.clear();
        }
        color = {
            Vector4f(1.0f, 0.0f, 0.0f, 1.0f), Vector4f(1.0f, 0.0f, 0.0f, 1.0f),
            Vector4f(0.0f, 1.0f, 0.0f, 1.0f), Vector4f(0.0f, 1.0f, 0.0f, 1.0f),
            Vector4f(0.0f, 0.0f, 1.0f, 1.0f), Vector4f(0.0f, 0.0f, 1.0f, 1.0f)
        };
    }

    vao.index() = { 0u, 1u, 2u, 3u, 4u, 5u };

    return true;
}
