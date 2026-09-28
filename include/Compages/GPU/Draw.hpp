// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/RenderPass.hpp"

#include <type_traits>

// ****************************************************************************
//! \file
//! \brief Asking the device to draw.
//!
//! A draw call names a pipeline, which says how, and a buffer, which says what.
//! Nothing else: the state was decided when the pipeline was created, the vertex
//! layout was checked then too, and the target was decided when the pass was
//! opened. What is left is the count.
//!
//! \code
//! compages::gpu::draw(pipeline, triangle);
//! compages::gpu::drawIndexed(pipeline, cube_vertices, cube_indices);
//! \endcode
//!
//! Every one of them checks before it draws, and every check is one that used to
//! be a black screen: a buffer holding fewer vertices than were asked for, a
//! count of four vertices drawn as triangles, an index buffer passed where a
//! vertex buffer was meant, a pipeline whose program has been released, or a draw
//! outside any pass. A check that fails draws nothing and is recorded as the
//! frame error (see Errors.hpp): drawing happens every frame, so there is
//! nothing to return that a caller could sensibly act on.
//!
//! These are the building blocks. compages::gpu::Drawable calls them for you and is what
//! most code should use.
// ****************************************************************************

namespace compages::gpu
{

// ****************************************************************************
//! \brief Optional slice selected from a vertex or index buffer.
//!
//! A zero count means every element from \c first to the end.
// ****************************************************************************
struct DrawOptions
{
    std::size_t first = 0u;
    std::size_t count = 0u;
};

// ----------------------------------------------------------------------------
//! \brief Draw vertices read straight from a buffer.
//!
//! \param[in] p_pipeline how to draw, and what one vertex looks like.
//! \param[in] p_vertices the memory holding the vertices.
//! \param[in] p_count how many vertices to read.
//! \param[in] p_first which vertex to start at.
//! \return why nothing was drawn.
// ----------------------------------------------------------------------------
void draw(Pipeline const& p_pipeline,
                          BufferHandle p_vertices,
                          std::size_t p_count,
                          std::size_t p_first = 0u);

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
// ----------------------------------------------------------------------------
template <typename Vertex>
void draw(Pipeline const& p_pipeline,
                          Buffer<Vertex> const& p_vertices)
{
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) + " bytes, but the buffer holds "
            "elements of " + std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }
    return draw(p_pipeline, p_vertices.handle(), p_vertices.count());
}

template <typename Vertex>
void draw(Pipeline const& p_pipeline,
                          Buffer<Vertex>& p_vertices)
{
    if (!check(p_vertices.upload()))
    {
        return;
    }
    return draw(
        p_pipeline, static_cast<Buffer<Vertex> const&>(p_vertices));
}

// ----------------------------------------------------------------------------
//! \brief Explicit-pass form used by multi-pass code.
// ----------------------------------------------------------------------------
template <typename Vertex>
void draw(RenderPass const& p_pass,
                          Pipeline const& p_pipeline,
                          Buffer<Vertex> const& p_vertices,
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
        return reportError(
            "this pipeline was built for another vertex struct");
    }
    return draw(
        p_pipeline, p_vertices.handle(), count, p_options.first);
}

template <typename Vertex>
void draw(RenderPass const& p_pass,
                          Pipeline const& p_pipeline,
                          Buffer<Vertex>& p_vertices,
                          DrawOptions p_options = {})
{
    if (!check(p_vertices.upload()))
    {
        return;
    }
    return draw(p_pass,
                p_pipeline,
                static_cast<Buffer<Vertex> const&>(p_vertices),
                p_options);
}

// ----------------------------------------------------------------------------
//! \brief Draw vertices named by a buffer of indices.
//!
//! What a mesh of triangles wants. A corner shared by six triangles is stored
//! once and named six times, which is both less memory and less work for the
//! device, since it remembers what it computed for a vertex it has already seen.
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
// ----------------------------------------------------------------------------
template <typename Vertex, typename Index>
void drawIndexed(Pipeline const& p_pipeline,
                                 Buffer<Vertex> const& p_vertices,
                                 Buffer<Index> const& p_indices)
{
    static_assert(std::is_same_v<Index, std::uint16_t> ||
                      std::is_same_v<Index, std::uint32_t>,
                  "an index is a std::uint16_t or a std::uint32_t. A signed "
                  "index has no meaning, and anything wider is not something the "
                  "hardware reads");

    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) + " bytes, but the buffer holds "
            "elements of " + std::to_string(sizeof(Vertex)) +
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
//! \brief Explicit-pass indexed draw.
// ----------------------------------------------------------------------------
template <typename Vertex, typename Index>
void drawIndexed(RenderPass const& p_pass,
                                 Pipeline const& p_pipeline,
                                 Buffer<Vertex> const& p_vertices,
                                 Buffer<Index> const& p_indices,
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
        return reportError(
            "this pipeline was built for another vertex struct");
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
// ----------------------------------------------------------------------------
template <typename Vertex, typename Index>
void drawIndexed(Pipeline const& p_pipeline,
                                 Buffer<Vertex>& p_vertices,
                                 Buffer<Index> const& p_indices)
{
    static_assert(std::is_same_v<Index, std::uint16_t> ||
                      std::is_same_v<Index, std::uint32_t>,
                  "an index is a std::uint16_t or a std::uint32_t. A signed "
                  "index has no meaning, and anything wider is not something the "
                  "hardware reads");

    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) + " bytes, but the buffer holds "
            "elements of " + std::to_string(sizeof(Vertex)) +
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
void drawWithoutVertices(Pipeline const& p_pipeline,
                                         std::size_t p_count);

// ****************************************************************************
//! \brief What a drawIndirect command buffer holds, one draw per element.
//!
//! The four words the driver reads, in this order. A compute pass that decides
//! what to draw writes these; the CPU never has to ask how many survived.
// ****************************************************************************
struct DrawIndirectCommand
{
    std::uint32_t vertex_count = 0u;
    std::uint32_t instance_count = 1u;
    std::uint32_t first_vertex = 0u;
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
//! \param[in] p_instances how many objects. Zero is refused.
// ----------------------------------------------------------------------------
void drawInstanced(Pipeline const& p_pipeline,
                                   BufferHandle p_vertices,
                                   std::size_t p_vertex_count,
                                   std::size_t p_instances,
                                   std::size_t p_first = 0u);

template <typename Vertex>
void drawInstanced(Pipeline const& p_pipeline,
                                   Buffer<Vertex> const& p_vertices,
                                   std::size_t p_vertex_count,
                                   std::size_t p_instances)
{
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) + " bytes, but the buffer holds "
            "elements of " + std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }
    return drawInstanced(p_pipeline,
                         p_vertices.handle(),
                         p_vertex_count,
                         p_instances);
}

// ----------------------------------------------------------------------------
//! \brief Draw as a command sitting on the device says.
//!
//! The CPU does not ask how many objects survived a cull: the compute pass
//! wrote the command, and this reads it. The buffer holds one
//! DrawIndirectCommand.
// ----------------------------------------------------------------------------
void drawIndirect(Pipeline const& p_pipeline,
                                  BufferHandle p_vertices,
                                  BufferHandle p_commands);

template <typename Vertex>
void drawIndirect(Pipeline const& p_pipeline,
                                  Buffer<Vertex> const& p_vertices,
                                  Buffer<DrawIndirectCommand> const& p_commands)
{
    if (p_pipeline.stride() != sizeof(Vertex))
    {
        return reportError(
            "this pipeline reads a vertex of " +
            std::to_string(p_pipeline.stride()) + " bytes, but the buffer holds "
            "elements of " + std::to_string(sizeof(Vertex)) +
            " bytes. The pipeline was built for another vertex struct");
    }
    return drawIndirect(p_pipeline, p_vertices.handle(), p_commands.handle());
}

} // namespace compages::gpu
