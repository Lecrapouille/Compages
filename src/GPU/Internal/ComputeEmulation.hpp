// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace compages::gpu::detail
{

// ----------------------------------------------------------------------------
//! \brief A storage block of a compute shader, as the emulation reads and
//! writes it.
// ----------------------------------------------------------------------------
struct EmulatedBlock
{
    //! \brief The name of the block (`buffer Particles { ... }`): what
    //! Program::bind() is given.
    std::string block;
    //! \brief The name of its array (`Particle particles[];`).
    std::string array;
    //! \brief Its `layout(binding = N)`, 0 when not given.
    int binding = 0;
    //! \brief Bytes from one element to the next (std430).
    std::uint32_t stride = 0u;
    //! \brief Does the shader write it?
    bool written = false;
    //! \brief The transform feedback outputs that hold one element, in memory
    //! order, padding included. Empty for a block only read.
    std::vector<std::string> outputs;
    //! \brief One field of an element, for the reflection: its name as the
    //! driver of a compute device reports it ("stars.position", or "values"
    //! for an element that is a number or a vector), its GLSL type, and its
    //! offset in bytes in the element.
    struct Field
    {
        std::string name;
        std::string type;
        std::uint32_t offset = 0u;
    };
    std::vector<Field> fields;
};

// ----------------------------------------------------------------------------
//! \brief A compute shader rewritten as a vertex shader whose outputs are
//! captured by transform feedback.
// ----------------------------------------------------------------------------
struct EmulatedCompute
{
    //! \brief The vertex shader, still in GLSL 4.50: GlslDialect lowers it.
    std::string text;
    //! \brief layout(local_size_x, _y, _z).
    std::array<int, 3> local_size{ 1, 1, 1 };
    //! \brief The storage blocks, in the order of the source.
    std::vector<EmulatedBlock> blocks;
    //! \brief Several blocks written: one output buffer each
    //! (GL_SEPARATE_ATTRIBS) instead of one interleaved record.
    bool separate = false;
};

// ----------------------------------------------------------------------------
//! \brief Rewrite a compute shader so that a device without compute runs it.
//!
//! Why: OpenGL 4.1 (macOS) and OpenGL ES 3.0 (WebGL 2) have no compute
//! shader and no storage block, but they have transform feedback: a vertex
//! shader whose outputs are written into a buffer, one record per vertex,
//! with the rasterizer off. The GL41 backend draws one point per invocation
//! with this vertex shader.
//!
//! How the source is rewritten:
//! - a storage block becomes a texture of 32 bit words, copied from its
//!   buffer before each dispatch: `particles[j]` reads the element j of that
//!   copy (as the buffer was when the dispatch started);
//! - a write `particles[i] = p;` or `particles[i].position += v;` goes to
//!   the record of the invocation, captured into the buffer;
//! - `gl_GlobalInvocationID` and its kin are computed from gl_VertexID;
//!   `particles.length()` becomes a uniform;
//! - `main` is renamed, so that an early `return` still writes the record;
//! - `COMPAGES_EMULATED_COMPUTE` is defined (`<compages/tiles.glsl>` uses it).
//!
//! \note What has no equivalent is refused with a sentence that says so:
//! `shared` and `barrier()` (use `<compages/tiles.glsl>`), atomics, images,
//! a block with more than one member, a struct holding an array or a matrix,
//! several written blocks when one of them holds structs, more than four
//! written blocks.
//!
//! \warning An invocation writes only the element of its own index (its
//! linear index in the dispatch, gl_GlobalInvocationID.x for a dispatch in
//! one dimension). The shader checks the index of each write: a write to
//! another element is lost, never put in the own one. A block written at
//! several different indices in the source is refused when it loads.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<EmulatedCompute> emulateCompute(std::string_view p_source);

// ----------------------------------------------------------------------------
//! \brief Rewrite a vertex or fragment shader that reads storage blocks, for
//! a device whose drawing shaders have none (OpenGL 4.1, OpenGL ES 3.0).
//!
//! The same rewriting as emulateCompute() for the reads: a block becomes a
//! texture holding a copy of its buffer, `values[j]` reads its element j,
//! `values.length()` is a uniform. `main` stays, nothing is captured.
//!
//! \note A write to a block is refused: a drawing shader writes a storage
//! block only with OpenGL 4.3 or OpenGL ES 3.1.
//!
//! \param[in] p_defined the macros the compiler defines (GL_ES on OpenGL ES),
//! for the `#if` of the source.
//! \return EmulatedCompute::blocks empty when the shader declares none.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<EmulatedCompute> emulateStorageReads(std::string_view p_source,
                                                          std::vector<std::string> const& p_defined);

//! \brief Does the text declare a storage block (`buffer`, outside comments
//! and preprocessor lines)? Cheap: says whether emulateStorageReads() is
//! needed.
[[nodiscard]] bool declaresStorageBlock(std::string_view p_source);

} // namespace compages::gpu::detail
