// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/Core/Vector.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

// ****************************************************************************
//! \file
//! \brief Where a frame is drawn, and what it starts from.
//!
//! A pass is a span of drawing with one target, one viewport, and one decision
//! about what the target held before. Every draw inside a pass goes to the same
//! place, and a draw outside any pass is a mistake rather than a draw into
//! whatever happened to be bound.
//!
//! In the examples the window already opened a pass over itself before calling
//! draw(), so drawing to the screen needs nothing but the colour to start from:
//! \code
//! compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
//! m_triangle.draw();
//! \endcode
//!
//! Drawing into a texture opens a pass over its framebuffer. Passes nest: the
//! one below is suspended meanwhile and resumed, not cleared, at the end:
//! \code
//! {
//!     compages::gpu::RenderPass offscreen(m_framebuffer, { .color = { 0, 0, 0, 1 } });
//!     m_scene.draw();
//! }                          // back to the window
//! m_screen["image"] = m_color;
//! m_screen.draw(3u);
//! \endcode
//!
//! The pass ends when the object goes out of scope. There is nothing to
//! remember to call, and nothing that stays half open when a frame returns
//! early.
// ****************************************************************************

namespace compages::gpu
{

// ****************************************************************************
//! \brief What a pass draws into and what it starts from.
// ****************************************************************************
struct PassDesc
{
    //! \brief Left edge of the part of the target being drawn into, in pixels.
    std::uint32_t x = 0u;
    //! \brief Bottom edge, in pixels. Zero is the bottom of the target, not the
    //! top, which is the one place this library keeps the convention of the
    //! graphics API rather than that of a window system.
    std::uint32_t y = 0u;
    //! \brief Width of the part being drawn into. Never zero: a pass drawing
    //! into nothing is a size that was never computed, most often a window
    //! whose size was read before it was mapped.
    std::uint32_t width = 0u;
    //! \brief Height of the part being drawn into.
    std::uint32_t height = 0u;

    //! \brief Should the target be filled with a colour before anything is
    //! drawn?
    //!
    //! Almost always yes. Leaving it off keeps whatever the previous frame
    //! drew, which is what an accumulating effect wants and what a forgotten
    //! clear looks like: a trail of smeared images.
    bool clear_color = true;
    //! \brief The colour to start from, as red, green, blue and alpha between 0
    //! and 1.
    Vector4f color{ 0.0f, 0.0f, 0.0f, 1.0f };

    //! \brief Should the recorded distances be forgotten before anything is
    //! drawn?
    //!
    //! Yes for the first pass of a frame. A second pass drawing on top of the
    //! first, such as an overlay that must respect the depth already there,
    //! says no.
    bool clear_depth = true;
    //! \brief The distance to start from. One is as far away as it gets, which
    //! is what a depth test using Less needs in order to let anything through.
    float depth = 1.0f;

    //! \brief Which target to draw into. Empty means the target of the pass
    //! this one is opened in, or the window when no pass is open: a viewer
    //! can draw an example into a texture without the example knowing. A
    //! handle that has been released is refused when the pass starts, rather
    //! than drawing into the window by accident.
    FramebufferHandle target{};
};

// ****************************************************************************
//! \brief One span of drawing, open until it goes out of scope.
// ****************************************************************************
class RenderPass
{
public:

    // ------------------------------------------------------------------------
    //! \brief Start drawing into the target p_desc names.
    //!
    //! \param[in] p_desc where, and what to start from.
    //! \return the open pass, or why it could not be started: a size of zero,
    //! or a framebuffer released or too small.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<RenderPass> begin(PassDesc const& p_desc);

    // ------------------------------------------------------------------------
    //! \brief Start drawing, as begin() does, recording why not as the frame
    //! error (see Errors.hpp). open() then says false and draws are refused.
    // ------------------------------------------------------------------------
    explicit RenderPass(PassDesc const& p_desc);

    // ------------------------------------------------------------------------
    //! \brief Start drawing into a framebuffer, all of it unless p_desc says
    //! which part.
    //!
    //! \code
    //! compages::gpu::RenderPass pass(m_framebuffer, { .color = { 1, 1, 1, 1 } });
    //! \endcode
    // ------------------------------------------------------------------------
    explicit RenderPass(Framebuffer const& p_target, PassDesc p_desc = {});

    RenderPass(RenderPass&& p_other) noexcept;
    RenderPass& operator=(RenderPass&& p_other) noexcept;
    RenderPass(RenderPass const&) = delete;
    RenderPass& operator=(RenderPass const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Close the pass.
    // ------------------------------------------------------------------------
    ~RenderPass();

    // ------------------------------------------------------------------------
    //! \brief Close the pass now rather than at the end of the scope, and
    //! resume the one it was opened over, if any.
    //!
    //! Passes close in the reverse order they were opened; closing one with
    //! others still open over it closes those too.
    // ------------------------------------------------------------------------
    void end();

    // ------------------------------------------------------------------------
    //! \brief Is this pass still open?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool open() const
    {
        return m_open;
    }

    // ------------------------------------------------------------------------
    //! \brief Where in the target this pass draws, and what it started from.
    // ------------------------------------------------------------------------
    [[nodiscard]] PassDesc const& description() const
    {
        return m_desc;
    }

private:

    RenderPass(PassDesc const& p_desc, std::size_t p_depth)
        : m_desc(p_desc), m_depth(p_depth), m_open(true)
    {
    }

    PassDesc m_desc;
    //! \brief How many passes are open, this one included, once it is.
    std::size_t m_depth = 0u;
    bool m_open = false;
};

// ----------------------------------------------------------------------------
//! \brief Fill the target of the open pass with a colour, keeping the depth.
//!
//! \code
//! compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
//! \endcode
// ----------------------------------------------------------------------------
void clear(Vector4f const& p_color);

//! \brief Same, fully opaque.
void clear(Vector3f const& p_color);

//! \brief Same, from braces: three numbers are opaque, four give the alpha.
void clear(std::initializer_list<float> p_color);

// ----------------------------------------------------------------------------
//! \brief Forget the distances recorded in the open pass, so that anything
//! drawn next passes the depth test.
// ----------------------------------------------------------------------------
void clearDepth(float p_depth = 1.0f);

// ----------------------------------------------------------------------------
//! \brief Clear both colour and depth at once, what a 3D frame starts with.
// ----------------------------------------------------------------------------
void clear(Vector3f const& p_color, float p_depth);

// ----------------------------------------------------------------------------
//! \brief Is a pass open right now?
//!
//! What a draw call checks before agreeing to draw, since drawing outside a
//! pass goes somewhere nobody chose.
// ----------------------------------------------------------------------------
[[nodiscard]] bool inRenderPass();

// ----------------------------------------------------------------------------
//! \brief Where the open pass is drawing, or a description of zero size when
//! none is open.
// ----------------------------------------------------------------------------
[[nodiscard]] PassDesc const& currentPass();

// ----------------------------------------------------------------------------
//! \brief Read the picture back out of the target of the open pass.
//!
//! Four bytes per pixel, red, green, blue and alpha, with the bottom row first,
//! because that is the order the target holds them in and turning them over
//! here would only move the surprise elsewhere.
//!
//! This waits for the device to finish everything it was asked, so it belongs
//! in a screenshot, in a test, or in a tool, never in a frame.
//!
//! \param[in] p_x left edge of what to read, relative to the target.
//! \param[in] p_y bottom edge.
//! \param[in] p_width how many pixels across. Zero means the whole width of the
//! open pass.
//! \param[in] p_height how many pixels up. Zero means the whole height.
//! \return the pixels, or why they could not be read.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<std::vector<std::byte>>
readPixels(std::uint32_t p_x = 0u,
           std::uint32_t p_y = 0u,
           std::uint32_t p_width = 0u,
           std::uint32_t p_height = 0u);

} // namespace compages::gpu
