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

#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/Errors.hpp"
#include "GPU/Internal/Pools.hpp"
#include "GPU/Internal/Statistics.hpp"
#include "Compages/GPU/RenderPass.hpp"

namespace compages::gpu
{

namespace
{

//------------------------------------------------------------------------------
//! \brief Everything a draw needs, once it is known to be possible.
//------------------------------------------------------------------------------
struct Ready
{
    detail::PipelineRecord const* pipeline = nullptr;
    backend::NativeId program = backend::NO_OBJECT;
};

//------------------------------------------------------------------------------
//! \brief The checks every draw shares.
//!
//! Three things can have gone wrong since the pipeline was created, and none of
//! them can be found out any earlier: the pipeline may have been released, its
//! program may have been released under it, and there may be no pass open to draw
//! into.
//------------------------------------------------------------------------------
Result<Ready> readyToDraw(Pipeline const& p_pipeline)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is no device "
                       "to draw on");
    }

    detail::PipelineRecord const* record =
        detail::pools().pipelines.get(p_pipeline.handle());
    if (record == nullptr)
    {
        return failure("this pipeline no longer exists, so there is nothing "
                       "saying how to draw");
    }

    detail::ProgramRecord const* program =
        detail::pools().programs.get(record->program);
    if (program == nullptr)
    {
        return failure("the program this pipeline draws with has been released. "
                       "A pipeline does not own its program, so the program has "
                       "to outlive it");
    }

    if (!inRenderPass())
    {
        return failure(
            "nothing is being drawn because no pass is open, so there is no "
            "target and no viewport. Open one with compages::gpu::RenderPass::begin() at "
            "the top of the frame");
    }

    return Ready{ record, program->native };
}

//------------------------------------------------------------------------------
//! \brief Does this many vertices make whole primitives?
//!
//! Four vertices drawn as triangles is one triangle and one vertex left over,
//! which the device silently ignores. The missing triangle is then blamed on the
//! shader.
//------------------------------------------------------------------------------
Status wholePrimitives(Primitive p_primitive, std::size_t p_count)
{
    const std::size_t per = verticesPerPrimitive(p_primitive);
    if (per == 0u)
    {
        // A strip or a fan makes as many primitives as it can from whatever it is
        // given, so no count is wrong. Too few to make one at all is still worth
        // saying.
        const std::size_t least =
            ((p_primitive == Primitive::TriangleStrip) ||
             (p_primitive == Primitive::TriangleFan))
                ? 3u
                : 2u;
        if (p_count < least)
        {
            return failure(std::to_string(p_count) +
                           " vertices are not enough to draw a " +
                           toString(p_primitive) + ", which needs at least " +
                           std::to_string(least));
        }
        return success();
    }

    if (p_count < per)
    {
        return failure(std::to_string(p_count) +
                       " vertices are not enough to draw one " +
                       toString(p_primitive) + ", which needs " +
                       std::to_string(per));
    }
    if ((p_count % per) != 0u)
    {
        return failure(
            std::to_string(p_count) + " vertices do not make whole " +
            toString(p_primitive) + ": " + std::to_string(per) +
            " are needed for each, so " + std::to_string(p_count % per) +
            " would be left over and quietly dropped");
    }
    return success();
}

//------------------------------------------------------------------------------
//! \brief Is this buffer one that can be read as vertices, and does it hold
//! enough of them?
//------------------------------------------------------------------------------
Status vertexSourceFits(detail::PipelineRecord const& p_pipeline,
                        BufferHandle p_vertices,
                        std::size_t p_count,
                        std::size_t p_first)
{
    detail::BufferRecord const* buffer =
        detail::pools().buffers.get(p_vertices);
    if (buffer == nullptr)
    {
        return failure("the buffer holding the vertices no longer exists. It was "
                       "released, or the Buffer object went out of scope while "
                       "something still drew from it");
    }
    if ((buffer->kind != BufferKind::Vertex) &&
        (buffer->kind != BufferKind::Storage))
    {
        return failure(std::string("this buffer was created to hold ") +
                       toString(buffer->kind) +
                       " data, not vertices. A vertex buffer, or a storage "
                       "buffer a compute pass just wrote, is what a draw reads");
    }

    const std::uint32_t stride = p_pipeline.layout.stride();
    if (stride == 0u)
    {
        return failure(
            "this pipeline was built for a shader that generates its own "
            "vertices, so there is nothing for a vertex buffer to feed. Use "
            "compages::gpu::drawWithoutVertices()");
    }

    const std::size_t held = buffer->bytes / stride;
    if (p_first + p_count > held)
    {
        return failure(
            "the draw asks for " + std::to_string(p_count) +
            " vertices starting at " + std::to_string(p_first) +
            ", but the buffer holds " + std::to_string(held) + " of " +
            std::to_string(stride) + " bytes each");
    }
    return success();
}

} // namespace

//------------------------------------------------------------------------------
void draw(Pipeline const& p_pipeline,
            BufferHandle p_vertices,
            std::size_t p_count,
            std::size_t p_first)
{
    auto ready_ = readyToDraw(p_pipeline);
    if (!ready_)
    {
        return reportError(ready_.error());
    }
    auto const ready = ready_.value();

    if (p_count == 0u)
    {
        return reportError("a draw of zero vertices is almost always a count that "
                       "was never computed. Skip the draw instead");
    }
    if (!check(wholePrimitives(ready.pipeline->state.primitive, p_count)))
    {
        return;
    }
    if (!check(vertexSourceFits(*ready.pipeline, p_vertices, p_count, p_first)))
    {
        return;
    }

    detail::BufferRecord const* buffer =
        detail::pools().buffers.get(p_vertices);
    backend::bindPipeline(
        ready.program, ready.pipeline->reader, ready.pipeline->state);
    backend::bindVertexBuffer(ready.pipeline->reader,
                              buffer->native,
                              ready.pipeline->layout.stride());
    backend::draw(ready.pipeline->state.primitive, p_first, p_count);

    detail::countDraw(p_count, 1u);
    return;
}

//------------------------------------------------------------------------------
void drawIndexed(Pipeline const& p_pipeline,
                   BufferHandle p_vertices,
                   BufferHandle p_indices,
                   IndexType p_type,
                   std::size_t p_count,
                   std::size_t p_first)
{
    auto ready_ = readyToDraw(p_pipeline);
    if (!ready_)
    {
        return reportError(ready_.error());
    }
    auto const ready = ready_.value();

    if (p_count == 0u)
    {
        return reportError("a draw of zero indices is almost always a count that was "
                       "never computed. Skip the draw instead");
    }
    if (!check(wholePrimitives(ready.pipeline->state.primitive, p_count)))
    {
        return;
    }

    detail::BufferRecord const* indices =
        detail::pools().buffers.get(p_indices);
    if (indices == nullptr)
    {
        return reportError("the buffer holding the indices no longer exists");
    }
    if (indices->kind != BufferKind::Index)
    {
        return reportError(
            std::string("the buffer given as indices was created to hold ") +
            toString(indices->kind) +
            " data. An index buffer has to be created with "
            "compages::gpu::BufferKind::Index, because the driver places it differently");
    }

    const std::size_t held = indices->bytes / sizeOf(p_type);
    if (p_first + p_count > held)
    {
        return reportError("the draw asks for " + std::to_string(p_count) +
                       " indices starting at " + std::to_string(p_first) +
                       ", but the buffer holds " + std::to_string(held) + " of " +
                       std::to_string(sizeOf(p_type)) + " bytes each");
    }

    // Every vertex the indices name has to exist, but checking that would mean
    // reading the indices back from the device on every draw. What can be checked
    // for free is that there is a vertex buffer at all and that it is the right
    // kind, so that is what is done.
    detail::BufferRecord const* vertices =
        detail::pools().buffers.get(p_vertices);
    if (vertices == nullptr)
    {
        return reportError("the buffer holding the vertices no longer exists");
    }
    if ((vertices->kind != BufferKind::Vertex) &&
        (vertices->kind != BufferKind::Storage))
    {
        return reportError(
            std::string("the buffer given as vertices was created to hold ") +
            toString(vertices->kind) +
            " data. A vertex buffer, or a storage buffer a compute pass just "
            "wrote, is what a draw reads");
    }
    if (ready.pipeline->layout.stride() == 0u)
    {
        return reportError(
            "this pipeline was built for a shader that generates its own "
            "vertices, so indices have nothing to name");
    }

    backend::bindPipeline(
        ready.program, ready.pipeline->reader, ready.pipeline->state);
    backend::bindVertexBuffer(ready.pipeline->reader,
                              vertices->native,
                              ready.pipeline->layout.stride());
    backend::bindIndexBuffer(ready.pipeline->reader, indices->native);
    backend::drawIndexed(
        ready.pipeline->state.primitive, p_type, p_first, p_count);

    detail::countDraw(p_count, 1u);
    return;
}

//------------------------------------------------------------------------------
void drawWithoutVertices(Pipeline const& p_pipeline, std::size_t p_count)
{
    auto ready_ = readyToDraw(p_pipeline);
    if (!ready_)
    {
        return reportError(ready_.error());
    }
    auto const ready = ready_.value();

    if (p_count == 0u)
    {
        return reportError("a draw of zero vertices is almost always a count that "
                       "was never computed. Skip the draw instead");
    }
    if (!check(wholePrimitives(ready.pipeline->state.primitive, p_count)))
    {
        return;
    }

    if (!ready.pipeline->attributes.empty())
    {
        return reportError(
            "this pipeline reads " +
            std::to_string(ready.pipeline->attributes.size()) +
            " vertex attributes, so there is nowhere for them to come from. "
            "Either pass a vertex buffer to compages::gpu::draw(), or build the pipeline "
            "with a default constructed compages::gpu::VertexLayout for a shader that "
            "makes its own vertices");
    }

    backend::bindPipeline(
        ready.program, ready.pipeline->reader, ready.pipeline->state);
    backend::draw(ready.pipeline->state.primitive, 0u, p_count);

    detail::countDraw(p_count, 1u);
    return;
}

//------------------------------------------------------------------------------
void drawInstanced(Pipeline const& p_pipeline,
                     BufferHandle p_vertices,
                     std::size_t p_vertex_count,
                     std::size_t p_instances,
                     std::size_t p_first)
{
    auto ready_ = readyToDraw(p_pipeline);
    if (!ready_)
    {
        return reportError(ready_.error());
    }
    auto const ready = ready_.value();

    if (p_instances == 0u)
    {
        return reportError(
            "a draw of zero instances draws nothing. Skip it instead");
    }
    if (p_vertex_count == 0u)
    {
        return reportError("a draw of zero vertices is almost always a count that "
                       "was never computed. Skip the draw instead");
    }
    if (!check(wholePrimitives(ready.pipeline->state.primitive, p_vertex_count)))
    {
        return;
    }

    if (p_pipeline.instanced())
    {
        // The buffer holds one record per object, not one per corner. Four
        // vertices and a hundred thousand instances is four corners generated
        // from gl_VertexID and a hundred thousand sprites in the buffer.
        if (!check(vertexSourceFits(*ready.pipeline, p_vertices, p_instances, 0u)))
        {
            return;
        }
    }
    else
    {
        if (!check(vertexSourceFits(
            *ready.pipeline, p_vertices, p_vertex_count, p_first)))
        {
            return;
        }
    }

    detail::BufferRecord const* buffer =
        detail::pools().buffers.get(p_vertices);
    backend::bindPipeline(
        ready.program, ready.pipeline->reader, ready.pipeline->state);
    backend::bindVertexBuffer(ready.pipeline->reader,
                              buffer->native,
                              ready.pipeline->layout.stride());
    backend::drawInstanced(ready.pipeline->state.primitive,
                           p_first,
                           p_vertex_count,
                           p_instances);

    detail::countDraw(p_vertex_count * p_instances, p_instances);
    return;
}

//------------------------------------------------------------------------------
void drawIndirect(Pipeline const& p_pipeline,
                    BufferHandle p_vertices,
                    BufferHandle p_commands)
{
    auto ready_ = readyToDraw(p_pipeline);
    if (!ready_)
    {
        return reportError(ready_.error());
    }
    auto const ready = ready_.value();

    if (!check(vertexSourceFits(*ready.pipeline, p_vertices, 1u, 0u)))
    {
        return;
    }

    detail::BufferRecord const* commands =
        detail::pools().buffers.get(p_commands);
    if (commands == nullptr)
    {
        return reportError("the buffer holding the draw command no longer exists");
    }
    if (commands->bytes < sizeof(DrawIndirectCommand))
    {
        return reportError(
            "a drawIndirect command is " +
            std::to_string(sizeof(DrawIndirectCommand)) +
            " bytes, and the buffer holds " +
            std::to_string(commands->bytes));
    }

    detail::BufferRecord const* buffer =
        detail::pools().buffers.get(p_vertices);
    backend::bindPipeline(
        ready.program, ready.pipeline->reader, ready.pipeline->state);
    backend::bindVertexBuffer(ready.pipeline->reader,
                              buffer->native,
                              ready.pipeline->layout.stride());
    backend::drawIndirect(ready.pipeline->state.primitive, commands->native);

    detail::countDraw(0u, 1u);
    return;
}

} // namespace compages::gpu
