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

#include "OpenGL/Textures/Textures.hpp"

GLint CPU2GPUFormat(GLenum format, GLenum type)
{
    if (format == GL_RGBA)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RGBA8;
        if (type == GL_BYTE)
            return GL_RGBA8_SNORM;
        if (type == GL_HALF_FLOAT)
            return GL_RGBA16F;
        if (type == GL_FLOAT)
            return GL_RGBA32F;
    }
    else if (format == GL_RGBA_INTEGER)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RGBA8UI;
        if (type == GL_BYTE)
            return GL_RGBA8I;
        if (type == GL_UNSIGNED_SHORT)
            return GL_RGBA16UI;
        if (type == GL_SHORT)
            return GL_RGBA16I;
        if (type == GL_INT)
            return GL_RGBA32I;
        if (type == GL_UNSIGNED_INT)
            return GL_RGBA32UI;
    }
    else if (format == GL_RGB)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RGB8;
        if (type == GL_BYTE)
            return GL_RGB8_SNORM;
        if (type == GL_HALF_FLOAT)
            return GL_RGB16F;
        if (type == GL_FLOAT)
            return GL_RGB32F;
    }
    else if (format == GL_RGB_INTEGER)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RGB8UI;
        if (type == GL_BYTE)
            return GL_RGB8I;
        if (type == GL_UNSIGNED_SHORT)
            return GL_RGB16UI;
        if (type == GL_SHORT)
            return GL_RGB16I;
        if (type == GL_INT)
            return GL_RGB32I;
        if (type == GL_UNSIGNED_INT)
            return GL_RGB32UI;
    }
    else if (format == GL_RED)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_R8;
        if (type == GL_BYTE)
            return GL_R8_SNORM;
        if (type == GL_HALF_FLOAT)
            return GL_R16F;
        if (type == GL_FLOAT)
            return GL_R32F;
    }
    else if (format == GL_RED_INTEGER)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_R8UI;
        if (type == GL_BYTE)
            return GL_R8I;
        if (type == GL_UNSIGNED_SHORT)
            return GL_R8UI;
        if (type == GL_SHORT)
            return GL_R16I;
        if (type == GL_INT)
            return GL_R32I;
        if (type == GL_UNSIGNED_INT)
            return GL_R32UI;
    }
    else if (format == GL_RG)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RG8;
        if (type == GL_BYTE)
            return GL_RG8_SNORM;
        if (type == GL_HALF_FLOAT)
            return GL_RG16F;
        if (type == GL_FLOAT)
            return GL_RG32F;
    }
    else if (format == GL_RG_INTEGER)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_RG8UI;
        if (type == GL_BYTE)
            return GL_RG8I;
        if (type == GL_UNSIGNED_SHORT)
            return GL_RG8UI;
        if (type == GL_SHORT)
            return GL_RG16I;
        if (type == GL_INT)
            return GL_RG32I;
        if (type == GL_UNSIGNED_INT)
            return GL_RG32UI;
    }
    else if (format == GL_LUMINANCE)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_LUMINANCE;
    }
    else if (format == GL_LUMINANCE_ALPHA)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_LUMINANCE_ALPHA;
    }
    else if (format == GL_ALPHA)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_ALPHA;
    }
    else if (format == GL_DEPTH_COMPONENT)
    {
        if (type == GL_UNSIGNED_SHORT)
            return GL_DEPTH_COMPONENT16;
        if (type == GL_UNSIGNED_INT)
            return GL_DEPTH_COMPONENT24;
        if (type == GL_FLOAT)
            return GL_DEPTH_COMPONENT32F;
    }
    else if (format == GL_DEPTH_STENCIL)
    {
        if (type == GL_UNSIGNED_INT_24_8)
            return GL_DEPTH24_STENCIL8;
        if (type == GL_FLOAT_32_UNSIGNED_INT_24_8_REV)
            return GL_DEPTH32F_STENCIL8;
    }
    else if (format == GL_STENCIL_INDEX)
    {
        if (type == GL_UNSIGNED_BYTE)
            return GL_STENCIL_INDEX8;
    }

    std::cerr << "Incompatible GPU pixel format" << std::endl;
    return -1;
}
