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

#ifndef OPENGLCPPWRAPPER_GLBUFFER_HPP
#define OPENGLCPPWRAPPER_GLBUFFER_HPP

#include "OpenGL/Buffers/PendingContainer.hpp"
#include "OpenGL/GLObject.hpp"

//! \brief Specifies the expected usage pattern of the data store for
//! glBufferData.
enum class BufferUsage : GLenum
{
    /* 0x88E0 */ STREAM_DRAW = GL_STREAM_DRAW,
    /* 0x88E4 */ STATIC_DRAW = GL_STATIC_DRAW,
    /* 0x88E8 */ DYNAMIC_DRAW = GL_DYNAMIC_DRAW,
};

// FIXME Workaround to have VAO::VBOs[0].size()
class IGLBuffer: public GLObject<GLenum>
{
public:

    explicit IGLBuffer(std::string const& name, GLenum const target)
        : GLObject(name, target)
    {
    }

    virtual size_t size() const = 0;
};

// *****************************************************************************
//! \brief GLBuffer is a class of contigous elements (array) CPU side that is
//! aware of area of dirty elements that needs to be updated to the GPU.
//! GLBuffer can be used to store vertex data in Vertex Buffer Object (VBO),
//! pixels data retrieved from images (texture) or for framebuffer. Mostly of
//! time vertices in a VBO are indexed by a Element Buffer Object (EBO) which is
//! also a GLBuffer.
// *****************************************************************************
template <typename T>
class GLBuffer: public IGLBuffer, public PendingContainer<T>
{
public:

    //--------------------------------------------------------------------------
    //! \brief Constructor with the object name.
    //! \param target: specialization into GL_ARRAY_BUFFER,
    //! GL_ELEMENT_ARRAY_BUFFER ...
    //! \param[in] usage:
    //!   - BufferUsage::STREAM_DRAW: The data store contents will be modified
    //!   once and
    //!     used at most a few times.
    //!   - BufferUsage::STATIC_DRAW: The data store contents will be modified
    //!   once and
    //!     used many times.
    //!   - BufferUsage::DYNAMIC_DRAW: The data store contents will be modified
    //!   repeatedly
    //!     and used many times.
    //--------------------------------------------------------------------------
    explicit GLBuffer(std::string const& name,
                      GLenum const target,
                      BufferUsage const usage)
        : IGLBuffer(name, target)
    {
        m_usage = static_cast<GLenum>(usage);
    }

    //--------------------------------------------------------------------------
    //! \brief Constructor with the object name and reserved number of elements.
    //--------------------------------------------------------------------------
    explicit GLBuffer(std::string const& name,
                      GLenum const target,
                      size_t const size,
                      BufferUsage const usage)
        : IGLBuffer(name, target), PendingContainer<T>(size)
    {
        m_usage = static_cast<GLenum>(usage);
    }

    // FIXME: can be removed ?
    explicit GLBuffer() : IGLBuffer("", GL_ARRAY_BUFFER)
    {
        m_usage = static_cast<GLenum>(BufferUsage::STATIC_DRAW);
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual ~GLBuffer() override
    {
        release();
    }

    // FIXME: workaround
    virtual size_t size() const override
    {
        return PendingContainer<T>::size();
    }

    //--------------------------------------------------------------------------
    //! \brief Set the usage for VBOs when they are created:
    //!   - BufferUsage::STREAM_DRAW: The data store contents will be modified
    //!   once and
    //!     used at most a few times.
    //!   - BufferUsage::STATIC_DRAW: The data store contents will be modified
    //!   once and
    //!     used many times.
    //!   - BufferUsage::DYNAMIC_DRAW: The data store contents will be modified
    //!   repeatedly
    //!     and used many times.
    //! \note: to be called before calling begin(), else it will not talen into
    //! account.
    //--------------------------------------------------------------------------
    void usage(BufferUsage const type)
    {
        m_usage = static_cast<GLenum>(type);
    }

private:

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual bool onCreate() override
    {
        glCheck(glGenBuffers(1, &m_handle));
        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void onActivate() override
    {
        glCheck(glBindBuffer(m_target, m_handle));
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual bool onSetup() override
    {
        const GLsizeiptr bytes = static_cast<GLsizeiptr>(
            PendingContainer<T>::capacity() * sizeof(T));
        glCheck(glBufferData(m_target, bytes, NULL, m_usage));

        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual inline bool needUpdate() const override
    {
        return PendingContainer<T>::isPending();
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual bool onUpdate() override
    {
        size_t pos_start, pos_end;
        PendingContainer<T>::getPending(pos_start, pos_end);
        PendingContainer<T>::clearPending();

        size_t offset = sizeof(T) * pos_start;
        size_t nbytes = sizeof(T) * (pos_end - pos_start);
        glCheck(glBufferSubData(m_target,
                                static_cast<GLintptr>(offset),
                                static_cast<GLsizeiptr>(nbytes),
                                PendingContainer<T>::to_array()));

        return false;
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void onDeactivate() override
    {
        glCheck(glBindBuffer(m_target, 0));
    }

    //--------------------------------------------------------------------------
    //! \brief
    //--------------------------------------------------------------------------
    virtual void onRelease() override
    {
        glCheck(glDeleteBuffers(1, &m_handle));
    }

private:

    GLenum m_usage;
};

#endif // OPENGLCPPWRAPPER_GLBUFFER_HPP
