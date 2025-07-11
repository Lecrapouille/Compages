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

#ifndef OPENGLCPPWRAPPER_VERTEX_BUFFER_OBJECT_HPP
#define OPENGLCPPWRAPPER_VERTEX_BUFFER_OBJECT_HPP

#include "OpenGL/Buffers/Buffer.hpp"

// *****************************************************************************
//! \brief Buffer for vertex attribute data.
// *****************************************************************************
template <typename T>
class GLVertexBuffer: public GLBuffer<T>
{
public:

    //--------------------------------------------------------------------------
    //! \brief Allows:
    //! \code
    //!   GLVertexBuffer<float> vbo;
    //!   vbo = { 1.0f, 2.0f };
    //! \endcode
    //! Else GLVertexBuffer define a couple of operator=() that hide the
    //! operators inherited from GLBuffer. See
    //! https://stackoverflow.com/questions/64633899/no-inheritance-found-with-
    //! operator-and-initializer-list
    //--------------------------------------------------------------------------
    // using GLBuffer<T>::operator=;

    //--------------------------------------------------------------------------
    //! \brief Constructor with the object name and reserved number of
    //! elements.
    //--------------------------------------------------------------------------
    explicit GLVertexBuffer(std::string const& name,
                            size_t const size,
                            BufferUsage const usage)
        : GLBuffer<T>(name, GL_ARRAY_BUFFER, size, usage)
    {
    }

    GLVertexBuffer() : GLBuffer<T>() {}

    template <typename U>
    inline GLVertexBuffer<T>& operator=(std::initializer_list<U> il)
    {
        PendingContainer<T>::operator=(il);
        return *this;
    }

    template <typename U>
    inline GLVertexBuffer<T>& operator=(std::vector<U> const& other)
    {
        PendingContainer<T>::operator=(other);
        return *this;
    }

    inline GLVertexBuffer<T>& operator=(GLVertexBuffer<T> const& other)
    {
        PendingContainer<T>::operator=(other);
        return *this;
    }
};

#endif // OPENGLCPPWRAPPER_VERTEX_BUFFER_OBJECT_HPP
