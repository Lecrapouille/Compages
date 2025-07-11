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

#ifndef TEXTURES_LOADER_HPP
#define TEXTURES_LOADER_HPP

#include "OpenGL/Textures/Texture.hpp"

// ***************************************************************************
//! \brief Interface class for loading and saving 2D texture from picture file
//! (jpeg, bmp, png ...). The derived class shall implement concretly these
//! method using for example an external library (SOIL ...)
// ***************************************************************************
class TextureLoader
{
public:

    //--------------------------------------------------------------------------
    //! \brief Virtual destructor needed because of pure virtual methods.
    //--------------------------------------------------------------------------
    virtual ~TextureLoader() = default;

    //--------------------------------------------------------------------------
    //! \brief Mandatory method allowing to configurate the texture. This method
    //! has to be called before methods such as load(), save() which depends on
    //! this configuration.
    //!
    //! \param[in] pixelformat the desired format pixel format of the texture.
    //! (i.e. GLTexture::PixelFormat::RGBA).
    //!
    //! \return true if the desired setting is valid, else return false and the
    //! error message can be get through the method error().
    //--------------------------------------------------------------------------
    virtual bool setPixelFormat(GLTexture::PixelFormat const pixelformat) = 0;

    //--------------------------------------------------------------------------
    //! This method return the type of OpenGL pixel. For example with SOIL
    //! library only array of unsigned chars is managed and therefore will
    //! return GL_UNSIGNED_BYTE.
    //--------------------------------------------------------------------------
    virtual GLenum getPixelType() const = 0;

    //--------------------------------------------------------------------------
    //! \brief This method return the number of channels used if and only if the
    //! setPixelFormat() has been called with success. For example if RGBA was
    //! passed to setPixelFormat() then this method will return 4.
    //--------------------------------------------------------------------------
    virtual size_t getPixelCount() const = 0;

    //--------------------------------------------------------------------------
    //! \brief Load a picture file (jpeg, bmp, png, ...) from the disk into an
    //! array of unsigned chars that can manipulated from CPU and GPU.
    //!
    //! \param[in] filename the path of the  picture file (jpeg, bmp, png, ...).
    //! \param[in, out] The OpenGL texture buffer that will hold the texture.
    //! \param[out] width Return the width of the loaded texture.
    //! \param[out] height Return the height of the loaded texture.
    //! \return true if the textures has been loaded with success, esle return
    //! false and the error message can be get through the method error().
    //--------------------------------------------------------------------------
    virtual bool load(std::string const& filename,
                      GLTexture::Buffer& buffer,
                      size_t& width,
                      size_t& height) = 0;

    //--------------------------------------------------------------------------
    //! \brief Save the OpenGL texture into a picture file (jpeg, bmp, png ...)
    //! from the disk.
    //!
    //! \param[in] filename the path of the  picture file (bmp, tga or dds).
    //! \param[in] The OpenGL texture buffer that holds the texture.
    //! \param[in] width Return the width of the loaded texture.
    //! \param[in] height Return the height of the loaded texture.
    //! \return true if the textures has been saved with success, esle return
    //! false and the error message can be get through the method error().
    //--------------------------------------------------------------------------
    virtual bool save(std::string const& filename,
                      GLTexture::Buffer const& buffer,
                      size_t const width,
                      size_t const height) = 0;

    //--------------------------------------------------------------------------
    //! \brief Return the last errorr (if occured).
    //--------------------------------------------------------------------------
    inline std::string const& error() const
    {
        return m_error;
    }

protected:

    //! \brief Store the current error.
    std::string m_error;
};

#endif
