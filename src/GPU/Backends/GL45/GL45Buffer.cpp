//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "GPU/Backends/GL45/GL45.hpp"

#include <string>

namespace compages::gpu::backend
{

namespace
{

//------------------------------------------------------------------------------
//! \brief What the driver is allowed to do with the memory afterwards.
//!
//! This is the whole difference between the three usages, and it is a promise
//! rather than a permission: telling the driver that memory will never be
//! written again lets it put the memory where the device reads it fastest.
//! Asking for GL_DYNAMIC_STORAGE_BIT when it is not needed gives that up for
//! nothing.
//------------------------------------------------------------------------------
GLbitfield storageFlags(BufferUsage p_usage)
{
    switch (p_usage)
    {
        case BufferUsage::Immutable:
            return 0;
        case BufferUsage::Dynamic:
        case BufferUsage::Storage:
            return GL_DYNAMIC_STORAGE_BIT;
    }
    return GL_DYNAMIC_STORAGE_BIT;
}

} // namespace

//------------------------------------------------------------------------------
// Two calls, no binding. This is what Direct State Access bought and why the
// backend asks for OpenGL 4.5: before it, the same thing read
//
//     glGenBuffers(1, &name);
//     glBindBuffer(GL_ARRAY_BUFFER, name);
//     glBufferData(GL_ARRAY_BUFFER, bytes, data, usage);
//     glBindBuffer(GL_ARRAY_BUFFER, 0);
//
// where the target had to be picked before the buffer had a purpose, whatever
// was bound before was silently replaced, and forgetting the last line left a
// dangling binding for the next unrelated call to trip over.
//------------------------------------------------------------------------------
Result<NativeId> createBuffer(std::size_t p_bytes,
                              const void* p_data,
                              BufferKind /* p_kind */,
                              BufferUsage p_usage)
{
    GLuint name = 0u;
    glCreateBuffers(1, &name);
    if (name == 0u)
    {
        return failure("the driver refused to create a buffer");
    }

    glNamedBufferStorage(
        name, static_cast<GLsizeiptr>(p_bytes), p_data, storageFlags(p_usage));

    // Nothing here asks glGetError() whether that worked. Out of memory is
    // reported by the debug callback with a sentence naming the size, which is
    // strictly more than an error code would say, and it costs nothing per
    // call.
    return static_cast<NativeId>(name);
}

//------------------------------------------------------------------------------
void destroyBuffer(NativeId p_buffer)
{
    const GLuint name = static_cast<GLuint>(p_buffer);
    glDeleteBuffers(1, &name);
}

//------------------------------------------------------------------------------
void writeBuffer(NativeId p_buffer,
                 std::size_t p_offset,
                 std::size_t p_bytes,
                 const void* p_data)
{
    glNamedBufferSubData(static_cast<GLuint>(p_buffer),
                         static_cast<GLintptr>(p_offset),
                         static_cast<GLsizeiptr>(p_bytes),
                         p_data);
}

//------------------------------------------------------------------------------
// glBindBufferBase rather than glBindBufferRange when the whole buffer is
// meant, because the range form insists on an offset aligned to a device
// dependent number, 256 bytes on much hardware, and refuses a range that is
// not. Passing a zero offset through the range form would work; passing the
// whole buffer through it means saying its size, which the caller would have to
// fetch for no reason.
//------------------------------------------------------------------------------
void bindBufferToPoint(BufferKind p_kind,
                       NativeId p_buffer,
                       int p_binding,
                       std::size_t p_offset,
                       std::size_t p_bytes)
{
    const GLenum target = (p_kind == BufferKind::Storage)
                              ? GL_SHADER_STORAGE_BUFFER
                              : GL_UNIFORM_BUFFER;

    if ((p_offset == 0u) && (p_bytes == 0u))
    {
        glBindBufferBase(target,
                         static_cast<GLuint>(p_binding),
                         static_cast<GLuint>(p_buffer));
        return;
    }

    glBindBufferRange(target,
                      static_cast<GLuint>(p_binding),
                      static_cast<GLuint>(p_buffer),
                      static_cast<GLintptr>(p_offset),
                      static_cast<GLsizeiptr>(p_bytes));
}

//------------------------------------------------------------------------------
void readBuffer(NativeId p_buffer,
                std::size_t p_offset,
                std::size_t p_bytes,
                void* p_data)
{
    glGetNamedBufferSubData(static_cast<GLuint>(p_buffer),
                            static_cast<GLintptr>(p_offset),
                            static_cast<GLsizeiptr>(p_bytes),
                            p_data);
}

} // namespace compages::gpu::backend
