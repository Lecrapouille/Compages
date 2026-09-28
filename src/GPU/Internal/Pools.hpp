// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "GPU/Backends/Backend.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Core/Pool.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/GPU/Texture.hpp"

#include <string>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Where the resources of the device actually live.
//!
//! Internal to the library: nothing outside src/GPU includes this. A caller holds
//! handles and facades, never a record.
// ****************************************************************************

namespace compages::gpu::detail
{

// ****************************************************************************
//! \brief What the library remembers about one block of device memory.
//!
//! The size and the usage are kept on this side rather than asked of the driver
//! every time, because they are what a write has to be checked against, and
//! because a check that costs a call into the driver is a check people turn off.
// ****************************************************************************
struct BufferRecord
{
    //! \brief What the backend calls this memory.
    backend::NativeId native = backend::NO_OBJECT;
    //! \brief How many bytes were reserved.
    std::size_t bytes = 0u;
    //! \brief What it is for.
    BufferKind kind = BufferKind::Vertex;
    //! \brief Whether it may be written again.
    BufferUsage usage = BufferUsage::Dynamic;
};

using BufferPool = Pool<BufferRecord, BufferTag>;

// ****************************************************************************
//! \brief What the library remembers about one compiled stage.
// ****************************************************************************
struct ShaderRecord
{
    //! \brief What the backend calls the compiled stage.
    backend::NativeId native = backend::NO_OBJECT;
    //! \brief Which stage it is.
    ShaderStage stage = ShaderStage::Vertex;
    //! \brief What to call it in messages, usually the file it came from.
    std::string name;
};

using ShaderPool = Pool<ShaderRecord, ShaderTag>;

// ****************************************************************************
//! \brief What the library remembers about one linked program.
//!
//! The reflection is read once, at link time, and lives here. Every later
//! question about what the program declares is answered from this rather than
//! from the driver, which matters because setting a uniform by name happens in
//! the middle of a frame.
// ****************************************************************************
struct ProgramRecord
{
    //! \brief What the backend calls the linked program.
    backend::NativeId native = backend::NO_OBJECT;
    //! \brief Everything the program declares.
    ProgramReflection reflection;
};

using ProgramPool = Pool<ProgramRecord, ProgramTag>;

// ****************************************************************************
//! \brief What the library remembers about one image on the device.
//!
//! The description is kept here rather than asked of the driver, because it is
//! what every write has to be checked against, and because the size and the format
//! cannot change once the memory is set aside.
// ****************************************************************************
struct TextureRecord
{
    //! \brief What the backend calls the image.
    backend::NativeId native = backend::NO_OBJECT;
    //! \brief What the texture is, with everything filled in.
    TextureDesc desc;
    //! \brief How many bytes it occupies, every level and face counted.
    std::size_t bytes = 0u;
};

using TexturePool = Pool<TextureRecord, TextureTag>;

// ****************************************************************************
//! \brief What the library remembers about one way of drawing.
//!
//! The result of checking a program against a vertex layout, kept whole. Nothing
//! here is asked again at draw time: the attributes were matched once, the state
//! was decided once, and the pipeline is immutable, so there is nothing left that
//! could have changed.
// ****************************************************************************
struct PipelineRecord
{
    //! \brief What the backend calls the way of reading a vertex. Shared with
    //! every other pipeline that reads a vertex the same way.
    backend::NativeId reader = backend::NO_OBJECT;
    //! \brief Which program this draws with. Named, not owned, so that it can be
    //! noticed when it goes away.
    ProgramHandle program;
    //! \brief What the backend calls that program. Kept alongside the handle so
    //! that binding does not have to look the program up in its pool.
    backend::NativeId native_program = backend::NO_OBJECT;
    //! \brief What one vertex looks like, as it was checked.
    VertexLayout layout;
    //! \brief How to draw.
    RenderState state;
    //! \brief The fields the shader actually reads, each with its slot. The
    //! result of the check, kept because it is also the answer to "why is my
    //! attribute not arriving".
    std::vector<backend::VertexAttribute> attributes;
};

using PipelinePool = Pool<PipelineRecord, PipelineTag>;

// ****************************************************************************
//! \brief What the library remembers about one target made of textures.
//!
//! The textures are named, not owned. The sizes are those of the attached level,
//! which is what a pass has to fit.
// ****************************************************************************
struct FramebufferRecord
{
    backend::NativeId native = backend::NO_OBJECT;
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::vector<Attachment> colors;
    Attachment depth;
};

using FramebufferPool = Pool<FramebufferRecord, FramebufferTag>;

// ****************************************************************************
//! \brief Every resource of the one device, each kind in its own pool.
// ****************************************************************************
struct Pools
{
    BufferPool buffers;
    ShaderPool shaders;
    ProgramPool programs;
    TexturePool textures;
    PipelinePool pipelines;
    FramebufferPool framebuffers;
};

// ----------------------------------------------------------------------------
//! \brief The pools of the process.
// ----------------------------------------------------------------------------
[[nodiscard]] Pools& pools();

// ----------------------------------------------------------------------------
//! \brief Release everything still alive, and say what had been forgotten.
//!
//! Called by compages::gpu::shutdown() while the context is still current. A resource still
//! alive here was leaked by the caller, so it is both freed and reported: that
//! report is what turns the examples gallery into a leak test, since closing a
//! demo must bring every count back to zero.
// ----------------------------------------------------------------------------
void releaseAllResources();

} // namespace compages::gpu::detail
