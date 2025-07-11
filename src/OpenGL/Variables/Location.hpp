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

#ifndef OPENGLCPPWRAPPER_GLLOCATION_HPP
#define OPENGLCPPWRAPPER_GLLOCATION_HPP

#include "OpenGL/GLObject.hpp"

// *****************************************************************************
//! \brief Base class representing either an attribute variable or an uniform
//! variable used in a GLSL shader program and refered by one of the following
//! keywords: \c in or \c out or \c uniform. For example:
//! \code
//!   in vec3 position;
//!   uniform sampler2D texID;
//! \endcode
//!

//! A GLLocation serve as a link between a shader variable and your C++ variable
//! and therefore it is an entry point in the shader to allow uploading CPU
//! data to the GPU. There are mainly three types: uniforms, attributes and
//! samplers and it exist derived classes from GLLocation to wrap them.

//! This base class only stores informations about the attribute variable
//! (dimension, type) but does not hold any value. These informations are
//! managed privately by GLProgam and therefore shall not be used directly by
//! the user. Indeed

//! Locations, depending on their type and used for creating the associated VBO
//! for attributes, textures for samplers when a VAO is bound to a GLProgram or
//! modifiying shader states for uniforms when a VAO is bound to a GLProgram.

// this class is only managed by \c GLProgram and they are
//! created automatically when shader code is parsed.

// *****************************************************************************
class GLLocation: public GLObject<GLint>
{
    //! \brief GLProgram directly modifies GLLocation states.
    friend class GLProgram;

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor. This constructor makes no other actions.
    //!
    //! \param[in] name Give a name to the instance. GLProgram uses these names
    //! in their internal hash table and shall reference to a real variable name
    //! insde the GLSL shader code.
    //! \param[in] dim set the dimension of variable (1 for scalar else the
    //! dimension of vector: 2, 3, 4 or dimension of matrices ...)
    //! \param[in] type set the OpenGL type of data (GL_FLOAT, GL_INT ...)
    //! \param[in] program set the GLProgram identifier (which is the owner of
    //! this instance).
    //--------------------------------------------------------------------------
    GLLocation(const char* name,
               const GLint size,
               const GLenum type,
               const GLuint program)
        : GLObject(name, type), m_size(size), m_program(program)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements in CPU and GPU memories.
    //--------------------------------------------------------------------------
    virtual ~GLLocation() override
    {
        release();
    }

    //--------------------------------------------------------------------------
    //! \brief Alias for GLObject::handle() but in a more explicit way.
    //--------------------------------------------------------------------------
    inline GLint locate() const
    {
        return handle();
    }

    //--------------------------------------------------------------------------
    //! \brief Return the dimension of the shader variable.
    //--------------------------------------------------------------------------
    inline GLint size() const
    {
        return m_size;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the program bound to it.
    //--------------------------------------------------------------------------
    inline GLuint progam() const
    {
        return m_program;
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Reset internal states. No memory release is made here.
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {
        m_size = 0;
        m_program = 0u;
    }

protected:

    //! \brief Dimension of the attribute variable (scalar, vector, matrix).
    GLint m_size;
    //! \brief The handle of the GLProgram owning this instance.
    GLuint m_program;
};

#endif // OPENGLCPPWRAPPER_GLLOCATION_HPP
