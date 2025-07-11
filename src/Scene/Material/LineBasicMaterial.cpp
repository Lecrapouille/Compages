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

#include "Scene/Material/LineBasicMaterial.hpp"
#include "Scene/Material/ShaderLib.hpp"

//-----------------------------------------------------------------------------
void LineBasicMaterial::generate(GLVertexShader& vertexShader,
                                 GLFragmentShader& fragmentShader)
{
    // if (program.hasUniform<Vector3f>("color"))
    //     config.useColor = true;

    // if (program.hasUniform<float>("width"))
    //     config.useWidth = true;

    shaders::materials::basic::line::code(vertexShader);   //, config);
    shaders::materials::basic::line::code(fragmentShader); //, config);
}

//-----------------------------------------------------------------------------
void LineBasicMaterial::init()
{
    // if (!program.hasUniform<Vector3f>("color"))
    //     color() = Vector3f(1.0f, 1.0f, 1.0f);

    // if (!program.hasUniform<Vector3f>("width"))
    //     width() = 1.0f;
}
