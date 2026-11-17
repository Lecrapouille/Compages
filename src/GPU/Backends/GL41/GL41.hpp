// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The backend for OpenGL 4.1 core (macOS) and, compiled with
//! COMPAGES_GLES3, for OpenGL ES 3.0 (WebGL 2 through Emscripten).
//!
//! Both lack Direct State Access (4.5), separate vertex formats (4.3),
//! compute, storage blocks and images (4.3, ES 3.1) and `layout(binding)`
//! in GLSL (4.20). So this backend:
//!
//! - binds an object to configure it, on a target that touches no state the
//!   rest of the backend relies on (GL_COPY_WRITE_BUFFER for buffers, the
//!   unit GL_TEXTURE0 + SCRATCH_UNIT for textures);
//! - keeps the attributes of each vertex array and points them at the
//!   buffer at bindVertexBuffer() (glVertexAttribPointer);
//! - stores the target of a texture in the high bits of its NativeId,
//!   because glBindTexture needs it and the contract gives only the id;
//! - translates GLSL 4.50 (GPU/Internal/GlslDialect.hpp) and applies the
//!   bindings it removed after linking;
//! - refuses compute, storage and images with a sentence saying why.
//!
//! The differences between 4.1 and ES 3.0 are `#if` blocks in place.
//! Nothing outside src/GPU/Backends/GL41 includes this file.
// ****************************************************************************

#if defined(COMPAGES_GLES3)
#    include <GLES3/gl3.h>
#    if defined(__EMSCRIPTEN__)
#        include <webgl/webgl2.h>
#    endif
#else
#    include <glad/gl.h>
#endif

#include "Compages/GPU/Errors.hpp"
#include "GPU/Backends/Backend.hpp"
#include "GPU/Internal/ComputeEmulation.hpp"

#include <string>
#include <vector>

namespace compages::gpu::backend
{

#if defined(COMPAGES_GLES3)
constexpr int MINIMUM_VERSION_MAJOR = 3;
constexpr int MINIMUM_VERSION_MINOR = 0;
#else
constexpr int MINIMUM_VERSION_MAJOR = 4;
constexpr int MINIMUM_VERSION_MINOR = 1;
#endif

namespace detail
{

//! \brief The texture unit used to configure a texture. The last one the
//! OpenGL ES 3.0 minimum guarantees (32 combined units), so that a draw
//! never finds a texture of ours left where it expects its own.
constexpr GLuint SCRATCH_UNIT = 31u;

//! \brief The GL name of a texture (low 32 bits of its NativeId).
inline GLuint textureName(NativeId p_texture)
{
    return static_cast<GLuint>(p_texture & 0xFFFFFFFFu);
}

//! \brief The target the texture was made for (high 32 bits).
inline GLenum textureTarget(NativeId p_texture)
{
    return static_cast<GLenum>(p_texture >> 32u);
}

//! \brief A NativeId holding both.
inline NativeId textureId(GLuint p_name, GLenum p_target)
{
    return (static_cast<NativeId>(p_target) << 32u) | static_cast<NativeId>(p_name);
}

//! \brief glUseProgram, skipped when the program is already in use. Shared
//! by the pipeline (drawing) and the shader (uniforms) code.
void useProgram(GLuint p_program);

//! \brief glBindVertexArray, skipped when already bound.
void bindVertexArray(GLuint p_vao);

//! \brief Drop the vertex arrays held for sharing (shutdown).
void clearVertexArrayCache();

//! \brief A buffer is being deleted: its name may come back for another
//! one, so no vertex array may believe it still reads it.
void forgetBuffer(GLuint p_buffer);

//! \brief Log a message once per process: what this device cannot do,
//! said the first time it is asked rather than every frame.
void warnOnce(char const* p_what);

//! \brief Delete what reading textures back keeps (shutdown).
void releaseTextureReadback();

//! \brief OpenGL ES has no glPolygonMode: set by the pipeline when it asks
//! for lines, so that the draws give the edges of their triangles instead
//! (GL41Draw.cpp). Never set on desktop OpenGL.
void drawEdgesOnly(bool p_edges);

//! \brief Delete the index buffer of the edges (shutdown).
void releaseEdgeIndices();

//! \brief The program in use (detail::useProgram()).
GLuint usedProgram();

//! \brief Compile one stage from GLSL 4.50, translated to the dialect of the
//! backend. Keeps the bindings it removed, for linkProgram().
//! \return the shader, or 0 with the compiler log in \c p_log.
GLuint compileGlsl(GLenum p_stage, std::string_view p_source, std::string& p_log);

// ----------------------------------------------------------------------------
// Storage blocks and compute, emulated (GL41Compute.cpp).
// ----------------------------------------------------------------------------

//! \brief Width in words of the textures that hold a copy of a storage
//! buffer. Set by init().
void setDataTextureWidth(GLsizei p_width);

//! \brief Compile a compute shader as a vertex shader for transform
//! feedback (and the empty fragment shader OpenGL ES wants beside it).
Result<NativeId> compileCompute(std::string_view p_source);

//! \brief The storage blocks a vertex or fragment shader reads (rewritten
//! by emulateStorageReads()), for the programs it goes into.
void rememberStorageReads(GLuint p_shader, std::vector<gpu::detail::EmulatedBlock> p_blocks);

//! \brief Forget what a shader reads; delete it if it is an emulated compute
//! shader. \return true when it was one (deleted).
bool releaseCompute(GLuint p_shader);

//! \brief Before linking: when \c p_shaders holds an emulated compute shader,
//! attach its fragment shader and say which outputs are captured.
//! \return true when it does.
bool prepareComputeLink(GLuint p_program, std::span<const NativeId> p_shaders);

//! \brief After linking: detach what prepareComputeLink() attached and, when
//! the link worked, keep what a dispatch or a draw needs to read its blocks.
void finishStorageLink(GLuint p_program, std::span<const NativeId> p_shaders, bool p_linked);

//! \brief A program is deleted: drop what it kept.
void forgetStorageProgram(GLuint p_program);

//! \brief Complete the reflection: storage blocks, work group size, the
//! internal uniforms hidden.
void reflectStorage(GLuint p_program, ProgramReflection& p_reflection);

//! \brief Remember which buffer is on a storage binding point.
void bindStoragePoint(GLuint p_buffer, int p_binding, std::size_t p_offset, std::size_t p_bytes);

//! \brief The content of a buffer changed: its copy is out of date.
void touchBuffer(GLuint p_buffer);

//! \brief A buffer is deleted: drop its copy and its binding points.
void dropStorageCopy(GLuint p_buffer);

//! \brief Before a draw: bring the copies the program reads up to date and
//! bind them. Nothing for a program that reads no storage block.
void prepareStorageReads(GLuint p_program);

//! \brief Delete every copy (shutdown).
void releaseStorageCopies();

} // namespace detail

} // namespace compages::gpu::backend
