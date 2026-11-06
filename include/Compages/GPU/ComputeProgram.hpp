// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Make the graphics card compute numbers instead of drawing pixels.
//!
//! A graphics card is thousands of small calculators working at the same time.
//! Usually they colour pixels. This file lets them run any calculation over a
//! large table of numbers instead: move a million particles, blur an image,
//! solve a grid of equations. The table stays on the card, so a later drawing
//! can show the results without copying them back.
//!
//! What it holds:
//! - ComputeProgram: the calculation, written in GLSL, and the call that runs
//! it.
//! - dispatch(): the one-line case, one calculator per element of a Buffer.
//! - barrier(): wait until results are written before anything reads them.
//!
//! Two tables that swap at each step (PingPong) are in PingPong.hpp.
//!
//! Demos: 05a_Compute (the smallest case) and 05d_ComputeParticles (a compute
//! pass writing the vertices a draw then reads).
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Enums.hpp"
#include "Compages/GPU/Handle.hpp"
#include "Compages/GPU/Shader.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace compages::gpu
{

// ****************************************************************************
//! \brief A calculation that the graphics card runs many times at once.
//!
//! Think of a spreadsheet formula applied to every row at the same time: the
//! program says what to do with one element, and the card runs one copy per
//! element, in parallel. The copies are grouped into small teams called work
//! groups; the shader chooses the team size with layout(local_size_x = N).
//!
//! \code
//! compages::gpu::ComputeProgram step;
//! COMPAGES_TRY(step.loadSource(R"(#version 450 core
//!     layout(local_size_x = 64) in;
//!     layout(std430) buffer Values { float values[]; };
//!     void main() { values[gl_GlobalInvocationID.x] *= 2.0; })"));
//! COMPAGES_TRY(compages::gpu::dispatch(step, values)); // doubles each value
//! \endcode
//!
//! Nothing is drawn. The results stay in the buffers the program wrote; call
//! barrier() before another pass or a draw reads them. A ComputeProgram can be
//! moved but not copied: it owns the program on the device.
// ****************************************************************************
class ComputeProgram: private compages::NotCopiable
{
private:

    explicit ComputeProgram(Program p_program) : m_program(std::move(p_program))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Compile and link a program whose only stage is a compute shader.
    //!
    //! Fails, with the driver's message, when the source does not compile, and
    //! also when it compiles but declares no layout(local_size_x = ...):
    //! without a work group size there is no way to know how many copies one
    //! group runs.
    //!
    //! \param[in] p_source the GLSL text, starting with its \#version line.
    //! \return the program, or the reason it could not be built.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<ComputeProgram>
    fromSource(std::string_view p_source);

    // ------------------------------------------------------------------------
    //! \brief Same as fromSource(), reading the GLSL text from a file.
    //!
    //! \param[in] p_path path of the .comp file. Relative paths are resolved
    //! from the current directory, not from the executable.
    //! \return the program, or why the file could not be read or built.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<ComputeProgram>
    fromFile(std::filesystem::path const& p_path);

public:

    //! \brief An empty program: valid() is false until loadSource() or
    //! loadFile() succeeds.
    ComputeProgram() = default;

    // ------------------------------------------------------------------------
    //! \brief Compile and link a program whose only stage is a compute
    //! shader.
    //!
    //! Fails, with the driver's message, when the source does not compile,
    //! and also when it compiles but declares no layout(local_size_x = ...):
    //! without a work group size there is no way to know how many copies one
    //! group runs. On failure this object is left as it was, so a working
    //! program is not lost because a reloaded source has a typo.
    //!
    //! \code
    //! COMPAGES_TRY(m_simulation.loadSource(COMPUTE_SHADER));
    //! \endcode
    //!
    //! \param[in] p_source the GLSL text, starting with its \#version line.
    //! \return the compiler or linker log when it failed.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadSource(std::string_view p_source)
    {
        COMPAGES_TRY_ASSIGN(*this, fromSource(p_source));
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Build in place from a file. Like loadSource(), a failure keeps
    //! the previous program.
    //!
    //! \param[in] p_path path of the .comp file.
    //! \return why the file could not be read or built.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadFile(std::filesystem::path const& p_path)
    {
        COMPAGES_TRY_ASSIGN(*this, fromFile(p_path));
        return success();
    }

    // ------------------------------------------------------------------------
    //! \brief Take over another program, left not valid().
    // ------------------------------------------------------------------------
    ComputeProgram(ComputeProgram&&) noexcept = default;

    // ------------------------------------------------------------------------
    //! \brief Release this program, then take over another.
    //! \return this object, now owning the other program.
    // ------------------------------------------------------------------------
    ComputeProgram& operator=(ComputeProgram&&) noexcept = default;

    // ------------------------------------------------------------------------
    //! \brief False when default constructed, moved from, or never loaded.
    //! \return true when the program can be bound and dispatched.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const
    {
        return m_program.valid();
    }

    // ------------------------------------------------------------------------
    //! \brief The underlying program, for the calls ComputeProgram does not
    //! forward, such as setting an array uniform.
    //! \return the program, to change.
    // ------------------------------------------------------------------------
    [[nodiscard]] Program& program()
    {
        return m_program;
    }

    // ------------------------------------------------------------------------
    //! \copydoc program()
    //! \return the program, read only.
    // ------------------------------------------------------------------------
    [[nodiscard]] Program const& program() const
    {
        return m_program;
    }

    // ------------------------------------------------------------------------
    //! \brief What the driver found in the shader: its storage blocks, their
    //! binding points, its uniforms. Useful to print when a bind() fails.
    //! \return the reflection, read once at link time.
    // ------------------------------------------------------------------------
    [[nodiscard]] ProgramReflection const& reflection() const
    {
        return m_program.reflection();
    }

    // ------------------------------------------------------------------------
    //! \brief The team size the shader declared, as {x, y, z}.
    //!
    //! Never {0, 0, 0} on a valid program, since loading refuses a shader
    //! without one. Needed to turn a number of elements into a number of
    //! groups when calling dispatch() by hand.
    //! \return the three sizes, each at least 1 on a valid program.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::array<int, 3> const& workGroupSize() const
    {
        return m_program.reflection().work_group_size;
    }

    // ------------------------------------------------------------------------
    //! \brief Give the shader a buffer to read and write.
    //!
    //! The block is found by name, so the shader does not need a binding =
    //! qualifier, and the binding point is the one the driver chose. The
    //! buffer stays attached to that point until something else is bound there,
    //! so bind again before each dispatch if another pass used the same point.
    //!
    //! \param[in] p_block name of the buffer block in GLSL (Values in
    //! "buffer Values { ... }"), not the name of its array.
    //! \param[in] p_buffer a buffer created with BufferKind::Storage. Any other
    //! kind is refused, as is a released buffer.
    //! \return an error naming the blocks the shader does declare when
    //! p_block is not one of them.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind(std::string_view p_block, BufferHandle p_buffer);

    // ------------------------------------------------------------------------
    //! \brief Same as bind(std::string_view, BufferHandle), from a Buffer.
    //!
    //! Only the device memory is attached: changes still waiting in the CPU
    //! copy are not sent. Call p_buffer.upload() first, or use dispatch(),
    //! which does it.
    //!
    //! \param[in] p_block name of the buffer block in GLSL.
    //! \param[in] p_buffer a Storage buffer.
    //! \return the same errors as the BufferHandle overload.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] Status bind(std::string_view p_block,
                              StorageBuffer<T> const& p_buffer)
    {
        return bind(p_block, p_buffer.handle());
    }

    // ------------------------------------------------------------------------
    //! \brief Set a uniform: a value shared by every copy of the calculation,
    //! such as the time step.
    //!
    //! A wrong name or type does not stop the program: it is recorded as the
    //! frame error (see takeFrameError()) and the uniform keeps its old value.
    //!
    //! \param[in] p_name name of the uniform in GLSL.
    //! \param[in] p_value any type Program::set() accepts (float, int, vectors,
    //! matrices).
    // ------------------------------------------------------------------------
    template <typename T>
    void set(std::string_view p_name, T const& p_value)
    {
        m_program.set(p_name, p_value);
    }

    // ------------------------------------------------------------------------
    //! \brief Run the calculation over a grid of work groups.
    //!
    //! The number of copies run is groups times workGroupSize() on each axis.
    //! The call returns at once: the card works in the background, so call
    //! barrier() before reading the results.
    //!
    //! \param[in] p_groups_x how many groups on X. Must not be zero.
    //! \param[in] p_groups_y how many groups on Y, for 2D work such as an
    //! image.
    //! \param[in] p_groups_z how many groups on Z, for 3D work such as a
    //! volume.
    //! \return an error when gpu::init() was not called, a count is zero, or a
    //! count is above the driver limit
    //! (DeviceInfo::max_compute_work_group_count).
    // ------------------------------------------------------------------------
    [[nodiscard]] Status dispatch(std::uint32_t p_groups_x,
                                  std::uint32_t p_groups_y = 1u,
                                  std::uint32_t p_groups_z = 1u);

    // ------------------------------------------------------------------------
    //! \brief Run enough groups on X to cover this many elements.
    //!
    //! The count is rounded up to whole groups: 1000 elements with a group of
    //! 64 launch 16 groups, so 1024 copies. The shader must skip the 24 extra
    //! ones, usually with "if (i >= values.length()) return;".
    //!
    //! \param[in] p_count how many elements to cover. Must not be zero.
    //! \return an error when p_count is zero, or the same errors as dispatch().
    // ------------------------------------------------------------------------
    [[nodiscard]] Status dispatchItems(std::uint32_t p_count);

private:

    Program m_program;
};

// ----------------------------------------------------------------------------
//! \brief Wait until earlier GPU writes are visible to later GPU reads.
//!
//! The card runs dispatches and draws in the background and may start the next
//! one before the previous one has finished writing. A barrier says "finish
//! these writes first". Forgetting it gives wrong values from time to time,
//! not an error, which makes it hard to find.
//!
//! \code
//! COMPAGES_TRY(step.dispatchItems(count));
//! compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib); // a draw
//!                                                                // reads it
//! \endcode
//!
//! \param[in] p_what who will read next, combined with |: Storage for another
//! compute pass, VertexAttrib for a draw, Command for an indirect draw, All
//! when unsure. Does nothing before gpu::init().
// ----------------------------------------------------------------------------
void barrier(Barrier p_what);

// ----------------------------------------------------------------------------
//! \brief Run a program once per element of a buffer: the common case in one
//! call.
//!
//! In order: sends the changes waiting in the CPU copy of the buffer, binds it
//! to the program's storage block, runs one copy per element, and calls
//! barrier(Barrier::Storage). A draw reading the buffer as vertices still
//! needs barrier(Barrier::VertexAttrib).
//!
//! \code
//! COMPAGES_TRY(compages::gpu::dispatch(step, particles));
//! \endcode
//!
//! \param[in] p_program a program declaring exactly one storage block. With two
//! or more, the buffer would not know where to go: bind() each one and call
//! dispatch() on the program instead.
//! \param[in] p_buffer a Storage buffer, at most 2^32 - 1 elements.
//! \return why the dispatch could not run: not exactly one storage block, a bad
//! buffer, or too many elements.
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] Status dispatch(ComputeProgram& p_program,
                              StorageBuffer<T>& p_buffer)
{
    auto const& blocks = p_program.reflection().storage_blocks;
    if (blocks.size() != 1u)
    {
        return failure(
            "compages::gpu::dispatch(program, buffer) requires exactly one "
            "reflected "
            "storage block, but the program declares " +
            std::to_string(blocks.size()));
    }
    COMPAGES_TRY(p_buffer.upload());
    COMPAGES_TRY(p_program.bind(blocks.front().name, p_buffer));
    if (p_buffer.count() > std::numeric_limits<std::uint32_t>::max())
    {
        return failure("compages::gpu::dispatch(program, "
                       "buffer) cannot dispatch more than "
                       "2^32-1 buffer elements");
    }
    COMPAGES_TRY(
        p_program.dispatchItems(static_cast<std::uint32_t>(p_buffer.count())));
    barrier(Barrier::Storage);
    return success();
}

} // namespace compages::gpu
