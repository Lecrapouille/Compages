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

#ifndef OPENGLCPPWRAPPER_GLTEXTURE_HPP
#define OPENGLCPPWRAPPER_GLTEXTURE_HPP

// *****************************************************************************
//! \file GLTextures.hpp file implements:
//!   - GLTexture1D:
//!   - GLTexture2D:
//!   - GLTextureDepth2D:
//!   - GLTexture3D: which is a set of 2D Textures.
//!   - GLTextureCube: A 3D Texture specialized for rendering skybox.
// *****************************************************************************

#include "OpenGL/Buffers/PendingContainer.hpp"
#include "OpenGL/GLObject.hpp"

// *****************************************************************************
//! \brief Helper converter CPU to GPU pixel format.
// See OpenGL documentation concerning glTexImage2D()
// *****************************************************************************
GLint CPU2GPUFormat(GLenum format, GLenum type);

// *****************************************************************************
//! \brief Generic Texture.
//!
//! A texture is an OpenGL Object that contains one or more images
//! that all have the same image format. A texture can be used in two
//! ways: either it can be the source of a texture access from a
//! Shader, or it can be used as a render target (FrameBuffer). A
//! texture can be of dimension 1, 2 or 3.
// *****************************************************************************
class GLTexture: public GLObject<GLenum>
{
public:

    //! \brief Internal format storing texture data
    using Buffer = PendingContainer<unsigned char>;

    //! \brief Textures Minification Filter.
    enum class Minification : GLenum
    {
        /* 0x2600 */ NEAREST = GL_NEAREST,
        /* 0x2601 */ LINEAR = GL_LINEAR,
        /* 0x2700 */ NEAREST_MIPMAP_NEAREST = GL_NEAREST_MIPMAP_NEAREST,
        /* 0x2701 */ LINEAR_MIPMAP_NEAREST = GL_LINEAR_MIPMAP_NEAREST,
        /* 0x2702 */ NEAREST_MIPMAP_LINEAR = GL_NEAREST_MIPMAP_LINEAR,
        /* 0x2703 */ LINEAR_MIPMAP_LINEAR = GL_LINEAR_MIPMAP_LINEAR,
    };

    //! \brief Textures Magnification Filter.
    enum class Magnification : GLenum
    {
        /* 0x2600 */ NEAREST = GL_NEAREST,
        /* 0x2601 */ LINEAR = GL_LINEAR,
    };

    //! \brief Textures Wrap.
    enum class Wrap : GLenum
    {
        /* 0x2901 */ REPEAT = GL_REPEAT,
        /* 0x812D */ CLAMP_TO_BORDER = GL_CLAMP_TO_BORDER,
        /* 0x812F */ CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE,
        /* 0x8370 */ MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
    };

    //! \brief Texture Pixel Format (CPU side).
    enum class PixelFormat : GLenum
    {
        /* 0x1901 */ STENCIL_INDEX = GL_STENCIL_INDEX,
        /* 0x1902 */ DEPTH_COMPONENT = GL_DEPTH_COMPONENT,
        /* 0x1903 */ RED = GL_RED,
        /* 0x1906 */ ALPHA = GL_ALPHA,
        /* 0x1907 */ RGB = GL_RGB,
        /* 0x1908 */ RGBA = GL_RGBA,
        /* 0x1909 */ LUMINANCE = GL_LUMINANCE, // Greyscale
        /* 0x190A */ LUMINANCE_ALPHA =
            GL_LUMINANCE_ALPHA, // Luminance with alpha
        /* 0x84F9 */ DEPTH_STENCIL = GL_DEPTH_STENCIL,
    };

    // *****************************************************************************
    //! \brief Default options for textures when setup.
    // *****************************************************************************
    struct Options
    {
        Minification minFilter = Minification::LINEAR;
        Magnification magFilter = Magnification::LINEAR;
        Wrap wrapS = Wrap::REPEAT;
        Wrap wrapT = Wrap::REPEAT;
        Wrap wrapR = Wrap::REPEAT;
        bool generateMipmaps = false;
    };

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor. Should be used if you will load texture from
    //! a picture file (jpg, png ...) with the load() method.
    //!
    //! Give a name to the instance. This constructor makes no other
    //! actions.
    //!
    //! \param name the name of this instance used by GLProgram and GLVAO.
    //! \param target the texture type (GL_TEXTURE_1D .. GL_TEXTURE_3D ...)
    //--------------------------------------------------------------------------
    GLTexture(const uint8_t dimension,
              std::string const& name,
              const GLenum target)
        : GLObject(name, target), m_dimension(dimension)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements in CPU and GPU memories.
    //--------------------------------------------------------------------------
    virtual ~GLTexture()
    {
        release();
    }

    //--------------------------------------------------------------------------
    //! \brief Return the container holding texture data.
    //--------------------------------------------------------------------------
    inline Buffer& data()
    {
        return m_buffer;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the container holding texture data.
    //--------------------------------------------------------------------------
    inline const Buffer& data() const
    {
        return m_buffer;
    }

    //--------------------------------------------------------------------------
    //! \brief \brief Allow to know if data have been transfered into
    //! the CPU memory.
    //!
    //! \return true if texture data have been loaded.
    //--------------------------------------------------------------------------
    virtual inline bool loaded() const
    {
        // Do not check m_width and m_height because FrameBuffer allows to have
        // 0 sizes.
        return 0 != m_buffer.size();
    }

    //--------------------------------------------------------------------------
    //! \brief Change minifier and magnifier options.
    //! \return the reference of this instence.
    //--------------------------------------------------------------------------
    GLTexture& interpolation(Minification const min_filter,
                             Magnification const mag_filter)
    {
        m_options.minFilter = min_filter;
        m_options.magFilter = mag_filter;
        m_need_setup = true;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Change wrapping options for S, T and R.
    //! \return the reference of this instence.
    //--------------------------------------------------------------------------
    GLTexture& wrap(Wrap const wrap)
    {
        m_options.wrapS = wrap;
        m_options.wrapT = wrap;
        m_options.wrapR = wrap;
        m_need_setup = true;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Change wrapping options for S, T and R.
    //! \return the reference of this instence.
    //--------------------------------------------------------------------------
    GLTexture& wrapSTR(Wrap const wrapS, Wrap const wrapT, Wrap const wrapR)
    {
        m_options.wrapS = wrapS;
        m_options.wrapT = wrapT;
        m_options.wrapR = wrapR;
        m_need_setup = true;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Replace current texture settings by a new one.
    //! \return the reference of this instence.
    //--------------------------------------------------------------------------
    GLTexture& options(Options const& options)
    {
        m_options = options;
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the texture dimension (1D, 2D, 3D).
    //--------------------------------------------------------------------------
    inline uint8_t dimension() const
    {
        return m_dimension;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the texture width (in pixel).
    //!
    //! Available for all kind of texture.
    //--------------------------------------------------------------------------
    inline size_t width() const
    {
        return m_width;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the texture height (in pixel).
    //!
    //! Only available for texture 2D and 3D.
    //--------------------------------------------------------------------------
    inline size_t height() const
    {
        return m_height;
    }

    //--------------------------------------------------------------------------
    //! \brief Return the texture depth (in number of textures 2D).
    //!
    //! Only available for texture 3D.
    //--------------------------------------------------------------------------
    inline size_t depth() const
    {
        return m_depth;
    }

    //--------------------------------------------------------------------------
    //! \brief Read the texture data back into CPU memory
    //--------------------------------------------------------------------------
    void repatriate()
    {
        glCheck(glBindTexture(m_target, m_handle));
        glCheck(glGetTexImage(m_target,
                              0,
                              static_cast<GLenum>(m_cpuPixelFormat),
                              static_cast<GLenum>(m_cpuPixelType),
                              m_buffer.to_array()));
    }

protected:

    //--------------------------------------------------------------------------
    //! \brief Apply all settings to the texture.
    //--------------------------------------------------------------------------
    void applyTextureParam()
    {
        glCheck(glTexParameteri(m_target,
                                GL_TEXTURE_MIN_FILTER,
                                static_cast<GLint>(m_options.minFilter)));
        glCheck(glTexParameteri(m_target,
                                GL_TEXTURE_MAG_FILTER,
                                static_cast<GLint>(m_options.magFilter)));
        glCheck(glTexParameteri(
            m_target, GL_TEXTURE_WRAP_S, static_cast<GLint>(m_options.wrapS)));
        glCheck(glTexParameteri(
            m_target, GL_TEXTURE_WRAP_T, static_cast<GLint>(m_options.wrapT)));
        glCheck(glTexParameteri(
            m_target, GL_TEXTURE_WRAP_R, static_cast<GLint>(m_options.wrapR)));

        // TODO
        // GLfloat borderColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f};
        // glCheck(glTexParameterfv(m_target, GL_TEXTURE_BORDER_COLOR,
        // borderColor));
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Check if CPU data of the texture have been modified.
    //! If this is the case "dirty" data will be transfered to the GPU.
    //--------------------------------------------------------------------------
    virtual inline bool needUpdate() const override
    {
        return m_buffer.isPending();
    }

    //--------------------------------------------------------------------------
    //! \brief Create a new OpenGL texture.
    //--------------------------------------------------------------------------
    virtual bool onCreate() override
    {
        glCheck(glGenTextures(1U, &m_handle));
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Bind the Texture to OpenGL.
    //--------------------------------------------------------------------------
    virtual void onActivate() override
    {
        glCheck(glBindTexture(m_target, m_handle));
    }

    //--------------------------------------------------------------------------
    //! \brief Unbind the Texture to OpenGL.
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override
    {
        glCheck(glBindTexture(m_target, 0U));
    }

    //--------------------------------------------------------------------------
    //! \brief Destroy the OpenGL Texture.
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {
        glCheck(glDeleteTextures(1U, &m_handle));
        m_buffer.clear();
        m_width = m_height = m_depth = 0;
        m_cpuPixelFormat = PixelFormat::RGBA;
        m_cpuPixelType = GL_UNSIGNED_BYTE;
    }

protected:

    //! \brief Options to pass to OpenGL
    Options m_options;
    //! \brief Hold the texture (CPU side)
    Buffer m_buffer;
    //! \brief For Texture1D, Texture2D, Texture3D, TextureCube
    size_t m_width = 0u;
    //! \brief For Texture2D, Texture3D, TextureCube
    size_t m_height = 0u;
    //! \brief For Texture3D, TextureCube
    size_t m_depth = 0u;
    //! \brief Desired format of texture once loaded from the picture file (CPU
    //! side).  \note Beware all format are not supported by loaders: ie SOIL
    //! only manages RGB, RGBA, luminance greyscale and luminance with alpha.
    PixelFormat m_cpuPixelFormat = PixelFormat::RGBA;
    size_t m_cpuPixelCount = 4u;
    //! \brief Specify the data type of the GPU pixel data
    GLenum m_cpuPixelType = GL_UNSIGNED_BYTE;
    //! \brief Desired format of texture once loaded into the GPU.
    GLint m_gpuPixelFormat = GL_RGBA;

private:

    //! \brief Texture1D, Texture2D, Texture3D, TextureCube
    const uint8_t m_dimension;
};

#endif // OPENGLCPPWRAPPER_GLTEXTURE_HPP
