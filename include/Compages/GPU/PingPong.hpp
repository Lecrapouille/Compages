// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Two copies of a state that swap roles at each step.
//!
//! A parallel step must not read and write the same state: one element could
//! read a neighbour that another element has already changed. A ping-pong
//! keeps two copies:
//! - current(): the state to read, which is also the latest one to show;
//! - next(): where the step writes the new state.
//!
//! After the step, swap() exchanges the two roles. Nothing is copied: only
//! the roles change.
//!
//! \code
//! step 1: A -> B
//! step 2: B -> A
//! step 3: A -> B
//! \endcode
//!
//! What it holds:
//! - PingPong: two storage buffers, for a compute shader (13_Galaxy).
//! - dispatch(program, pingpong, blocks): one step in one call.
//! - TexturePingPong: two textures and the framebuffers that draw into them,
//!   for a fragment shader working on an image (11a_GameOfLife).
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/ComputeProgram.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Texture.hpp"

#include <cstdint>
#include <limits>
#include <span>
#include <string_view>

namespace compages::gpu
{

// ****************************************************************************
//! \brief Two storage buffers that swap roles at each step: read the current
//! state, write the next one.
//!
//! \code
//! auto state = compages::gpu::PingPong<Particle>::from(start).take();
//! COMPAGES_TRY(compages::gpu::dispatch(step, state,
//!                                      { .read = "Previous", .write = "Next" }));
//! drawable.borrowVertices(state.current());   // show the new state
//! \endcode
//!
//! Both buffers are Storage, so either can be drawn as vertices.
//!
//! \tparam T one element, trivially copyable (see Buffer).
// ****************************************************************************
template <typename T>
class PingPong: private compages::NotCopiable
{
public:

    //! \brief No buffers yet: valid() is false.
    PingPong() = default;

    // ------------------------------------------------------------------------
    //! \brief Two buffers of p_count elements whose content is undefined.
    //!
    //! Fits when the first step writes every element before any is read.
    //!
    //! \param[in] p_count elements in each buffer.
    //! \return the pair, or why the device memory could not be reserved.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<PingPong> create(std::size_t p_count)
    {
        PingPong pair;
        for (StorageBuffer<T>& buffer : pair.m_buffer)
        {
            COMPAGES_TRY_ASSIGN(
                buffer, StorageBuffer<T>::create(p_count, BufferUsage::Stream));
        }
        pair.m_count = p_count;
        return pair;
    }

    // ------------------------------------------------------------------------
    //! \brief Two buffers both filled with the same starting state.
    //!
    //! Filling both, not only current(), means a step that writes only some
    //! elements leaves the others at their start value rather than garbage.
    //!
    //! \param[in] p_data the starting state, one entry per element.
    //! \return the pair, or why the device memory could not be reserved.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<PingPong> from(std::span<const T> p_data)
    {
        PingPong pair;
        for (StorageBuffer<T>& buffer : pair.m_buffer)
        {
            COMPAGES_TRY_ASSIGN(
                buffer, StorageBuffer<T>::from(p_data, BufferUsage::Stream));
        }
        pair.m_count = p_data.size();
        return pair;
    }

    //! \brief The latest state: the one the next step reads and the one to
    //! draw.
    //! \return the buffer holding the latest state.
    [[nodiscard]] StorageBuffer<T>& current()
    {
        return m_buffer[m_current];
    }

    //! \copydoc current()
    [[nodiscard]] StorageBuffer<T> const& current() const
    {
        return m_buffer[m_current];
    }

    //! \brief Where the next step writes. Its content is the state of two
    //! steps ago: do not read it before the step has run.
    //! \return the buffer the next step writes.
    [[nodiscard]] StorageBuffer<T>& next()
    {
        return m_buffer[1 - m_current];
    }

    //! \copydoc next()
    [[nodiscard]] StorageBuffer<T> const& next() const
    {
        return m_buffer[1 - m_current];
    }

    //! \brief What next() holds becomes current(). Nothing is copied; bind
    //! both buffers again before the next step, since they changed roles.
    void swap()
    {
        m_current = 1 - m_current;
    }

    //! \brief Elements in each buffer, the number to pass to dispatchItems().
    //! \return the number of elements in each buffer.
    [[nodiscard]] std::size_t count() const
    {
        return m_count;
    }

    //! \brief True once create() or from() has succeeded, false after a move.
    //! \return true when both buffers exist.
    [[nodiscard]] bool valid() const
    {
        return m_buffer[0].valid() && m_buffer[1].valid();
    }

private:

    StorageBuffer<T> m_buffer[2];
    int m_current = 0;
    std::size_t m_count = 0u;
};

// ****************************************************************************
//! \brief The names of the two storage blocks of a ping-pong step, as the
//! compute shader declares them.
//!
//! Always written with the field names, so that the two cannot be exchanged
//! by mistake: { .read = "Input", .write = "Output" }.
// ****************************************************************************
struct PingPongBlocks
{
    //! \brief The block that receives current(), as in "readonly buffer
    //! Input".
    std::string_view read;
    //! \brief The block that receives next(), as in "writeonly buffer Output".
    std::string_view write;
};

// ----------------------------------------------------------------------------
//! \brief Run one step of a buffer ping-pong: the common case in one call.
//!
//! In order: binds current() to p_blocks.read and next() to p_blocks.write,
//! runs one copy of the program per element, waits until the writes are
//! visible to a later compute pass and to a draw
//! (Barrier::Storage | Barrier::VertexAttrib), then swaps. On a failure,
//! nothing is swapped.
//!
//! \code
//! COMPAGES_TRY(compages::gpu::dispatch(m_step, m_stars,
//!                                      { .read = "Input", .write = "Output" }));
//! \endcode
//!
//! \param[in] p_program the step. It may declare other blocks, bound by hand.
//! \param[in] p_state the two buffers, swapped on success.
//! \param[in] p_blocks the names of the two storage blocks.
//! \return why the step could not run: an unknown block name, a released
//! buffer, or more than 2^32 - 1 elements.
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] Status dispatch(ComputeProgram& p_program,
                              PingPong<T>& p_state,
                              PingPongBlocks const& p_blocks)
{
    if (p_state.count() > std::numeric_limits<std::uint32_t>::max())
    {
        return failure("compages::gpu::dispatch(program, pingpong) cannot "
                       "dispatch more than 2^32-1 elements");
    }
    COMPAGES_TRY(p_program.bind(p_blocks.read, p_state.current()));
    COMPAGES_TRY(p_program.bind(p_blocks.write, p_state.next()));
    COMPAGES_TRY(
        p_program.dispatchItems(static_cast<std::uint32_t>(p_state.count())));
    barrier(Barrier::Storage | Barrier::VertexAttrib);
    p_state.swap();
    return success();
}

// ****************************************************************************
//! \brief Two textures that swap roles at each step, each with the
//! framebuffer that draws into it.
//!
//! The image version of PingPong, for a step written as a fragment shader:
//! read current() as a texture, draw the new state into next().
//!
//! \code
//! COMPAGES_TRY_ASSIGN(m_field, compages::gpu::TexturePingPong::create(desc));
//! // each step
//! m_step["previous"] = m_field.current();
//! {
//!     compages::gpu::RenderPass into(m_field.next(), { .clear_color = false });
//!     m_step.draw(3u);
//! }
//! m_field.swap();
//! m_show["image"] = m_field.current();
//! \endcode
//!
//! The pass stays in the caller's code on purpose: it says where the draw
//! goes. A TexturePingPong can be moved but not copied.
// ****************************************************************************
class TexturePingPong: private compages::NotCopiable
{
public:

    //! \brief No textures yet: valid() is false.
    TexturePingPong() = default;

    // ------------------------------------------------------------------------
    //! \brief Two textures made from the same description, each wired to its
    //! framebuffer. Their content is undefined: write current() or clear both
    //! targets before the first step.
    //!
    //! \param[in] p_desc the size, format and sampling of both textures.
    //! \return the pair, or why a texture or a framebuffer could not be made.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<TexturePingPong>
    create(TextureDesc const& p_desc);

    //! \brief The latest state, to read in the step and to show.
    //! \return the texture holding the latest state.
    [[nodiscard]] Texture& current()
    {
        return m_texture[m_current];
    }

    //! \copydoc current()
    [[nodiscard]] Texture const& current() const
    {
        return m_texture[m_current];
    }

    //! \brief Where the next step draws: the target of the other texture.
    //! \return the framebuffer to open a RenderPass on.
    [[nodiscard]] Framebuffer const& next() const
    {
        return m_target[1 - m_current];
    }

    //! \brief The target of current(), to draw over the latest state in place
    //! (paint a stamp, clear it), without a step.
    //! \return the framebuffer of current().
    [[nodiscard]] Framebuffer const& currentTarget() const
    {
        return m_target[m_current];
    }

    //! \brief What was drawn into next() becomes current(). Nothing is copied.
    void swap()
    {
        m_current = 1 - m_current;
    }

    //! \brief True once create() has succeeded, false after a move.
    //! \return true when both textures and targets exist.
    [[nodiscard]] bool valid() const
    {
        return m_target[0].valid() && m_target[1].valid();
    }

private:

    Texture m_texture[2];
    Framebuffer m_target[2];
    int m_current = 0;
};

} // namespace compages::gpu
