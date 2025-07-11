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

#ifndef OPENGLCPPWRAPPER_GLSAMPLER_HPP
#define OPENGLCPPWRAPPER_GLSAMPLER_HPP

// *****************************************************************************
//! \file Sampler.hpp file implements:
//!   - GLSampler:
//!   - GLSampler1D:
//!   - GLSampler2D:
//!   - GLSampler3D:
//!   - GLSamplerCube:
// *****************************************************************************

#include "OpenGL/Variables/Location.hpp"

// *****************************************************************************
//! \brief A GLSampler is an OpenGL uniform for texture.
// *****************************************************************************
class GLSampler: public GLLocation
{
public:

    //--------------------------------------------------------------------------
    //! \brief See GLLocation constructor.
    //! \param name
    //! \param gltype
    //! \param texture_id count texture.
    //! \param prog
    //--------------------------------------------------------------------------
    GLSampler(const char* name,
              const GLint gltype,
              const size_t texture_id,
              const GLuint prog)
        : GLLocation(name, 0, static_cast<GLenum>(gltype), prog),
          m_texture_id(GLenum(texture_id))
    {
        m_need_update = true;
    }

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements from CPU and GPU.
    //--------------------------------------------------------------------------
    virtual ~GLSampler() override
    {
        release();
    }

    //--------------------------------------------------------------------------
    //! \brief Return the texture identifier.
    //--------------------------------------------------------------------------
    inline GLint textureID() const
    {
        return static_cast<GLint>(m_texture_id);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Create a new OpenGL Uniform.
    //--------------------------------------------------------------------------
    virtual bool onCreate() override
    {
        m_handle = glCheck(glGetUniformLocation(m_program, cname()));
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Bind the OpenGL Uniform. This is a dummy method. No
    //! action is made.
    //--------------------------------------------------------------------------
    virtual void onActivate() override
    {
        glCheck(glActiveTexture(GL_TEXTURE0 + m_texture_id));
    }

    //--------------------------------------------------------------------------
    //! \brief Setup the behavior of the instance. This is a dummy
    //! method. No action is made.
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief This is a dummy method. No action is made.
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        glCheck(glUniform1i(m_handle, textureID()));
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Unbind the OpenGL Uniform. This is a dummy method. No
    //! action is made.
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override {}

    //--------------------------------------------------------------------------
    //! \brief Destroy the OpenGL Uniform. This is a dummy method. No
    //! action is made.
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {
        // GLLocation::onRelease();
    }

protected:

    const GLenum m_texture_id;
};

#endif // OPENGLCPPWRAPPER_GLSAMPLER_HPP
