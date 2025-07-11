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

#ifndef OPENGLCPPWRAPPER_GLTEXTURE3D_HPP
#define OPENGLCPPWRAPPER_GLTEXTURE3D_HPP

// *****************************************************************************
//! \file GLTexture3D.hpp file implements:
//!   - GLTexture3D: which is a set of 2D Textures.
//!   - GLTextureCube: A 3D Texture specialized for rendering skybox.
// *****************************************************************************

#include "OpenGL/Textures/Texture2D.hpp"

// *****************************************************************************
//! \brief A 3D Texture.
// *****************************************************************************
class GLTexture3D: public GLTexture
{
public:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    GLTexture3D(std::string const& name) : GLTexture(3u, name, GL_TEXTURE_3D) {}

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    template <class L>
    bool load(std::vector<std::string> const& filenames)
    {
        static_assert(std::is_base_of<TextureLoader, L>::value,
                      "Template l is not derived class from TextureLoader");
        L loader;

        m_width = m_height = m_depth = 0u;
        size_t prevWidth = 0;
        size_t prevHeight = 0;

        if (!loader.setPixelFormat(m_cpuPixelFormat))
            return false;

        m_cpuPixelCount = loader.getPixelCount();
        m_cpuPixelType = loader.getPixelType();
        m_gpuPixelFormat =
            CPU2GPUFormat(GLenum(m_cpuPixelFormat), GLenum(m_cpuPixelType));
        if (m_gpuPixelFormat < 0)
            return false;

        m_buffer.clear();
        for (size_t i = 0u; i < filenames.size(); ++i)
        {
            // Load a Texture2D and pack it subsequently into a large 2D texture
            m_width = m_height = 0;
            if (unlikely(!loader.load(
                    filenames[i].c_str(), m_buffer, m_width, m_height)))
                return false;

            // Check consistency of Texture2D dimension
            if ((i != 0u) &&
                ((prevWidth != m_width) || (prevHeight != m_height)))
            {
                std::cerr << "Failed picture file " << i << ": '"
                          << filenames[i] << "' has not correct dimension ("
                          << prevWidth << " x " << prevHeight << ")"
                          << std::endl;
                return false;
            }

            prevWidth = m_width;
            prevHeight = m_height;
        }

        // Success
        m_depth = filenames.size();
        return true;
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Specify to OpenGL a three-dimensional texture image.
    //--------------------------------------------------------------------------
    inline void specifyTexture3D() const
    {
        glCheck(glTexImage3D(m_target,
                             0,
                             static_cast<GLint>(m_gpuPixelFormat),
                             static_cast<GLsizei>(m_width),
                             static_cast<GLsizei>(m_height),
                             static_cast<GLsizei>(m_depth),
                             0,
                             static_cast<GLenum>(m_cpuPixelFormat),
                             static_cast<GLenum>(m_cpuPixelType),
                             m_buffer.to_array()));
    }

    //--------------------------------------------------------------------------
    //! \brief Apply OpenGL texture settings.
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        // Note: m_buffer can nullptr
        if (unlikely(!loaded()))
        {
            std::cerr << "Cannot setup texture '" << name()
                      << "'. Reason 'Data not yet loaded'" << std::endl;
            return true;
        }

        // Data is aligned in byte order
        glCheck(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

        applyTextureParam();
        specifyTexture3D();

        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Upload dirty CPU data to the GPU.
    //! \todo To be implemeted FIXME
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        return false;
    };
};

#endif // OPENGLCPPWRAPPER_GLTEXTURE3D_HPP
