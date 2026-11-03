// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Ask the graphics card to draw: the low-level calls under Drawable.
//!
//! Drawing needs two things: how to draw (a Pipeline: the shader and its
//! settings) and what to draw (a Buffer of vertices). The calls here take both
//! and draw. Everything else was decided before: the settings when the pipeline
//! was created, the place to draw when the pass was opened.
//!
//! \code
//! compages::gpu::draw(pipeline, triangle);
//! compages::gpu::drawIndexed(pipeline, cube_vertices, cube_indices);
//! \endcode
//!
//! Every call checks before it draws, and every check is one that used to be a
//! black screen: a buffer holding fewer vertices than were asked for, four
//! vertices drawn as triangles, an index buffer passed where a vertex buffer
//! was meant, a released program, a draw outside any pass. A failed check draws
//! nothing and is recorded as the frame error (see Errors.hpp): drawing happens
//! every frame, so there is nothing to return that a caller could act on.
//!
//! What it holds:
//! - draw(), drawIndexed(): the common draws, with or without indices.
//! - drawInstanced(): one shape many times, in one call.
//! - drawIndirect() and DrawIndirectCommand: a draw decided by the card.
//! - drawWithoutVertices(): full screen effects that need no buffer.
//! - DrawOptions: draw only a slice of a buffer.
//!
//! compages::gpu::Drawable calls these for you and is what most code should
//! use.
// ****************************************************************************

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/RenderPass.hpp"

#include <type_traits>

namespace compages::gpu
{

// ****************************************************************************
//! \brief Which part of a buffer to draw, when not all of it.
//!
//! Like choosing a page range when printing: by default everything is drawn,
//! and a range draws only a slice, for instance one object out of many stored
//! in the same buffer.
//!
//! \code
//! compages::gpu::draw(pass, pipeline, shapes, { .first = 36u, .count = 36u });
//! \endcode
// ****************************************************************************
struct DrawOptions
{
    //! \brief Index of the first vertex, or index, to draw.
    std::size_t first = 0u;
    //! \brief How many to draw. Zero means every element from first to the end.
    std::size_t count = 0u;
};

// ----------------------------------------------------------------------------
//! \brief Draw vertices read straight from a buffer.
//!
//! \param[in] p_pipeline how to draw, and what one vertex looks like.
//! \param[in] p_vertices the memory holding the vertices.
//! \param[in] p_count how many vertices to read.
//! \param[in] p_first which vertex to start at.
//!
//! A failed check is recorded as the frame error and draws nothing.
// ----------------------------------------------------------------------------
void draw(Pipeline const& p_pipeline,
          BufferHandle p_vertices,
          std::size_t p_count,
          std::size_t p_first = 0u);

// ----------------------------------------------------------------------------
//! \brief Same as draw(Pipeline const&, BufferHandle, std::size_t,
//! std::size_t), checking first that p_pass is still open.
//!
//! \param[in] p_pass the pass the draw goes into. A closed pass is refused,
//! which catches a draw left after the pass was ended.
//! \param[in] p_pipeline how to draw, and what one vertex looks like.
//! \param[in] p_vertices the memory holding the vertices.
//! \param[in] p_count how many vertices to read.
//! \param[in] p_first which vertex to start at.
// ----------------------------------------------------------------------------
inline void draw(RenderPass const& p_pass,
                 Pipeline const& p_pipeline,
                 BufferHandle p_vertices,
                 std::size_t p_count,
                 std::size_t p_first = 0u)
{
    if (!p_pass.open())
    {
        return reportError("cannot draw through a closed render pass");
    }
    return draw(p_pipeline, p_vertices, p_count, p_first);
}

// ----------------------------------------------------------------------------
//! \brief Draw the whole of a buffer.
//!
//! The overload almost every caller wants: the count comes from the buffer, so
//! there is no number to keep in step with it.
//!
//! \tparam Vertex what one element of the buffer is. Checked against the vertex
//! the pipeline was built for, since drawing a buffer of one struct with the
//! layout of another reads the right number of bytes from the wrong places.
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices every vertex is drawn. Const, so changes waiting on
//! the CPU are not sent: use the non const overload for that.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind>
    requires VertexSource<VertexKind>
void draw(Pipeline const& p_pipeline,
          Buffer<Vertex, VertexKind> const& p_vertices)
{
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) +
            " bytes, but the buffer holds "
            "elements of " +
            std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }
    return draw(p_pipeline, p_vertices.handle(), p_vertices.count());
}

// ----------------------------------------------------------------------------
//! \brief Draw the whole of a buffer, sending its CPU changes first.
//!
//! \code
//! trail.emplace_back(position, color);
//! compages::gpu::draw(pipeline, trail);   // uploads, then draws
//! \endcode
//!
//! \param[in] p_pipeline how to draw.
//! \param[in,out] p_vertices uploaded (see Buffer::upload()), then drawn.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind>
    requires VertexSource<VertexKind>
void draw(Pipeline const& p_pipeline, Buffer<Vertex, VertexKind>& p_vertices)
{
    if (!check(p_vertices.upload()))
    {
        return;
    }
    return draw(p_pipeline,
                static_cast<Buffer<Vertex, VertexKind> const&>(p_vertices));
}

// ----------------------------------------------------------------------------
//! \brief Draw a buffer, or a slice of it, into a given pass.
//!
//! The form multi-pass code uses: naming the pass makes it visible which
//! target each draw goes to.
//!
//! \param[in] p_pass the pass the draw goes into. A closed pass is refused,
//! which catches a draw left after the pass was ended.
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the vertices. Const: CPU changes are not sent.
//! \param[in] p_options which slice; all of it by default.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind>
    requires VertexSource<VertexKind>
void draw(RenderPass const& p_pass,
          Pipeline const& p_pipeline,
          Buffer<Vertex, VertexKind> const& p_vertices,
          DrawOptions p_options = {})
{
    if (!p_pass.open())
    {
        return reportError("cannot draw through a closed render pass");
    }
    if (p_options.first > p_vertices.count())
    {
        return reportError("the first vertex is outside the buffer");
    }
    const std::size_t available = p_vertices.count() - p_options.first;
    const std::size_t count =
        (p_options.count == 0u) ? available : p_options.count;
    if (count > available)
    {
        return reportError("the requested vertex range exceeds the buffer");
    }
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError("this pipeline was built for another vertex struct");
    }
    return draw(p_pipeline, p_vertices.handle(), count, p_options.first);
}

// ----------------------------------------------------------------------------
//! \brief Same as the const overload, sending the CPU changes of p_vertices
//! first.
//!
//! \param[in] p_pass the pass the draw goes into. A closed pass is refused,
//! which catches a draw left after the pass was ended.
//! \param[in] p_pipeline how to draw.
//! \param[in,out] p_vertices uploaded, then drawn.
//! \param[in] p_options which slice; all of it by default.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind>
    requires VertexSource<VertexKind>
void draw(RenderPass const& p_pass,
          Pipeline const& p_pipeline,
          Buffer<Vertex, VertexKind>& p_vertices,
          DrawOptions p_options = {})
{
    if (!check(p_vertices.upload()))
    {
        return;
    }
    return draw(p_pass,
                p_pipeline,
                static_cast<Buffer<Vertex, VertexKind> const&>(p_vertices),
                p_options);
}

// ----------------------------------------------------------------------------
//! \brief Draw vertices named by a buffer of indices.
//!
//! What a mesh of triangles wants. A corner shared by six triangles is stored
//! once and named six times, which is both less memory and less work for the
//! device, since it remembers what it computed for a vertex it has already
//! seen.
//!
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the memory holding the vertices.
//! \param[in] p_indices the memory holding which vertices, in which order.
//! \param[in] p_type the size of one index.
//! \param[in] p_count how many indices to read.
//! \param[in] p_first which index to start at.
// ----------------------------------------------------------------------------
void drawIndexed(Pipeline const& p_pipeline,
                 BufferHandle p_vertices,
                 BufferHandle p_indices,
                 IndexType p_type,
                 std::size_t p_count,
                 std::size_t p_first = 0u);

// ----------------------------------------------------------------------------
//! \brief Same as drawIndexed() on handles, checking first that p_pass is
//! still open.
//!
//! \param[in] p_pass the pass the draw goes into. A closed pass is refused,
//! which catches a draw left after the pass was ended.
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the memory holding the vertices.
//! \param[in] p_indices the memory holding which vertices, in which order.
//! \param[in] p_type the size of one index.
//! \param[in] p_count how many indices to read.
//! \param[in] p_first which index to start at.
// ----------------------------------------------------------------------------
inline void drawIndexed(RenderPass const& p_pass,
                        Pipeline const& p_pipeline,
                        BufferHandle p_vertices,
                        BufferHandle p_indices,
                        IndexType p_type,
                        std::size_t p_count,
                        std::size_t p_first = 0u)
{
    if (!p_pass.open())
    {
        return reportError("cannot draw through a closed render pass");
    }
    return drawIndexed(
        p_pipeline, p_vertices, p_indices, p_type, p_count, p_first);
}

// ----------------------------------------------------------------------------
//! \brief Draw a whole indexed mesh, taking the index size from the type.
//!
//! \tparam Index std::uint16_t or std::uint32_t. Sixteen bits is enough below
//! 65536 vertices and halves what the device has to read.
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the vertices. Const: CPU changes are not sent.
//! \param[in] p_indices every index is drawn.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind, typename Index>
    requires VertexSource<VertexKind>
void drawIndexed(Pipeline const& p_pipeline,
                 Buffer<Vertex, VertexKind> const& p_vertices,
                 IndexBuffer<Index> const& p_indices)
{
    static_assert(
        std::is_same_v<Index, std::uint16_t> ||
            std::is_same_v<Index, std::uint32_t>,
        "an index is a std::uint16_t or a std::uint32_t. A signed "
        "index has no meaning, and anything wider is not something the "
        "hardware reads");

    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) +
            " bytes, but the buffer holds "
            "elements of " +
            std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }

    constexpr IndexType type = std::is_same_v<Index, std::uint16_t>
                                   ? IndexType::UInt16
                                   : IndexType::UInt32;
    return drawIndexed(p_pipeline,
                       p_vertices.handle(),
                       p_indices.handle(),
                       type,
                       p_indices.count());
}

// ----------------------------------------------------------------------------
//! \brief Draw an indexed mesh, or a slice of its indices, into a given
//! pass.
//!
//! \param[in] p_pass the pass the draw goes into. A closed pass is refused,
//! which catches a draw left after the pass was ended.
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the vertices. Const: CPU changes are not sent.
//! \param[in] p_indices the indices, std::uint16_t or std::uint32_t.
//! \param[in] p_options which slice of the indices; all of them by default.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind, typename Index>
    requires VertexSource<VertexKind>
void drawIndexed(RenderPass const& p_pass,
                 Pipeline const& p_pipeline,
                 Buffer<Vertex, VertexKind> const& p_vertices,
                 IndexBuffer<Index> const& p_indices,
                 DrawOptions p_options = {})
{
    static_assert(std::is_same_v<Index, std::uint16_t> ||
                      std::is_same_v<Index, std::uint32_t>,
                  "an index must be std::uint16_t or std::uint32_t");
    if (!p_pass.open())
    {
        return reportError("cannot draw through a closed render pass");
    }
    if (p_options.first > p_indices.count())
    {
        return reportError("the first index is outside the buffer");
    }
    const std::size_t available = p_indices.count() - p_options.first;
    const std::size_t count =
        (p_options.count == 0u) ? available : p_options.count;
    if (count > available)
    {
        return reportError("the requested index range exceeds the buffer");
    }
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError("this pipeline was built for another vertex struct");
    }
    constexpr IndexType type = std::is_same_v<Index, std::uint16_t>
                                   ? IndexType::UInt16
                                   : IndexType::UInt32;
    return drawIndexed(p_pipeline,
                       p_vertices.handle(),
                       p_indices.handle(),
                       type,
                       count,
                       p_options.first);
}

// ----------------------------------------------------------------------------
//! \brief Draw an indexed mesh whose vertices are kept on the CPU.
//!
//! Sends whatever changed, then draws. The indices are a Buffer because they
//! do not move: a surface that waves still names the same triangles.
//!
//! \param[in] p_pipeline how to draw.
//! \param[in,out] p_vertices uploaded, then drawn.
//! \param[in] p_indices every index is drawn.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind, typename Index>
    requires VertexSource<VertexKind>
void drawIndexed(Pipeline const& p_pipeline,
                 Buffer<Vertex, VertexKind>& p_vertices,
                 IndexBuffer<Index> const& p_indices)
{
    static_assert(
        std::is_same_v<Index, std::uint16_t> ||
            std::is_same_v<Index, std::uint32_t>,
        "an index is a std::uint16_t or a std::uint32_t. A signed "
        "index has no meaning, and anything wider is not something the "
        "hardware reads");

    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) +
            " bytes, but the buffer holds "
            "elements of " +
            std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }

    if (!check(p_vertices.upload()))
    {
        return;
    }

    constexpr IndexType type = std::is_same_v<Index, std::uint16_t>
                                   ? IndexType::UInt16
                                   : IndexType::UInt32;
    return drawIndexed(p_pipeline,
                       p_vertices.handle(),
                       p_indices.handle(),
                       type,
                       p_indices.count());
}

// ----------------------------------------------------------------------------
//! \brief Draw without any vertex data at all.
//!
//! The vertex shader builds the positions from the index of the vertex it is
//! running for. Two triangles covering the whole target need no buffer, no
//! layout and no upload, which is what every full screen effect uses: the
//! Mandelbrot set, a blur, a tone mapping pass.
//!
//! \param[in] p_pipeline a pipeline built with a default constructed
//! VertexLayout.
//! \param[in] p_count how many vertices to run the shader for. Three for a
//! triangle covering the target, four with a triangle strip for a quad.
// ----------------------------------------------------------------------------
void drawWithoutVertices(Pipeline const& p_pipeline, std::size_t p_count);

// ****************************************************************************
//! \brief A draw order written on the graphics card itself, so the card
//! decides how much to draw without asking the CPU.
//!
//! A compute pass that hides what the camera cannot see writes how many objects
//! are left into one of these, and drawIndirect() reads it. The four words are
//! the ones the driver reads, in this order.
//!
//! \code
//! auto commands =
//!     compages::gpu::StorageBuffer<compages::gpu::DrawIndirectCommand>::create(
//!         1u, compages::gpu::BufferUsage::Static).take();
//! // a compute pass bound to commands writes the counts, then:
//! compages::gpu::barrier(compages::gpu::Barrier::Command);
//! compages::gpu::drawIndirect(pipeline, quad, commands);
//! \endcode
// ****************************************************************************
struct DrawIndirectCommand
{
    //! \brief How many vertices one object has.
    std::uint32_t vertex_count = 0u;
    //! \brief How many objects to draw. What a culling pass usually writes.
    std::uint32_t instance_count = 1u;
    //! \brief Index of the first vertex in the vertex buffer.
    std::uint32_t first_vertex = 0u;
    //! \brief Number of the first object: per instance attributes start
    //! reading there. gl_InstanceID still counts from 0.
    std::uint32_t first_instance = 0u;
};

static_assert(sizeof(DrawIndirectCommand) == 16u,
              "the driver reads four unsigned integers and nothing else");
static_assert(std::is_standard_layout_v<DrawIndirectCommand>,
              "a drawIndirect command is sent as the bytes it already is");

// ----------------------------------------------------------------------------
//! \brief Draw the same vertices several times, once per instance.
//!
//! The corners of a sprite, or of a mesh, are described once. What changes per
//! object lives in a second buffer whose fields were marked perInstance(). One
//! draw call is what a hundred thousand sprites cost, which is the point.
//!
//! \param[in] p_pipeline a pipeline whose layout has per instance fields.
//! \param[in] p_vertices the corners of one object.
//! \param[in] p_vertex_count how many corners one object has.
//! \param[in] p_instances how many objects. Zero is refused.
//! \param[in] p_first which vertex to start at.
// ----------------------------------------------------------------------------
void drawInstanced(Pipeline const& p_pipeline,
                   BufferHandle p_vertices,
                   std::size_t p_vertex_count,
                   std::size_t p_instances,
                   std::size_t p_first = 0u);

// ----------------------------------------------------------------------------
//! \brief Same as drawInstanced() on a handle, checking first that the
//! buffer holds the vertex struct the pipeline was built for.
//!
//! \param[in] p_pipeline a pipeline whose layout has per instance fields.
//! \param[in] p_vertices the corners of one object. Const: CPU changes are
//! not sent.
//! \param[in] p_vertex_count how many corners one object has.
//! \param[in] p_instances how many objects. Zero is refused.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind>
    requires VertexSource<VertexKind>
void drawInstanced(Pipeline const& p_pipeline,
                   Buffer<Vertex, VertexKind> const& p_vertices,
                   std::size_t p_vertex_count,
                   std::size_t p_instances)
{
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) +
            " bytes, but the buffer holds "
            "elements of " +
            std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }
    return drawInstanced(
        p_pipeline, p_vertices.handle(), p_vertex_count, p_instances);
}

// ----------------------------------------------------------------------------
//! \brief Draw as a command sitting on the device says.
//!
//! The CPU does not ask how many objects survived a cull: the compute pass
//! wrote the command, and this reads it. The buffer holds one
//! DrawIndirectCommand. A barrier(Barrier::Command) must come between the
//! compute pass and this call.
//!
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the memory holding the vertices.
//! \param[in] p_commands the memory holding the command.
// ----------------------------------------------------------------------------
void drawIndirect(Pipeline const& p_pipeline,
                  BufferHandle p_vertices,
                  BufferHandle p_commands);

// ----------------------------------------------------------------------------
//! \brief Same as drawIndirect() on handles, checking first that the buffer
//! holds the vertex struct the pipeline was built for.
//!
//! \param[in] p_pipeline how to draw.
//! \param[in] p_vertices the vertices. Const: CPU changes are not sent.
//! \param[in] p_commands a buffer holding one DrawIndirectCommand.
// ----------------------------------------------------------------------------
template <typename Vertex, BufferKind VertexKind>
    requires VertexSource<VertexKind>
void drawIndirect(Pipeline const& p_pipeline,
                  Buffer<Vertex, VertexKind> const& p_vertices,
                  StorageBuffer<DrawIndirectCommand> const& p_commands)
{
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) +
            " bytes, but the buffer holds "
            "elements of " +
            std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }
    return drawIndirect(p_pipeline, p_vertices.handle(), p_commands.handle());
}

} // namespace compages::gpu
