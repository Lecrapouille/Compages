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
//
// This file is a derived work of https://github.com/glumpy/glumpy
//
// Copyright (c) 2009-2016 Nicolas P. Rougier. All rights reserved.
// Distributed under the (new) BSD License.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_GLSAMPLERS_HPP
#define OPENGLCPPWRAPPER_GLSAMPLERS_HPP

// *****************************************************************************
//! \file Sampler.hpp file implements:
//!   - GLSampler:
//!   - GLSampler1D:
//!   - GLSampler2D:
//!   - GLSampler3D:
//!   - GLSamplerCube:
// *****************************************************************************

#include "OpenGL/Variables/Sampler.hpp"

// *****************************************************************************
//! \brief Sampler for 1D texture.
// *****************************************************************************
class GLSampler1D: public GLSampler
{
public:

    //----------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //----------------------------------------------------------------------------
    GLSampler1D(const char* name, const GLenum texture_id, const GLuint prog)
        : GLSampler(name, GL_SAMPLER_1D, texture_id, prog)
    {
    }
};

// *****************************************************************************
//! \brief Sampler for 2D texture.
// *****************************************************************************
class GLSampler2D: public GLSampler
{
public:

    //----------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //----------------------------------------------------------------------------
    GLSampler2D(const char* name, const GLenum texture_id, const GLuint prog)
        : GLSampler(name, GL_SAMPLER_2D, texture_id, prog)
    {
    }
};

// *****************************************************************************
//! \brief Sampler for 3D texture.
// *****************************************************************************
class GLSampler3D: public GLSampler
{
public:

    //----------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //----------------------------------------------------------------------------
    GLSampler3D(const char* name, const GLenum texture_id, const GLuint prog)
        : GLSampler(name, GL_SAMPLER_3D, texture_id, prog)
    {
    }
};

// *****************************************************************************
//! \brief Sampler for Cubic texture.
// *****************************************************************************
class GLSamplerCube: public GLSampler
{
public:

    //----------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //----------------------------------------------------------------------------
    GLSamplerCube(const char* name, const GLenum texture_id, const GLuint prog)
        : GLSampler(name, GL_SAMPLER_CUBE, texture_id, prog)
    {
    }
};

#endif // OPENGLCPPWRAPPER_GLSAMPLERS_HPP
