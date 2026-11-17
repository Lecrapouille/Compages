// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

namespace compages::gpu::backend
{

namespace
{

GLenum toGL(Primitive p_primitive)
{
    switch (p_primitive)
    {
        case Primitive::Points:
            return GL_POINTS;
        case Primitive::Lines:
            return GL_LINES;
        case Primitive::LineStrip:
            return GL_LINE_STRIP;
        case Primitive::LineLoop:
            return GL_LINE_LOOP;
        case Primitive::Triangles:
            return GL_TRIANGLES;
        case Primitive::TriangleStrip:
            return GL_TRIANGLE_STRIP;
        case Primitive::TriangleFan:
            return GL_TRIANGLE_FAN;
    }
    return GL_TRIANGLES;
}

GLenum toGL(IndexType p_type)
{
    return (p_type == IndexType::UInt16) ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
}

#if defined(COMPAGES_GLES3)
// ----------------------------------------------------------------------------
// Wireframe without glPolygonMode (OpenGL ES): the edges of the triangles are
// drawn as GL_LINES, from an index list made at each draw. Slow (an indexed
// draw reads its indices back), but only a wireframe view pays for it.
// ----------------------------------------------------------------------------
bool g_edges = false;
GLuint g_edge_buffer = 0u;
std::vector<std::uint32_t> g_edge_indices;

bool isTriangles(Primitive p_primitive)
{
    return (p_primitive == Primitive::Triangles) || (p_primitive == Primitive::TriangleStrip) ||
           (p_primitive == Primitive::TriangleFan);
}

//! \brief The three edges of each triangle, whose corners are vertices of
//! \c p_vertices taken in the order of the primitive.
void collectEdges(Primitive p_primitive, std::span<const std::uint32_t> p_vertices)
{
    g_edge_indices.clear();
    const auto add = [](std::uint32_t p_a, std::uint32_t p_b, std::uint32_t p_c)
    {
        g_edge_indices.insert(g_edge_indices.end(), { p_a, p_b, p_b, p_c, p_c, p_a });
    };
    const std::size_t n = p_vertices.size();
    if (p_primitive == Primitive::Triangles)
    {
        for (std::size_t k = 0u; k + 2u < n; k += 3u)
        {
            add(p_vertices[k], p_vertices[k + 1u], p_vertices[k + 2u]);
        }
    }
    else if (p_primitive == Primitive::TriangleStrip)
    {
        for (std::size_t k = 0u; k + 2u < n; ++k)
        {
            add(p_vertices[k], p_vertices[k + 1u], p_vertices[k + 2u]);
        }
    }
    else // TriangleFan
    {
        for (std::size_t k = 1u; k + 1u < n; ++k)
        {
            add(p_vertices[0u], p_vertices[k], p_vertices[k + 1u]);
        }
    }
}

//! \brief Draw the edges collected, with the vertex array bound. Its index
//! buffer is given back after.
void drawEdges(std::size_t p_instances)
{
    if (g_edge_indices.empty())
    {
        return;
    }
    if (g_edge_buffer == 0u)
    {
        glGenBuffers(1, &g_edge_buffer);
    }
    GLint previous = 0;
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &previous);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_edge_buffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(g_edge_indices.size() * sizeof(std::uint32_t)),
                 g_edge_indices.data(), GL_STREAM_DRAW);
    glDrawElementsInstanced(GL_LINES, static_cast<GLsizei>(g_edge_indices.size()), GL_UNSIGNED_INT,
                            nullptr, static_cast<GLsizei>(p_instances));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLuint>(previous));
}

//! \brief The edges of a draw of vertices read in order.
void drawEdgesInOrder(Primitive p_primitive, std::size_t p_first, std::size_t p_count, std::size_t p_instances)
{
    std::vector<std::uint32_t> vertices(p_count);
    for (std::size_t i = 0u; i < p_count; ++i)
    {
        vertices[i] = static_cast<std::uint32_t>(p_first + i);
    }
    collectEdges(p_primitive, vertices);
    drawEdges(p_instances);
}
#endif

} // namespace

namespace detail
{

void drawEdgesOnly(bool p_edges)
{
#if defined(COMPAGES_GLES3)
    g_edges = p_edges;
#else
    (void)p_edges;
#endif
}

void releaseEdgeIndices()
{
#if defined(COMPAGES_GLES3)
    if (g_edge_buffer != 0u)
    {
        glDeleteBuffers(1, &g_edge_buffer);
        g_edge_buffer = 0u;
    }
    g_edge_indices = {};
    g_edges = false;
#endif
}

} // namespace detail

void beginPass(PassDesc const& p_desc, NativeId p_target)
{
    useReversedDepth(p_desc.reversed_depth);
    glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(p_target));

    glViewport(static_cast<GLint>(p_desc.x),
               static_cast<GLint>(p_desc.y),
               static_cast<GLsizei>(p_desc.width),
               static_cast<GLsizei>(p_desc.height));
    // As in the GL45 backend: the scissor keeps the clear inside the pass.
    glScissor(static_cast<GLint>(p_desc.x),
              static_cast<GLint>(p_desc.y),
              static_cast<GLsizei>(p_desc.width),
              static_cast<GLsizei>(p_desc.height));
    glEnable(GL_SCISSOR_TEST);

    // The clear goes through the write masks: opened here, put back after.
    GLboolean color_mask[4] = { GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE };
    GLboolean depth_mask = GL_TRUE;
    if (p_desc.clear_color)
    {
        glGetBooleanv(GL_COLOR_WRITEMASK, color_mask);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    }
    if (p_desc.clear_depth)
    {
        glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask);
        glDepthMask(GL_TRUE);
    }

    GLbitfield what = 0u;
    if (p_desc.clear_color)
    {
        glClearColor(p_desc.color[0], p_desc.color[1], p_desc.color[2], p_desc.color[3]);
        what |= GL_COLOR_BUFFER_BIT;
    }
    if (p_desc.clear_depth)
    {
        const float depth = p_desc.reversed_depth ? (1.0f - p_desc.depth) : p_desc.depth;
        glClearDepthf(depth);
        what |= GL_DEPTH_BUFFER_BIT;
    }
    if (what != 0u)
    {
        glClear(what);
    }

    if (p_desc.clear_color)
    {
        glColorMask(color_mask[0], color_mask[1], color_mask[2], color_mask[3]);
    }
    if (p_desc.clear_depth)
    {
        glDepthMask(depth_mask);
    }
}

void endPass()
{
    glDisable(GL_SCISSOR_TEST);
}

void draw(Primitive p_primitive, std::size_t p_first, std::size_t p_count)
{
#if defined(COMPAGES_GLES3)
    if (g_edges && isTriangles(p_primitive))
    {
        drawEdgesInOrder(p_primitive, p_first, p_count, 1u);
        return;
    }
#endif
    glDrawArrays(toGL(p_primitive), static_cast<GLint>(p_first), static_cast<GLsizei>(p_count));
}

void drawInstanced(Primitive p_primitive,
                   std::size_t p_first,
                   std::size_t p_count,
                   std::size_t p_instances)
{
#if defined(COMPAGES_GLES3)
    if (g_edges && isTriangles(p_primitive))
    {
        drawEdgesInOrder(p_primitive, p_first, p_count, p_instances);
        return;
    }
#endif
    glDrawArraysInstanced(toGL(p_primitive),
                          static_cast<GLint>(p_first),
                          static_cast<GLsizei>(p_count),
                          static_cast<GLsizei>(p_instances));
}

void drawIndirect(Primitive p_primitive, NativeId p_commands)
{
#if defined(COMPAGES_GLES3)
    (void)p_primitive;
    (void)p_commands;
    reportError(std::string(name()) + " has no indirect draw (it needs OpenGL ES 3.1)");
#else
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, static_cast<GLuint>(p_commands));
    glDrawArraysIndirect(toGL(p_primitive), nullptr);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0u);
#endif
}

void drawIndexed(Primitive p_primitive,
                 IndexType p_type,
                 std::size_t p_first,
                 std::size_t p_count)
{
    const std::size_t offset = p_first * sizeOf(p_type);
#if defined(COMPAGES_GLES3)
    if (g_edges && isTriangles(p_primitive))
    {
        GLint indices = 0;
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &indices);
        std::vector<std::uint32_t> vertices(p_count);
        if (p_type == IndexType::UInt16)
        {
            std::vector<std::uint16_t> shorts(p_count);
            readBuffer(static_cast<NativeId>(indices), offset, p_count * 2u, shorts.data());
            for (std::size_t i = 0u; i < p_count; ++i)
            {
                vertices[i] = shorts[i];
            }
        }
        else
        {
            readBuffer(static_cast<NativeId>(indices), offset, p_count * 4u, vertices.data());
        }
        collectEdges(p_primitive, vertices);
        drawEdges(1u);
        return;
    }
#endif
    glDrawElements(toGL(p_primitive),
                   static_cast<GLsizei>(p_count),
                   toGL(p_type),
                   reinterpret_cast<const void*>(offset));
}

void waitForDevice()
{
    glFinish();
}

void readTargetPixels(std::uint32_t p_x,
                      std::uint32_t p_y,
                      std::uint32_t p_width,
                      std::uint32_t p_height,
                      void* p_pixels)
{
    GLint alignment = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(static_cast<GLint>(p_x),
                 static_cast<GLint>(p_y),
                 static_cast<GLsizei>(p_width),
                 static_cast<GLsizei>(p_height),
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 p_pixels);
    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
}

} // namespace compages::gpu::backend
