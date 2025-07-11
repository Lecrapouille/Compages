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

#ifndef DEPTH_MATERIAL_HPP
#define DEPTH_MATERIAL_HPP

#include "Scene/Material/Material.hpp"

class DepthMaterial: public Material
{
public:

    struct Config
    {
    };

    DepthMaterial(GLVAO& vao) : Material("DepthMaterial", vao) {}

    float& near()
    {
        return program.scalarf("near");
    }

    float& far()
    {
        return program.scalarf("far");
    }

    float& opacity()
    {
        return program.scalarf("opacity");
    }

private:

    virtual void generate(GLVertexShader& vertexShader,
                          GLFragmentShader& fragmentShader) override;

    virtual void init() override;

public:

    Config config;
};

#endif
