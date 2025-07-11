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

#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include "OpenGL/Buffers/VAO.hpp"

// *****************************************************************************
//! \brief Interface class for defining the reaction of an object to the light.
// *****************************************************************************
class Material
{
public:

    //--------------------------------------------------------------------------
    //! \brief Give a name to the material. It will be passed to program and
    //! shaders.
    //--------------------------------------------------------------------------
    Material(std::string const& name, GLVAO& vao)
        : program(name),
          m_vert_shader("VS_" + name),
          m_frag_shader("FS_" + name),
          m_vao(vao)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Needed because of pure virtual method.
    //--------------------------------------------------------------------------
    virtual ~Material() = default;

    //--------------------------------------------------------------------------
    //! \brief Return the material name.
    //--------------------------------------------------------------------------
    inline std::string const& name() const
    {
        return program.name();
    }

    //--------------------------------------------------------------------------
    //! \brief Generate shaders, compile shaders and init their variables.
    //! \return true if shader have been compiled, else return false.
    //--------------------------------------------------------------------------
    bool compile();

private:

    //--------------------------------------------------------------------------
    //! \brief Generate the code for vertex and fragment shaders.
    //--------------------------------------------------------------------------
    virtual void generate(GLVertexShader& /*vert*/, GLFragmentShader& /*frag*/)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Initialize uniform glsl variables with default values.
    //--------------------------------------------------------------------------
    virtual void init() {}

public:

    GLProgram program;

private:

    GLVertexShader m_vert_shader;
    GLFragmentShader m_frag_shader;

protected:

    GLVAO& m_vao;
};

#endif
