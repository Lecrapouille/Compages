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

#ifndef OPENGLCPPWRAPPER_TEXTURE1D_HPP
#define OPENGLCPPWRAPPER_TEXTURE1D_HPP

// *****************************************************************************
//! \file GLTexture1D.hpp file implements:
//!   - GLTexture1D:
// *****************************************************************************

#include "OpenGL/Textures/Texture.hpp"
#include <iostream>

// *****************************************************************************
//! \brief A 1D Texture.
// *****************************************************************************
class GLTexture1D: public GLTexture
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    GLTexture1D(std::string const& name) : GLTexture(1u, name, GL_TEXTURE_1D) {}

private:

    //--------------------------------------------------------------------------
    //! \brief sApply OpenGL texture settings.
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        if (unlikely(!loaded()))
        {
            std::cerr << "Cannot setup texture '" << name()
                      << "'. Reason 'Data not yet loaded'" << std::endl;
            return true;
        }

        glCheck(glTexImage1D(m_target,
                             0,
                             static_cast<GLint>(m_gpuPixelFormat),
                             static_cast<GLsizei>(m_width),
                             0,
                             static_cast<GLenum>(m_cpuPixelFormat),
                             static_cast<GLenum>(m_cpuPixelType),
                             nullptr));
        applyTextureParam();
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Upload dirty CPU data to the GPU.
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        size_t pos_start;
        size_t pos_end;
        m_buffer.getPending(pos_start, pos_end);

        // FIXME: pour le moment on envoie toute la texture entiere
        // au lieu de la portion modifiee.
        // TODO pendingData --> x,width
        const GLint x = 0U;
        const GLsizei width = static_cast<GLsizei>(m_width);

        glCheck(glTexSubImage1D(m_target,
                                0,
                                x,
                                width,
                                static_cast<GLenum>(m_cpuPixelFormat),
                                static_cast<GLenum>(m_cpuPixelType),
                                m_buffer.to_array()));

        m_buffer.clearPending();
        return false;
    }
};

#endif // OPENGLCPPWRAPPER_TEXTURE1D_HPP
