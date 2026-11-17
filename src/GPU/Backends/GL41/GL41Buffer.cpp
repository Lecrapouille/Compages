// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <cstring>

namespace compages::gpu::backend
{

namespace
{

GLenum usageOf(BufferUsage p_usage)
{
    switch (p_usage)
    {
        case BufferUsage::Static:
            return GL_STATIC_DRAW;
        case BufferUsage::Stream:
            return GL_STREAM_DRAW;
        case BufferUsage::Dynamic:
            return GL_DYNAMIC_DRAW;
    }
    return GL_DYNAMIC_DRAW;
}

} // namespace

Result<NativeId> createBuffer(std::size_t p_bytes,
                              const void* p_data,
                              BufferKind p_kind,
                              BufferUsage p_usage)
{
    GLuint name = 0u;
    glGenBuffers(1, &name);
    if (name == 0u)
    {
        return failure("the driver refused to create a buffer");
    }

    // The first target a buffer is bound to is its kind for WebGL 2, which
    // then refuses to read indices from a buffer first bound as anything
    // else, and the other way round. Indices are bound with no vertex array
    // bound, since the index buffer is part of the vertex array's state.
    GLenum target = GL_ARRAY_BUFFER;
    if (p_kind == BufferKind::Index)
    {
        detail::bindVertexArray(0u);
        target = GL_ELEMENT_ARRAY_BUFFER;
    }
    else if (p_kind == BufferKind::Uniform)
    {
        target = GL_UNIFORM_BUFFER;
    }
    glBindBuffer(target, name);
    glBufferData(target, static_cast<GLsizeiptr>(p_bytes), p_data, usageOf(p_usage));
    glBindBuffer(target, 0u);
    return static_cast<NativeId>(name);
}

void destroyBuffer(NativeId p_buffer)
{
    const GLuint buffer = static_cast<GLuint>(p_buffer);
    detail::forgetBuffer(buffer);
    detail::dropStorageCopy(buffer);
    glDeleteBuffers(1, &buffer);
}

void writeBuffer(NativeId p_buffer,
                 std::size_t p_offset,
                 std::size_t p_bytes,
                 const void* p_data)
{
    // A copy target is bound to nothing else, whatever the kind of buffer.
    glBindBuffer(GL_COPY_WRITE_BUFFER, static_cast<GLuint>(p_buffer));
    glBufferSubData(GL_COPY_WRITE_BUFFER,
                    static_cast<GLintptr>(p_offset),
                    static_cast<GLsizeiptr>(p_bytes),
                    p_data);
    glBindBuffer(GL_COPY_WRITE_BUFFER, 0u);
    detail::touchBuffer(static_cast<GLuint>(p_buffer));
}

void bindBufferToPoint(BufferKind p_kind,
                       NativeId p_buffer,
                       int p_binding,
                       std::size_t p_offset,
                       std::size_t p_bytes)
{
    if (p_kind == BufferKind::Storage)
    {
        // No storage binding point on this device: the emulated compute
        // reads the table at dispatch().
        detail::bindStoragePoint(static_cast<GLuint>(p_buffer), p_binding, p_offset, p_bytes);
        return;
    }

    if ((p_offset == 0u) && (p_bytes == 0u))
    {
        glBindBufferBase(GL_UNIFORM_BUFFER, static_cast<GLuint>(p_binding), static_cast<GLuint>(p_buffer));
        return;
    }
    glBindBufferRange(GL_UNIFORM_BUFFER,
                      static_cast<GLuint>(p_binding),
                      static_cast<GLuint>(p_buffer),
                      static_cast<GLintptr>(p_offset),
                      static_cast<GLsizeiptr>(p_bytes));
}

void readBuffer(NativeId p_buffer,
                std::size_t p_offset,
                std::size_t p_bytes,
                void* p_data)
{
    glBindBuffer(GL_COPY_READ_BUFFER, static_cast<GLuint>(p_buffer));
#if defined(COMPAGES_GLES3) && !defined(__EMSCRIPTEN__)
    // OpenGL ES 3.0 reads a buffer back only by mapping it.
    const void* mapped = glMapBufferRange(GL_COPY_READ_BUFFER,
                                          static_cast<GLintptr>(p_offset),
                                          static_cast<GLsizeiptr>(p_bytes),
                                          GL_MAP_READ_BIT);
    if (mapped != nullptr)
    {
        std::memcpy(p_data, mapped, p_bytes);
        glUnmapBuffer(GL_COPY_READ_BUFFER);
    }
#else
    // Desktop OpenGL, and WebGL 2 (getBufferSubData, through Emscripten).
    glGetBufferSubData(GL_COPY_READ_BUFFER,
                       static_cast<GLintptr>(p_offset),
                       static_cast<GLsizeiptr>(p_bytes),
                       p_data);
#endif
    glBindBuffer(GL_COPY_READ_BUFFER, 0u);
}

} // namespace compages::gpu::backend
