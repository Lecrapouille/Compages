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

#ifndef OPENGLCPPWRAPPER_GLATTRIBUTES_HPP
#define OPENGLCPPWRAPPER_GLATTRIBUTES_HPP

#include "OpenGL/Variables/Location.hpp"
#include <cassert>

// *****************************************************************************
//! \brief Represent an attribute variable used in a GLSL shader program
//! (refered by \c in and \c out keywords) and used for creating the associated
//! VBO when a VAO is bound to a GLProgram. Example:
//! \code
//!   in vec3 position;
//! \endcode
//!
//! This class only stores informations about the attribute variable (dimension,
//! type) but does not hold any value. These information are then used by
//! GLProgam when a VAO is bind to it for creating VBO inside the VAO.
//! GLAttribute should not be used directly by the user but internaly private
//! instances by GLProgram.
// *****************************************************************************
class GLAttribute: public GLLocation
{
public:

    //--------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //! \param[in] name Give a name to the instance. The name shall be in
    //! accordance to the attibute variable in the GLSL shader. The GLProgram
    //! uses these names as internal hash key.
    //! \param[in] size set the dimension of variable (1 for scalar, 2 .. 4
    //! depending on the dimension of the vector).
    //! \param[in] gltype set the OpenGL type of data (GL_FLOAT, GL_INT,
    //! GL_FLOAT_VEC4 ...)
    //! \param[in] prog the handle of the GLProgram (which is the owner of this
    //! instance).
    //--------------------------------------------------------------------------
    GLAttribute(const char* name,
                const GLint size,
                const GLint gltype,
                const GLuint prog)
        : GLLocation(name, size, static_cast<GLenum>(gltype), prog)
    {
        assert((size >= 1) && (size <= 4));
    }

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements in CPU and GPU memories.
    //--------------------------------------------------------------------------
    virtual ~GLAttribute() override
    {
        release();
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Create a new OpenGL attribute location.
    //! \return always false (success).
    //--------------------------------------------------------------------------
    virtual bool onCreate() override
    {
        m_handle = glCheck(glGetAttribLocation(m_program, cname()));
        m_index = static_cast<GLuint>(m_handle);
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Bind the OpenGL attribute location.
    //--------------------------------------------------------------------------
    virtual void onActivate() override
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        glCheck(
            glVertexAttribPointer(m_index,
                                  m_size,
                                  m_target,
                                  GL_FALSE,
                                  static_cast<GLsizei>(m_stride),
                                  (void*)m_offset)); // Do not place it onCreate
#pragma GCC diagnostic pop
        glCheck(glEnableVertexAttribArray(m_index));
    }

    //--------------------------------------------------------------------------
    //! \brief Setup the behavior of the instance. This is a dummy
    //! method. No action is made.
    //! \return always false (success).
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief This is a dummy method. No action is made.
    //! \return always false (success).
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Unbind the OpenGL attribute location.
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override
    {
        glCheck(glDisableVertexAttribArray(m_index));
    }

    //--------------------------------------------------------------------------
    //! \brief Reset internal states. No memory release is made here.
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {
        // TODO: UT Check GLLocation::onRelease(); is called
        m_index = 0;
        m_stride = 0;
        m_offset = 0;
    }

private:

    //! \brief Hack ! This is an alias for m_handle but with a different type.
    GLuint m_index = 0;
    //! \brief Specifies the byte offset between consecutive generic vertex
    //! attributes. See OpenGL API doc for glVertexAttribPointer().
    size_t m_stride = 0;
    //! \brief Specifies a offset of the first component of the first generic
    //! vertex attribute in the array in the data store. See OpenGL API doc for
    //! glVertexAttribPointer().
    size_t m_offset = 0;
};

#endif // OPENGLCPPWRAPPER_GLATTRIBUTES_HPP
