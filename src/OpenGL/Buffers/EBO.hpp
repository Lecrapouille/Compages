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

#ifndef OPENGLCPPWRAPPER_ELEMENT_BUFFER_OBJECT_HPP
#define OPENGLCPPWRAPPER_ELEMENT_BUFFER_OBJECT_HPP

#include "OpenGL/Buffers/Buffer.hpp"

// *****************************************************************************
//! \brief Element Buffer Object
// *****************************************************************************
template <typename T>
class GLElementBuffer: public GLBuffer<T>
{
public:

    using Type = T;

    //--------------------------------------------------------------------------
    //! \brief Constructor with the object name
    //--------------------------------------------------------------------------
    explicit GLElementBuffer(std::string const& name, BufferUsage const usage)
        : GLBuffer<T>(name, GL_ELEMENT_ARRAY_BUFFER, usage)
    {
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor with the object name and reserved number of
    //! elements.
    //--------------------------------------------------------------------------
    explicit GLElementBuffer(std::string const& name,
                             const size_t size,
                             BufferUsage const usage)
        : GLBuffer<T>(name, GL_ELEMENT_ARRAY_BUFFER, size, usage)
    {
    }

    template <typename U>
    inline GLElementBuffer<T>& operator=(std::initializer_list<U> il)
    {
        PendingContainer<T>::operator=(il);
        return *this;
    }

    template <typename U>
    inline GLElementBuffer<T>& operator=(std::vector<U> const& other)
    {
        PendingContainer<T>::operator=(other);
        return *this;
    }

    inline GLElementBuffer<T>& operator=(GLElementBuffer<T> const& other)
    {
        PendingContainer<T>::operator=(other);
        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    inline GLenum gltype() const;
};

template <>
inline GLenum GLElementBuffer<uint32_t>::gltype() const
{
    return GL_UNSIGNED_INT;
}

template <>
inline GLenum GLElementBuffer<uint16_t>::gltype() const
{
    return GL_UNSIGNED_SHORT;
}

template <>
inline GLenum GLElementBuffer<uint8_t>::gltype() const
{
    return GL_UNSIGNED_BYTE;
}

// *****************************************************************************
//! \brief
// *****************************************************************************

//! \brief Human Friendly name (hide template misery)
using GLIndex32 = GLElementBuffer<uint32_t>;

//! \brief Human Friendly name (hide template misery)
using GLIndex16 = GLElementBuffer<uint16_t>;

//! \brief Human Friendly name (hide template misery)
using GLIndex8 = GLElementBuffer<uint8_t>;

#endif // OPENGLCPPWRAPPER_ELEMENT_BUFFER_OBJECT_HPP
