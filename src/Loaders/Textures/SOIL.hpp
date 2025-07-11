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

#ifndef OPENGLCPPWRAPPER_SOIL_TEXTURES_LOADER_HPP
#define OPENGLCPPWRAPPER_SOIL_TEXTURES_LOADER_HPP

#include "Loaders/TextureLoader.hpp"
#include "SOIL/SOIL.h"
#include <iostream>

// *****************************************************************************
//! \brief Class wrapping SOIL library for loading and saving textures
// *****************************************************************************
class SOIL: public TextureLoader
{
public:

    //--------------------------------------------------------------------------
    //! \brief See documention from TextureLoader class.
    //! Accepted format:
    //!   - GLTexture::PixelFormat::RGBA
    //!   - GLTexture::PixelFormat::RGB
    //!   - GLTexture::PixelFormat::LUMINANCE
    //!   - GLTexture::PixelFormat::LUMINANCE_ALPHA
    //--------------------------------------------------------------------------
    virtual bool
    setPixelFormat(GLTexture::PixelFormat const cpuformat) override;

    //--------------------------------------------------------------------------
    //! \brief See documention from TextureLoader class.
    //! Will always return GL_UNSIGNED_BYTE
    //--------------------------------------------------------------------------
    virtual GLenum getPixelType() const override
    {
        return m_pixelType;
    }

    //--------------------------------------------------------------------------
    //! \brief See documention from TextureLoader class.
    //! Depending on param passed to setPixelFormat(), will return:
    //!   - 4 if GLTexture::PixelFormat::RGBA was passed.
    //!   - 3 if GLTexture::PixelFormat::RGB was passed.
    //!   - 1 if GLTexture::PixelFormat::LUMINANCE was passed.
    //!   - 2 if GLTexture::PixelFormat::LUMINANCE_ALPHA was passed.
    //--------------------------------------------------------------------------
    virtual size_t getPixelCount() const override
    {
        return m_pixelCount;
    }

    //--------------------------------------------------------------------------
    //! \brief See documention from TextureLoader class.
    //! Accepted format:
    //!   - BMP: non-1bpp, non-RLE (from stb_image documentation)
    //!   - PNG: non-interlaced (from stb_image documentation)
    //!   - JPG: JPEG baseline (from stb_image documentation)
    //!   - TGA: greyscale or RGB or RGBA or indexed, uncompressed or RLE
    //!   - DDS: DXT1/2/3/4/5, uncompressed, cubemaps (can't read 3D DDS files
    //!          yet)
    //!   - PSD: (from stb_image documentation)
    //!   - HDR: converted to LDR, unless loaded with HDR functions (RGBE or
    //!          RGBdivA or RGBdivA2)
    //--------------------------------------------------------------------------
    virtual bool load(std::string const&,
                      GLTexture::Buffer& buffer,
                      size_t& width,
                      size_t& height) override;

    //--------------------------------------------------------------------------
    //! \brief See documention from TextureLoader class.
    //! Accepted format:
    //!   - TGA: Greyscale or RGB or RGBA, uncompressed
    //!   - BMP: RGB, uncompressed
    //!   - DDS: RGB as DXT1, or RGBA as DXT5
    //--------------------------------------------------------------------------
    virtual bool save(std::string const&,
                      GLTexture::Buffer const& buffer,
                      size_t const width,
                      size_t const height) override;

private:

    size_t m_pixelCount = 0;
    int m_soilFormat = SOIL_LOAD_AUTO;
    GLenum m_pixelType = GL_UNSIGNED_BYTE;
    bool m_isValid = false;
};

#endif // OPENGLCPPWRAPPER_SOIL_TEXTURES_LOADER_HPP
