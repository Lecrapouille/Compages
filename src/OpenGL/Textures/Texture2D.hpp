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

#ifndef OPENGLCPPWRAPPER_GLTEXTURE2D_HPP
#define OPENGLCPPWRAPPER_GLTEXTURE2D_HPP

// *****************************************************************************
//! \file GLTexture2D.hpp file implements:
//!   - GLTexture2D:
//!   - GLTextureDepth2D:
// *****************************************************************************

#include "Loaders/TextureLoader.hpp"
#include "OpenGL/Textures/Texture.hpp"
#include <iostream>

// *****************************************************************************
//! \brief A 2D Texture.
// *****************************************************************************
class GLTexture2D: public GLTexture
{
    //! \brief GLTextureCube is made of GLTexture2D. Let it access to
    //! GLTexture2D private states.
    friend class GLTextureCube;
    friend class GLTextureBuffer;

public:

    //--------------------------------------------------------------------------
    //! \brief Constructor. Should be used if you will load texture from
    //! a picture file (jpg, png ...) with the load() method.
    //!
    //! Give a name to the instance. This constructor makes no other
    //! actions.
    //!
    //! \param name the name of this instance used by GLProgram and GLVAO.
    //--------------------------------------------------------------------------
    GLTexture2D(std::string const& name) : GLTexture(2u, name, GL_TEXTURE_2D) {}

    //--------------------------------------------------------------------------
    //! \brief Constructor. Should be used if you will load texture from
    //! a frame buffer.
    //!
    //! Give a name to the instance. Get the texture size. This
    //! constructor makes no other actions.
    //!
    //! \param name the name of this instance used by GLProgram and GLVAO.
    //! \param target the texture type (GL_TEXTURE_1D .. GL_TEXTURE_3D ...)
    //! \param width Buffer width (pixels). Shall be > 0.
    //! \param height Buffer height (pixel). Shall be > 0.
    //--------------------------------------------------------------------------
    GLTexture2D(std::string const& name,
                const uint32_t width,
                const uint32_t height)
        : GLTexture(2u, name, GL_TEXTURE_2D)
    {
        // Note: Allow texture with no size for FrameBuffers
        m_width = width;
        m_height = height;
    }

    //--------------------------------------------------------------------------
    //! \brief Destructor. Release elements in CPU and GPU memories.
    //--------------------------------------------------------------------------
    virtual ~GLTexture2D() = default;

    //--------------------------------------------------------------------------
    //! \brief Allow to know if data have been transfered into the CPU
    //! memory.
    //!
    //! \return true if texture data have been loaded (a jpeg, png, bmp
    //! file have been loaded)
    //--------------------------------------------------------------------------
    virtual inline bool loaded() const override
    {
        return // Texture loaded from a file (jpeg ...)
            GLTexture::loaded() ||
            // Dummy textures accepted by FrameBuffer
            ((0 != m_width) && (0 != m_height));
    }

    //--------------------------------------------------------------------------
    //! \brief Load a jpeg, png, bmp ... file as texture.
    //!
    //! \param filename the path of the jpeg, png, bmp file.
    //! \param rename if set to true then the name of this instance (set
    //! in the constructor) takes the name of the file. Else, the
    //! instance name is not mdified.
    //! \tparam L: class deriving from TextureLoader (ie SOIL).
    //!
    //! \return true if texture data have been loaded.
    //--------------------------------------------------------------------------
    template <class L>
    inline bool load(
        std::string const& filename) // TODO hold the filename. name == filename
    {
        static_assert(std::is_base_of<TextureLoader, L>::value,
                      "Template L is not derived class from TextureLoader");
        L loader;
        return doload(loader, filename.c_str());
    }

    //--------------------------------------------------------------------------
    //! \brief Load a jpeg, png, bmp ... file as texture.
    //!
    //! \param filename the path of the jpeg, png, bmp file.
    //! \param rename if set to true then the name of this instance (set
    //! in the constructor) takes the name of the file. Else, the
    //! instance name is not mdified.
    //! \tparam L: class deriving from TextureLoader (ie SOIL).
    //!
    //! \return true if texture data have been loaded.
    //--------------------------------------------------------------------------
    template <class L>
    inline bool
    load(const char* const filename) // TODO hold the filename. name == filename
    {
        static_assert(std::is_base_of<TextureLoader, L>::value,
                      "Template L is not derived class from TextureLoader");
        L loader;
        return doload(loader, filename);
    }

    //--------------------------------------------------------------------------
    //! \brief Save the texture into a picture file depending on the file
    //! extension on filename.
    //! \note Beware loaders may not manage all picture format. For example
    //! SOIL only manages BMP, TGA and DDS.
    //! \tparam L: class deriving from TextureLoader (ie SOIL).
    //!
    //! \return true if texture data have been writen in the file, else false.
    //--------------------------------------------------------------------------
    template <class L>
    inline bool save(std::string const& filename)
    {
        static_assert(std::is_base_of<TextureLoader, L>::value,
                      "Template L is not derived class from TextureLoader");
        L loader;
        return dosave(loader, filename.c_str());
    }

    //--------------------------------------------------------------------------
    //! \brief Save the texture into a picture file depending on the file
    //! extension on filename.
    //! \note Beware loaders may not manage all picture format. For example
    //! SOIL only manages BMP, TGA and DDS.
    //! \tparam L: class deriving from TextureLoader (ie SOIL).
    //!
    //! \return true if texture data have been writen in the file, else false.
    //--------------------------------------------------------------------------
    template <class L>
    bool save(const char* const filename)
    {
        static_assert(std::is_base_of<TextureLoader, L>::value,
                      "Template L is not derived class from TextureLoader");
        L loader;
        return dosave(loader, filename);
    }

    //--------------------------------------------------------------------------
    //! \brief Set to the nth byte of the texture (write access).
    //--------------------------------------------------------------------------
    inline unsigned char& set(std::size_t const nth)
    {
        // TBD ?
        // if (nth > m_width * m_height)
        //   {
        //     reserve(nth);
        //   }
        return m_buffer.set(nth);
    }

    //--------------------------------------------------------------------------
    //! \brief Get to the nth byte of the texture (read only access).
    //--------------------------------------------------------------------------
    inline const unsigned char& get(std::size_t const nth) const
    {
        return m_buffer.get(nth);
    }

    //--------------------------------------------------------------------------
    //! \brief Set to the nth byte of the texture (write access).
    //--------------------------------------------------------------------------
    inline unsigned char& set(const size_t u, const size_t v, const size_t off)
    {
        return GLTexture2D::set((u * m_width + v) * 4 +
                                off); // TODO 4 because RGBA
    }

    //--------------------------------------------------------------------------
    //! \brief Get to the nth byte of the texture (read only access.
    //--------------------------------------------------------------------------
    inline const unsigned char&
    get(const size_t u, const size_t v, const size_t off) const
    {
        return GLTexture2D::get((u * m_width + v) * 4 + off);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief Load a jpeg, png, bmp ... file as texture.
    //!
    //! \param filename the path of the jpeg, png, bmp file.
    //! \param rename if set to true then the name of this instance (set
    //! in the constructor) takes the name of the file. Else, the
    //! instance name is not mdified.
    //!
    //! \return true if texture data have been loaded.
    //--------------------------------------------------------------------------
    bool doload(TextureLoader& loader, const char* const filename)
    {
        if (!loader.setPixelFormat(m_cpuPixelFormat))
            return false;

        m_cpuPixelCount = loader.getPixelCount();
        m_cpuPixelType = loader.getPixelType();
        m_gpuPixelFormat =
            CPU2GPUFormat(GLenum(m_cpuPixelFormat), GLenum(m_cpuPixelType));
        if (m_gpuPixelFormat < 0)
            return false;

        m_buffer.clear();
        m_width = m_height = 0;
        return loader.load(filename, m_buffer, m_width, m_height);
    }

    //--------------------------------------------------------------------------
    //! \brief Save the texture into a picture file depending on the file
    //! extension on filename.
    //! \note Beware loaders may not manage all picture format. For example
    //! SOIL only manages BMP, TGA and DDS.
    //!
    //! \return true if texture data have been writen in the file, else false.
    //--------------------------------------------------------------------------
    bool dosave(TextureLoader& loader, const char* const filename)
    {
        if (loader.setPixelFormat(m_cpuPixelFormat))
            return loader.save(filename, m_buffer, m_width, m_height);
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Specify to OpenGL a two-dimensional texture image.
    //--------------------------------------------------------------------------
    inline void specifyTexture2D() const
    {
        // Note: is allowed this case:
        // m_width != 0 and m_height != 0 and buffer == nullptr
        // This will reserve the buffer size.
        glCheck(glTexImage2D(m_target,
                             0,
                             static_cast<GLint>(m_gpuPixelFormat),
                             static_cast<GLsizei>(m_width),
                             static_cast<GLsizei>(m_height),
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

        applyTextureParam();
        specifyTexture2D();
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief Upload dirty CPU data to the GPU.
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        // TODO
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"

        size_t start, stop;
        m_buffer.getPending(start, stop);

        start = start / m_cpuPixelCount;
        stop = stop / m_cpuPixelCount;
        const GLint x = start / m_width;
        const GLint y = start % m_width;
        const GLsizei width = (stop / m_width) - x;
        const GLsizei height = (stop % m_width) - y;

        // FIXME: not working if width and height are not the texture size
        glCheck(glTexSubImage2D(m_target,
                                0,
                                x,
                                y,
                                width,
                                height,
                                static_cast<GLenum>(m_cpuPixelFormat),
                                static_cast<GLenum>(m_cpuPixelType),
                                m_buffer.to_array()));

        m_buffer.clearPending();
        return false;
#pragma GCC diagnostic pop
    };
};

// *****************************************************************************
//! \brief a 2D Texture Float
// *****************************************************************************
class GLTextureFloat2D: public GLTexture2D
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //!
    //! Give a name to the instance. This constructor makes no other
    //! actions.
    //!
    //! \param name the name of this instance used by GLProgram and GLVAO.
    //--------------------------------------------------------------------------
    GLTextureFloat2D(std::string const& name) : GLTexture2D(name)
    {
        m_cpuPixelFormat = PixelFormat::RGBA;
        m_cpuPixelType = GL_FLOAT;
        m_gpuPixelFormat = GL_RGBA32F;
    }
};

// *****************************************************************************
//! \brief a 2D Texture storing the depth information of the scene.
// *****************************************************************************
class GLTextureDepth2D: public GLTexture2D
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor.
    //!
    //! Give a name to the instance. This constructor makes no other
    //! actions.
    //!
    //! \param name the name of this instance used by GLProgram and GLVAO.
    //--------------------------------------------------------------------------
    GLTextureDepth2D(std::string const& name) : GLTexture2D(name)
    {
        m_cpuPixelFormat = PixelFormat::DEPTH_COMPONENT;
        m_cpuPixelType = GL_UNSIGNED_SHORT;
        m_gpuPixelFormat = GL_DEPTH_COMPONENT16;
    }
};

#endif // OPENGLCPPWRAPPER_GLTEXTURE2D_HPP
