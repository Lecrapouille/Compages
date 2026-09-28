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

#pragma once

#include "Compages/GPU/Core/Enums.hpp"
#include "Compages/GPU/Core/Reflection.hpp"
#include "Compages/GPU/Core/RenderState.hpp"
#include "Compages/GPU/Device.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/GPU/Texture.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

// ****************************************************************************
//! \file
//! \brief The contract every graphics backend must honour.
//!
//! This file declares free functions and never defines them. Exactly one
//! backend implements them, chosen when the project is compiled through the
//! GPU_BACKEND variable of Makefile.common. There is no virtual method and no
//! function pointer anywhere: a call from the compages::gpu:: layer into the backend is a
//! plain direct call, resolved at link time.
//!
//! The consequence for anyone adding a backend is pleasant: the compiler lists
//! precisely what is missing. The consequence for the public API is that it
//! mentions no graphics API at all, which is the whole point.
//!
//! What the backends differ on, for the record. The OpenGL 4.5 backend creates
//! a buffer with:
//! \code
//! glCreateBuffers(1, &name);
//! glNamedBufferStorage(name, bytes, data, flags);
//! \endcode
//! while an OpenGL 4.1 backend, the newest macOS ever supported, would have to
//! write the same thing as:
//! \code
//! glGenBuffers(1, &name);
//! glBindBuffer(GL_ARRAY_BUFFER, name);
//! glBufferData(GL_ARRAY_BUFFER, bytes, data, usage);
//! glBindBuffer(GL_ARRAY_BUFFER, 0);
//! \endcode
//! Binding an object merely to configure it is what Direct State Access
//! removed, and it is also what makes the order of calls fragile. Only the 4.5
//! backend is implemented; see doc/Philosophy.md.
// ****************************************************************************

namespace compages::gpu::backend
{

// ----------------------------------------------------------------------------
//! \brief What a backend calls one of its own objects.
//!
//! Wide enough for the name OpenGL gives an object and for the pointer sized
//! handle other graphics APIs use. The compages::gpu:: layer never looks inside one: it
//! stores it in a pool and hands it back to the backend.
// ----------------------------------------------------------------------------
using NativeId = std::uint64_t;

//! \brief The value meaning no object.
constexpr NativeId NO_OBJECT = 0u;

// ----------------------------------------------------------------------------
//! \brief Load the driver entry points and check the device is usable.
//!
//! Called by compages::gpu::init(), which has already made sure it is not called twice.
//!
//! \param[in] p_load the symbol loader given by the caller.
//! \param[out] p_info filled with what the driver is and allows.
//! \return success, or why this device cannot be used.
// ----------------------------------------------------------------------------
[[nodiscard]] Status init(LoadProc p_load, DeviceInfo& p_info);

// ----------------------------------------------------------------------------
//! \brief Drop whatever the backend holds globally.
//!
//! Called by compages::gpu::shutdown() once the resource pools are already empty.
// ----------------------------------------------------------------------------
void shutdown();

// ----------------------------------------------------------------------------
//! \brief Name of this backend, for logs and error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* name();

// ----------------------------------------------------------------------------
//! \brief Reserve memory on the device and optionally fill it.
//!
//! \param[in] p_bytes how much memory, never zero.
//! \param[in] p_data what to put in it, or nullptr to leave it uninitialized.
//! \param[in] p_kind what the memory is for.
//! \param[in] p_usage how often it will be written from the CPU. Immutable means
//! never again, and the backend is expected to tell the driver so.
//! \return what the backend calls the new object, or why it could not be made.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<NativeId> createBuffer(std::size_t p_bytes,
                                           const void* p_data,
                                           BufferKind p_kind,
                                           BufferUsage p_usage);

// ----------------------------------------------------------------------------
//! \brief Give the memory back to the device.
// ----------------------------------------------------------------------------
void destroyBuffer(NativeId p_buffer);

// ----------------------------------------------------------------------------
//! \brief Overwrite part of a buffer from the CPU.
//!
//! The caller has already checked that the range fits and that the buffer was not
//! created immutable.
// ----------------------------------------------------------------------------
void writeBuffer(NativeId p_buffer,
                 std::size_t p_offset,
                 std::size_t p_bytes,
                 const void* p_data);

// ----------------------------------------------------------------------------
//! \brief Read part of a buffer back into CPU memory.
//!
//! Waits for the device to finish whatever it was doing with that memory, so this
//! is for tests, screenshots and debugging rather than for every frame.
// ----------------------------------------------------------------------------
void readBuffer(NativeId p_buffer,
                std::size_t p_offset,
                std::size_t p_bytes,
                void* p_data);

// ----------------------------------------------------------------------------
//! \brief Compile the source of one pipeline stage.
//!
//! \param[in] p_stage which stage it is.
//! \param[in] p_source the shader source, not necessarily null terminated.
//! \return what the backend calls the compiled stage, or the log the compiler
//! produced. The log is passed on as written: its line numbers are the whole
//! reason anybody reads it.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<NativeId> compileShader(ShaderStage p_stage,
                                            std::string_view p_source);

// ----------------------------------------------------------------------------
//! \brief Drop a compiled stage.
// ----------------------------------------------------------------------------
void destroyShader(NativeId p_shader);

// ----------------------------------------------------------------------------
//! \brief Link compiled stages into a program.
//! \return the program, or the log the linker produced.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<NativeId> linkProgram(
    std::span<const NativeId> p_shaders);

// ----------------------------------------------------------------------------
//! \brief Drop a linked program.
// ----------------------------------------------------------------------------
void destroyProgram(NativeId p_program);

// ----------------------------------------------------------------------------
//! \brief Ask the driver what the program declares.
//!
//! Called once, right after linking. The offsets of block members must be the
//! ones the driver really chose, never ones computed from the std140 rules: the
//! rules say what a well behaved driver should do, this says what it did.
//!
//! \param[in] p_stages which stages went into the program. Needed because some
//! questions only make sense for some stages, such as the work group size of a
//! compute shader, which it is an error to ask of a program without one.
// ----------------------------------------------------------------------------
void reflectProgram(NativeId p_program,
                    std::span<const ShaderStage> p_stages,
                    ProgramReflection& p_reflection);

// ----------------------------------------------------------------------------
//! \brief Write one uniform of a program.
//!
//! One function rather than one per type, so that a backend has a single switch
//! to write and the compiler checks it covers every case.
//!
//! \param[in] p_program the program owning the uniform.
//! \param[in] p_location where the uniform lives, as reflection reported it.
//! \param[in] p_type what the shader declares it as. The caller has already
//! checked that it matches what is being written.
//! \param[in] p_data the value, laid out as the type says.
// ----------------------------------------------------------------------------
void setUniform(NativeId p_program,
                int p_location,
                DataType p_type,
                const void* p_data);

//! \brief Write \c p_count consecutive array elements starting at
//! \c p_location. Used for \c uniform mat4 uJoints[N].
void setUniformArray(NativeId p_program,
                     int p_location,
                     DataType p_type,
                     const void* p_data,
                     int p_count);

// ----------------------------------------------------------------------------
//! \brief Say which binding point a uniform block is to be read from.
//!
//! \param[in] p_block_index position of the block in the list reflection
//! reported, which is how the driver identifies it.
// ----------------------------------------------------------------------------
void bindUniformBlock(NativeId p_program, int p_block_index, int p_binding);

// ----------------------------------------------------------------------------
//! \brief Say which binding point a shader storage block is to be read from.
// ----------------------------------------------------------------------------
void bindStorageBlock(NativeId p_program, int p_block_index, int p_binding);

// ----------------------------------------------------------------------------
//! \brief Make this program the one a dispatch will run.
// ----------------------------------------------------------------------------
void useProgram(NativeId p_program);

// ----------------------------------------------------------------------------
//! \brief Run a compute program over a grid of work groups.
// ----------------------------------------------------------------------------
void dispatch(std::uint32_t p_groups_x,
              std::uint32_t p_groups_y,
              std::uint32_t p_groups_z);

// ----------------------------------------------------------------------------
//! \brief Make earlier writes of these kinds visible to later reads.
// ----------------------------------------------------------------------------
void memoryBarrier(Barrier p_what);

// ----------------------------------------------------------------------------
//! \brief Put a buffer on a numbered binding point, where whichever programs
//! declare a block on that number will read it.
//!
//! The other half of bindUniformBlock(): that one says which number a program
//! reads a block from, this one says what is on that number. Neither mentions the
//! other, which is the point of numbered points: a buffer bound once is read by
//! every program that has been told the same number, however many passes later.
//!
//! \param[in] p_kind Uniform or Storage. Anything else is a mistake in the
//! caller.
//! \param[in] p_buffer the memory to be read, or zero to leave the point empty.
//! \param[in] p_binding which numbered point.
//! \param[in] p_offset, p_bytes which part of the buffer, so that one large
//! buffer can hold the blocks of many objects. Zero bytes means the whole of it.
// ----------------------------------------------------------------------------
void bindBufferToPoint(BufferKind p_kind,
                       NativeId p_buffer,
                       int p_binding,
                       std::size_t p_offset,
                       std::size_t p_bytes);

// ----------------------------------------------------------------------------
//! \brief Set aside the memory for a texture and its sampling state.
//!
//! The size and the format are fixed here and never change afterwards, which is
//! what lets a framebuffer or a pipeline refer to a texture without having to
//! check every frame that it is still the shape it was.
//!
//! \param[in] p_desc what the texture is to be, already checked by the caller.
//! \return what the backend calls the new image, or why it could not be made.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<NativeId> createTexture(TextureDesc const& p_desc);

// ----------------------------------------------------------------------------
//! \brief Give the memory back to the device.
// ----------------------------------------------------------------------------
void destroyTexture(NativeId p_texture);

// ----------------------------------------------------------------------------
//! \brief Fill part of a texture from CPU memory.
//!
//! The caller has already checked that the region fits and that the pixels handed
//! over are the right number of bytes for it.
// ----------------------------------------------------------------------------
void writeTexture(NativeId p_texture,
                  TextureDesc const& p_desc,
                  std::uint32_t p_level,
                  std::uint32_t p_x,
                  std::uint32_t p_y,
                  std::uint32_t p_z,
                  std::uint32_t p_width,
                  std::uint32_t p_height,
                  std::uint32_t p_depth,
                  const void* p_pixels);

// ----------------------------------------------------------------------------
//! \brief Read a whole level of detail back into CPU memory.
// ----------------------------------------------------------------------------
void readTexture(NativeId p_texture,
                 TextureDesc const& p_desc,
                 std::uint32_t p_level,
                 std::size_t p_bytes,
                 void* p_pixels);

// ----------------------------------------------------------------------------
//! \brief Build the smaller levels of detail from the largest one.
// ----------------------------------------------------------------------------
void generateMipmaps(NativeId p_texture);

// ----------------------------------------------------------------------------
//! \brief Change how a texture is read between its pixels.
// ----------------------------------------------------------------------------
void setTextureFilter(NativeId p_texture,
                      Filter p_magnify,
                      Filter p_minify,
                      bool p_has_mipmaps);

// ----------------------------------------------------------------------------
//! \brief Change what happens outside a texture.
// ----------------------------------------------------------------------------
void setTextureWrap(NativeId p_texture, Wrap p_x, Wrap p_y, Wrap p_z);

// ----------------------------------------------------------------------------
//! \brief Make a texture readable by a shader on one texture unit.
// ----------------------------------------------------------------------------
void bindTexture(NativeId p_texture, std::uint32_t p_unit);

// ----------------------------------------------------------------------------
//! \brief Make a texture writable by a shader, as an image.
// ----------------------------------------------------------------------------
void bindTextureAsImage(NativeId p_texture,
                        TextureDesc const& p_desc,
                        std::uint32_t p_unit,
                        ImageAccess p_access,
                        std::uint32_t p_level);

// ****************************************************************************
//! \brief One field of a vertex, matched to the attribute slot that reads it.
//!
//! What comes out of checking a VertexLayout against a program: the fields the
//! shader really declared, each with the slot the driver assigned to it. Fields
//! the shader ignores are not here, which is why this is what the backend is
//! given rather than the layout itself.
// ****************************************************************************
struct VertexAttribute
{
    //! \brief The attribute slot the driver assigned, as reflection reported it.
    int location = -1;
    //! \brief How the field is stored and how the shader is to read it.
    AttributeFormat format;
    //! \brief Distance in bytes from the start of the vertex.
    std::uint32_t offset = 0u;
    //! \brief Does the field change once per instance rather than once per
    //! vertex?
    bool per_instance = false;
};

// ----------------------------------------------------------------------------
//! \brief Set up a way of reading vertices, or hand back one already set up.
//!
//! Two pipelines that read a vertex identically are given the same object, so a
//! mesh drawn by three passes that all read position, normal and texture
//! coordinates costs one of these rather than three. The backend keeps the count
//! of how many pipelines asked for each, and only lets go of one when the last of
//! them does.
//!
//! What is deliberately *not* part of it is which buffer the vertices come from.
//! That is bound at draw time, which is exactly what lets one description serve
//! every mesh laid out the same way, and what the previous design could not do.
//!
//! \param[in] p_attributes the fields the shader reads, already matched to slots.
//! \param[in] p_stride distance in bytes from one vertex to the next.
//! \return what the backend calls it, or why it could not be made.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<NativeId> acquireVertexReader(
    std::span<const VertexAttribute> p_attributes, std::uint32_t p_stride);

// ----------------------------------------------------------------------------
//! \brief Say that one fewer pipeline needs this way of reading vertices.
// ----------------------------------------------------------------------------
void releaseVertexReader(NativeId p_reader);

// ----------------------------------------------------------------------------
//! \brief How many distinct ways of reading a vertex the backend is holding.
//!
//! Only used to show that the sharing works, and to catch it going wrong.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t vertexReadersHeld();

// ----------------------------------------------------------------------------
//! \brief Choose the program, the way of reading vertices, and the state, all at
//! once.
//!
//! One call rather than three so that a backend may skip whatever is already
//! set, which it can only know if it is told everything together.
// ----------------------------------------------------------------------------
void bindPipeline(NativeId p_program,
                  NativeId p_reader,
                  RenderState const& p_state);

// ----------------------------------------------------------------------------
//! \brief Throw away whatever the backend remembers about the state of the
//! device.
//!
//! Because bindPipeline() is allowed to send only differences, it has to be told
//! when its record of the device stops being true, which is whenever code outside
//! this library has talked to the same context.
// ----------------------------------------------------------------------------
void forgetRenderState();

// ----------------------------------------------------------------------------
//! \brief Draw every filled polygon as its edges, whatever the pipelines say.
// ----------------------------------------------------------------------------
void showWireframe(bool p_enabled);
[[nodiscard]] bool wireframeShown();

// ----------------------------------------------------------------------------
//! \brief Say where the vertices are.
//!
//! Kept apart from the way of reading them on purpose: this is the half that
//! changes per mesh, while the other half changes per layout. Separating them is
//! what lets one description serve every mesh laid out the same way.
// ----------------------------------------------------------------------------
void bindVertexBuffer(NativeId p_reader,
                      NativeId p_buffer,
                      std::uint32_t p_stride);

// ----------------------------------------------------------------------------
//! \brief Say which vertices to read, and in which order.
// ----------------------------------------------------------------------------
void bindIndexBuffer(NativeId p_reader, NativeId p_buffer);

// ----------------------------------------------------------------------------
//! \brief Point the device at a target and put it in a known state.
//!
//! The caller has already checked the size is not zero.
//!
//! \param[in] p_target what the backend calls the framebuffer, or zero for the
//! window. Zero is not a missing value here: it is what the driver calls the
//! default framebuffer.
// ----------------------------------------------------------------------------
void beginPass(PassDesc const& p_desc, NativeId p_target);

// ----------------------------------------------------------------------------
//! \brief Make an empty framebuffer, with nothing attached yet.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<NativeId> createFramebuffer();

// ----------------------------------------------------------------------------
//! \brief Wire a colour texture to one numbered attachment.
// ----------------------------------------------------------------------------
void attachColor(NativeId p_framebuffer,
                 std::uint32_t p_index,
                 NativeId p_texture,
                 std::uint32_t p_level);

// ----------------------------------------------------------------------------
//! \brief Wire a depth, or depth-and-stencil, texture to the depth attachment.
// ----------------------------------------------------------------------------
void attachDepth(NativeId p_framebuffer,
                 NativeId p_texture,
                 std::uint32_t p_level,
                 bool p_stencil);

// ----------------------------------------------------------------------------
//! \brief Say how many colour attachments will be written, or none at all.
// ----------------------------------------------------------------------------
void setColorCount(NativeId p_framebuffer, std::uint32_t p_count);

// ----------------------------------------------------------------------------
//! \brief Ask the driver whether the attachments make a target it can draw into.
//!
//! \return why not, in the driver's words, or success.
// ----------------------------------------------------------------------------
[[nodiscard]] Status checkFramebuffer(NativeId p_framebuffer);

// ----------------------------------------------------------------------------
//! \brief Give the framebuffer back. The textures it named are left alone.
// ----------------------------------------------------------------------------
void destroyFramebuffer(NativeId p_framebuffer);

// ----------------------------------------------------------------------------
//! \brief Finish with the target of the open pass.
// ----------------------------------------------------------------------------
void endPass();

// ----------------------------------------------------------------------------
//! \brief Draw vertices read in order.
//!
//! Everything has been checked: a pipeline is bound, a buffer is bound, and the
//! count makes whole primitives.
// ----------------------------------------------------------------------------
void draw(Primitive p_primitive, std::size_t p_first, std::size_t p_count);

// ----------------------------------------------------------------------------
//! \brief Draw vertices named by indices.
// ----------------------------------------------------------------------------
void drawIndexed(Primitive p_primitive,
                 IndexType p_type,
                 std::size_t p_first,
                 std::size_t p_count);

// ----------------------------------------------------------------------------
//! \brief Draw the same vertices several times, once per instance.
// ----------------------------------------------------------------------------
void drawInstanced(Primitive p_primitive,
                   std::size_t p_first,
                   std::size_t p_count,
                   std::size_t p_instances);

// ----------------------------------------------------------------------------
//! \brief Draw as a command buffer sitting on the device says.
//!
//! The command is four unsigned integers: how many vertices, how many
//! instances, which vertex to start at, and which instance to start at.
// ----------------------------------------------------------------------------
void drawIndirect(Primitive p_primitive, NativeId p_commands);

// ----------------------------------------------------------------------------
//! \brief Wait until the device has finished everything asked of it.
//!
//! For tests and for measuring, never for a frame: waiting for the device is
//! exactly what a frame should not do.
// ----------------------------------------------------------------------------
void waitForDevice();

// ----------------------------------------------------------------------------
//! \brief Read the picture back out of the target, four bytes per pixel, bottom
//! row first.
//!
//! Waits for the device. The caller has already checked that the region fits.
// ----------------------------------------------------------------------------
void readTargetPixels(std::uint32_t p_x,
                      std::uint32_t p_y,
                      std::uint32_t p_width,
                      std::uint32_t p_height,
                      void* p_pixels);

} // namespace compages::gpu::backend
