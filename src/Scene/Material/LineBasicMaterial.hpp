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

#ifndef LINE_BASIC_MATERIAL_HPP
#define LINE_BASIC_MATERIAL_HPP

#include "OpenGL/Textures/Textures.hpp"
#include "Scene/Material/Color.hpp"
#include "Scene/Material/Material.hpp"

class LineBasicMaterial: public Material
{
public:

    struct Config
    {
        Config() {}

        bool useWidth = true;
        bool useColor = true;
    };

public:

    LineBasicMaterial(
        GLVAO& vao,
        LineBasicMaterial::Config const& conf = LineBasicMaterial::Config())
        : Material("LineBasicMaterial", vao), config(conf)
    {
    }

    /*inline float& width()
    {
        return program.scalarf("width");
    }
*/

    inline GLVertexBuffer<Vector4f>& colors()
    {
        return m_vao.vector4f("colors");
    }

private:

    virtual void generate(GLVertexShader& vertexShader,
                          GLFragmentShader& fragmentShader) override;

    virtual void init() override;

public:

    Config config;
};

#endif
