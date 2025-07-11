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

#include "Loaders/Textures/SOIL.hpp"
#include "Common/File.hpp"

//------------------------------------------------------------------------------
bool SOIL::setPixelFormat(GLTexture::PixelFormat const cpuformat)
{
    m_error.clear();

    switch (cpuformat)
    {
        case GLTexture::PixelFormat::RGBA:
            m_soilFormat = SOIL_LOAD_RGBA;
            m_pixelCount = 4_z;
            m_isValid = true;
            break;
        case GLTexture::PixelFormat::RGB:
            m_soilFormat = SOIL_LOAD_RGB;
            m_pixelCount = 3_z;
            m_isValid = true;
            break;
        case GLTexture::PixelFormat::LUMINANCE:
            m_soilFormat = SOIL_LOAD_L;
            m_pixelCount = 1_z;
            m_isValid = true;
            break;
        case GLTexture::PixelFormat::LUMINANCE_ALPHA:
            m_soilFormat = SOIL_LOAD_LA;
            m_pixelCount = 2_z;
            m_isValid = true;
            break;
        case GLTexture::PixelFormat::STENCIL_INDEX:
        case GLTexture::PixelFormat::DEPTH_COMPONENT:
        case GLTexture::PixelFormat::RED:
        case GLTexture::PixelFormat::ALPHA:
        case GLTexture::PixelFormat::DEPTH_STENCIL:
        default:
            m_error = "SOIL does not suport the given CPU pixel format";
            std::cerr << m_error << std::endl;
            // m_soilFormat = SOIL_LOAD_AUTO;
            m_isValid = false;
            break;
    }

    m_pixelType = GL_UNSIGNED_BYTE; // Only managed by SOIL
    return m_isValid;
}

//------------------------------------------------------------------------------
bool SOIL::load(std::string const& filename,
                GLTexture::Buffer& buffer,
                size_t& width,
                size_t& height)
{
    if (unlikely(!m_isValid))
    {
        m_error = "Failed loading picture file '" + filename +
                  "'. Reason was: '" +
                  "the setPixelFormat() method previously return false " +
                  "or you have never called it !'";
        std::cerr << m_error << std::endl;
        return false;
    }

    // Load the image as a C array.
    int w, h;
    unsigned char* image = SOIL_load_image(
        filename.c_str(), &w, &h, 0, static_cast<int>(m_soilFormat));
    if (likely(nullptr != image))
    {
        // Use the max because with framebuffer we can resize texture
        width = std::max(width, static_cast<size_t>(w)); // FIXME
        height = std::max(height, static_cast<size_t>(h));

        // Convert it as std::vector
        size_t size =
            static_cast<size_t>(w * h) * m_pixelCount * sizeof(unsigned char);
        buffer.append(image, size); // FIXME: not working with preallocated size
        SOIL_free_image_data(image);
        m_error.clear();
        return true;
    }
    else
    {
        width = height = 0;
        buffer.clear();
        m_error = "Failed loading picture file '" + filename +
                  "'. Reason was: '" + SOIL_last_result() + "'";
        std::cerr << m_error << std::endl;
        return false;
    }
}

//------------------------------------------------------------------------------
bool SOIL::save(std::string const& filename,
                GLTexture::Buffer const& texture,
                size_t const width,
                size_t const height)
{
    if (unlikely(!m_isValid))
    {
        m_error = "Failed saving picture file '" + filename +
                  "'. Reason was: '" +
                  "the setPixelFormat() method previously return false " +
                  "or you have never called it !'";
        std::cerr << m_error << std::endl;
        return false;
    }

    std::string ext = File::extension(filename);
    if (ext == "bmp")
        m_soilFormat = SOIL_SAVE_TYPE_BMP;
    else if (ext == "tga")
        m_soilFormat = SOIL_SAVE_TYPE_TGA;
    else if (ext == "dds")
        m_soilFormat = SOIL_SAVE_TYPE_DDS;
    else
    {
        m_error = "Failed saving picture file '" + filename +
                  "'. Reason was: '" + "'unsuported file format " + ext + "'";
        std::cerr << m_error << std::endl;
        return false;
    }

    const unsigned char* buffer = texture.to_array();
    if (unlikely(nullptr == buffer))
    {
        m_error = "Failed saving picture file '" + filename +
                  "'. Reason was: '" + "'Cannot save a texture with no buffer'";
        std::cerr << m_error << std::endl;
        return false;
    }

    bool res = !!SOIL_save_image(filename.c_str(),
                                 m_soilFormat,
                                 static_cast<int>(width),
                                 static_cast<int>(height),
                                 static_cast<int>(m_pixelCount),
                                 buffer);
    if (unlikely(!res))
    {
        m_error = "Failed saving picture file '" + filename +
                  "'. Reason was: '" + SOIL_last_result() + "'";
        std::cerr << m_error << std::endl;
        return false;
    }

    m_error.clear();
    return true;
}
