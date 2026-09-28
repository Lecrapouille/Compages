// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdint>

// ****************************************************************************
//! \file
//! \brief The vocabulary of the public API.
//!
//! Every one of these is an enum class of our own, never a value taken from a
//! graphics API. A caller writing compages::gpu::Primitive::Triangles does not learn that
//! it happens to be GL_TRIANGLES today, so the day another backend maps it to
//! something else, no caller changes. The translation lives in exactly one
//! switch per backend, where the compiler can check that no case is missing.
// ****************************************************************************

namespace compages::gpu
{

// ----------------------------------------------------------------------------
//! \brief What a buffer is going to be used for.
//!
//! The kind is fixed when the buffer is created because the driver wants to know
//! where to put the memory. It also lets the library refuse an obvious mistake,
//! such as drawing indexed geometry from a buffer holding uniforms.
// ----------------------------------------------------------------------------
enum class BufferKind
{
    //! \brief Per vertex data read by a vertex shader.
    Vertex,
    //! \brief Indices into a vertex buffer.
    Index,
    //! \brief A uniform block, read only, small, laid out in std140.
    Uniform,
    //! \brief A shader storage block: large, readable and writable by shaders.
    //! This is what a compute pass works on.
    Storage,
};

// ----------------------------------------------------------------------------
//! \brief How often the contents are going to change.
//!
//! This is a promise, not a restriction, and getting it wrong costs speed rather
//! than correctness. Immutable is worth reaching for: it lets the driver place
//! the data in the memory the GPU reads fastest, and it is what geometry loaded
//! once from a file should use.
// ----------------------------------------------------------------------------
enum class BufferUsage
{
    //! \brief Written once when created, never again. The fastest to read.
    Immutable,
    //! \brief Written from the CPU now and then, for instance a matrix that
    //! changes every frame.
    Dynamic,
    //! \brief Written by the GPU itself, by a compute pass or by transform
    //! feedback, and rarely read back.
    Storage,
};

// ----------------------------------------------------------------------------
//! \brief Size of one index in an index buffer.
//!
//! Sixteen bits is enough below 65536 vertices and halves the memory the GPU has
//! to read, which is why it is worth choosing rather than always using 32.
// ----------------------------------------------------------------------------
enum class IndexType
{
    //! \brief Unsigned 16 bit indices.
    UInt16,
    //! \brief Unsigned 32 bit indices.
    UInt32,
};

// ----------------------------------------------------------------------------
//! \brief What the vertices are supposed to draw.
// ----------------------------------------------------------------------------
enum class Primitive
{
    //! \brief One point per vertex. Used by particle systems, where the size is
    //! set by the vertex shader.
    Points,
    //! \brief One segment per pair of vertices.
    Lines,
    //! \brief A chain of segments, each vertex continuing from the previous one.
    LineStrip,
    //! \brief A chain of segments closing back on the first vertex.
    LineLoop,
    //! \brief One triangle per group of three vertices.
    Triangles,
    //! \brief A strip of triangles, each sharing the last two vertices.
    TriangleStrip,
    //! \brief A fan of triangles, all sharing the first vertex.
    TriangleFan,
};

// ----------------------------------------------------------------------------
//! \brief The stages of the graphics and compute pipelines.
// ----------------------------------------------------------------------------
enum class ShaderStage
{
    //! \brief Runs once per vertex.
    Vertex,
    //! \brief Runs once per fragment, that is roughly once per pixel covered.
    Fragment,
    //! \brief Runs once per primitive and may emit more of them.
    Geometry,
    //! \brief Decides how finely a patch is to be subdivided.
    TessellationControl,
    //! \brief Places the vertices the subdivision produced.
    TessellationEvaluation,
    //! \brief Runs outside any drawing, on a grid of work groups.
    Compute,
};

// ----------------------------------------------------------------------------
//! \brief How many stages there are, for sizing arrays indexed by stage.
// ----------------------------------------------------------------------------
constexpr std::size_t SHADER_STAGE_COUNT = 6u;

// ----------------------------------------------------------------------------
//! \brief Name of a stage, for error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr const char* toString(ShaderStage p_stage)
{
    switch (p_stage)
    {
        case ShaderStage::Vertex:
            return "vertex";
        case ShaderStage::Fragment:
            return "fragment";
        case ShaderStage::Geometry:
            return "geometry";
        case ShaderStage::TessellationControl:
            return "tessellation control";
        case ShaderStage::TessellationEvaluation:
            return "tessellation evaluation";
        case ShaderStage::Compute:
            return "compute";
    }
    return "unknown";
}

// ----------------------------------------------------------------------------
//! \brief Name of a buffer kind, for error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr const char* toString(BufferKind p_kind)
{
    switch (p_kind)
    {
        case BufferKind::Vertex:
            return "vertex";
        case BufferKind::Index:
            return "index";
        case BufferKind::Uniform:
            return "uniform";
        case BufferKind::Storage:
            return "storage";
    }
    return "unknown";
}

// ----------------------------------------------------------------------------
//! \brief How many vertices one primitive needs, or 0 for the strip and fan
//! kinds where the count does not work that way.
//!
//! Used to explain a draw call that asks for a vertex count the primitive cannot
//! use, such as four vertices as triangles.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr std::size_t verticesPerPrimitive(Primitive p_primitive)
{
    switch (p_primitive)
    {
        case Primitive::Points:
            return 1u;
        case Primitive::Lines:
            return 2u;
        case Primitive::Triangles:
            return 3u;
        case Primitive::LineStrip:
        case Primitive::LineLoop:
        case Primitive::TriangleStrip:
        case Primitive::TriangleFan:
            return 0u;
    }
    return 0u;
}

// ----------------------------------------------------------------------------
//! \brief Size in bytes of one index.
// ----------------------------------------------------------------------------
[[nodiscard]] constexpr std::size_t sizeOf(IndexType p_type)
{
    return (p_type == IndexType::UInt16) ? 2u : 4u;
}

// ----------------------------------------------------------------------------
//! \brief What a compute pass wrote that a later pass is about to read.
//!
//! The device is allowed to run ahead. A barrier is what says "the writes of
//! that kind are visible now", so a compute pass that fills a buffer and a draw
//! that reads it as vertices cannot race. Combine them with | .
// ----------------------------------------------------------------------------
enum class Barrier : std::uint32_t
{
    //! \brief Vertices about to be drawn, after a compute pass wrote them.
    VertexAttrib = 1u << 0,
    //! \brief Indices about to be drawn.
    Index = 1u << 1,
    //! \brief A uniform buffer about to be read.
    Uniform = 1u << 2,
    //! \brief A texture about to be sampled.
    TextureFetch = 1u << 3,
    //! \brief An image about to be read or written by a shader.
    Image = 1u << 4,
    //! \brief A storage buffer about to be read or written by a shader.
    Storage = 1u << 5,
    //! \brief A framebuffer about to be drawn into or read from.
    Framebuffer = 1u << 6,
    //! \brief A drawIndirect command about to be read, after a compute pass wrote
    //! how many objects survived.
    Command = 1u << 7,
    //! \brief Everything. When it is unclear which kind, this is the safe one.
    All = 0xFFFFFFFFu,
};

[[nodiscard]] constexpr Barrier operator|(Barrier p_left, Barrier p_right)
{
    return static_cast<Barrier>(static_cast<std::uint32_t>(p_left) |
                                static_cast<std::uint32_t>(p_right));
}

[[nodiscard]] constexpr Barrier operator&(Barrier p_left, Barrier p_right)
{
    return static_cast<Barrier>(static_cast<std::uint32_t>(p_left) &
                                static_cast<std::uint32_t>(p_right));
}

} // namespace compages::gpu
