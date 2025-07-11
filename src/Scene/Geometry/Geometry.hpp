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

#ifndef GEOMETRY_HPP
#define GEOMETRY_HPP

#include "OpenGL/Buffers/iVAO.hpp"
#include "Scene/Material/ShaderLib.hpp"

// *****************************************************************************
//! \brief Base class for generating vertex positions, normals, texture
//! coordinates and index of predefined shapes (cube, sphere, tube ...)
// *****************************************************************************
class Geometry
{
public:

    //--------------------------------------------------------------------------
    //! \brief Virtual destructor because of pure virtual method.
    //--------------------------------------------------------------------------
    virtual ~Geometry() = default;

    //--------------------------------------------------------------------------
    //! \brief Do the generation of the geometry (vertices, normals, texture
    //! coordinates). Data will be stored inside the VBOs of the given VAO.
    //!
    //! \param[in,out] vao: Indexed VAO that will hold vertices, normals,
    //!   texture coordinates inside VBOs. The VAO should be bound to GLProgram
    //!   before being passed as param to this method to know if normals and
    //!   texture coordinates shall be populated or not; else only vertices will
    //!   be populated.
    //! \param[in] clear: if true clear VBOs before generating data.
    //! \return true in case of success.
    //--------------------------------------------------------------------------
    virtual bool generate(GLVAO32& vao, const bool clear = true) = 0;
};

#endif
