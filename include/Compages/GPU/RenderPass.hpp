// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

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
//!     compages::gpu::RenderPass offscreen(m_framebuffer,
//!           { .color = { 0, 0, 0, 1 } });
//!     m_scene.draw();
//! }                          // back to the window
//! m_screen["image"] = m_color;
//! m_screen.draw(3u);
//! \endcode
//!
//! The pass ends when the object goes out of scope. There is nothing to
//! remember to call, and nothing that stays half open when a frame returns
//! early.
//!
//! What it holds:
//! - RenderPass and PassDesc: open a pass, and say how.
//! - clear(), clearDepth(): wipe the open pass.
//! - inRenderPass(), currentPass(), readPixels(): ask about the open pass.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Handle.hpp"

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

namespace compages::gpu
{

// ****************************************************************************
//! \brief What a pass draws into and what it starts from: the canvas, the
//! part of it in use, and whether it is wiped first.
//!
//! \code
//! // Left half of the window, cleared to dark blue:
//! compages::gpu::PassDesc left{ .width = w / 2u, .height = h,
//!                               .color = { 0.0f, 0.0f, 0.2f, 1.0f } };
//! compages::gpu::RenderPass pass(left);
//! \endcode
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
    compages::Vector4f color{ 0.0f, 0.0f, 0.0f, 1.0f };

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

    //! \brief Store the depth the other way round: 1 near, 0 far, in [0, 1]
    //! (OpenGL's glClipControl), what is called "reversed-Z".
    //!
    //! With a Depth32F target, the depth keeps the same relative precision
    //! from a metre to the Moon, where the usual depth runs out of it a few
    //! kilometres away and far faces flicker (z-fighting). The comparisons
    //! of the pipelines are mirrored for you (Less becomes Greater) and
    //! \c depth keeps its meaning (1 is far): only the projection changes,
    //! it must map the near plane to 1 and the far one to 0. The Scene draws
    //! this way (CameraFrame::reversed_projection); a pass drawing over its
    //! picture with its depth must too.
    bool reversed_depth = false;

    //! \brief Which target to draw into. Empty means the target of the pass
    //! this one is opened in, or the window when no pass is open: a viewer
    //! can draw an example into a texture without the example knowing. A
    //! handle that has been released is refused when the pass starts, rather
    //! than drawing into the window by accident.
    FramebufferHandle target{};
};

// ****************************************************************************
//! \brief A drawing session on one canvas, open until it goes out of scope.
//!
//! Like putting a sheet on the easel: everything drawn while the pass is open
//! lands on that sheet, and taking the sheet off happens by itself at the end
//! of the block. A pass can be opened over another one, to draw into a texture
//! for a moment, and the one below is resumed untouched afterwards.
//!
//! \code
//! {
//!     compages::gpu::RenderPass shadow(m_shadow_map);   // into the texture
//!     m_scene.draw();
//! }                                                     // back to the window
//! \endcode
// ****************************************************************************
class RenderPass: private compages::NotCopiable
{
public:

    // ------------------------------------------------------------------------
    //! \brief Start drawing into the target p_desc names. Why it could not
    //! start (a size of zero, a framebuffer released or too small) is
    //! recorded as the frame error (see Errors.hpp); open() then says false
    //! and draws are refused.
    //!
    //! \param[in] p_desc where, and what to start from.
    // ------------------------------------------------------------------------
    explicit RenderPass(PassDesc const& p_desc);

    // ------------------------------------------------------------------------
    //! \brief Start drawing into a framebuffer, all of it unless p_desc says
    //! which part.
    //!
    //! \code
    //! compages::gpu::RenderPass pass(m_framebuffer,
    //!                                { .color = { 1, 1, 1, 1 } });
    //! \endcode
    //!
    //! \param[in] p_target the framebuffer to draw into. A failure, such as a
    //! released framebuffer, is recorded as the frame error.
    //! \param[in] p_desc what to start from. A width and height of zero mean
    //! the whole framebuffer; its target field is replaced by p_target.
    // ------------------------------------------------------------------------
    explicit RenderPass(Framebuffer const& p_target, PassDesc p_desc = {});

    //! \brief Take over the open pass of p_other, which is left closed: the
    //! pass now ends with this object.
    //! \param[in,out] p_other the pass to take over.
    RenderPass(RenderPass&& p_other) noexcept;
    //! \brief End this pass, then take over p_other's.
    //! \param[in,out] p_other the pass to take over.
    //! \return this object, now owning the other pass.
    RenderPass& operator=(RenderPass&& p_other) noexcept;

    // ------------------------------------------------------------------------
    //! \brief Close the pass, as end() does. Nothing happens if it is already
    //! closed or was never opened.
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
    //! \brief Is this pass still open? False after end(), after a move, and
    //! when the constructor could not start it.
    //! \return true while draws are accepted.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool open() const
    {
        return m_open;
    }

    // ------------------------------------------------------------------------
    //! \brief Where in the target this pass draws, and what it started from.
    //! \return the description the pass was opened with, its target resolved.
    // ------------------------------------------------------------------------
    [[nodiscard]] PassDesc const& description() const
    {
        return m_desc;
    }

private:

    //! \brief Open the pass, or say why not. What the constructors call.
    [[nodiscard]] static Result<RenderPass> begin(PassDesc const& p_desc);

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
//!
//! Only the part of the target the pass covers is filled.
//!
//! \param[in] p_color red, green, blue and alpha between 0 and 1.
// ----------------------------------------------------------------------------
void clear(compages::Vector4f const& p_color);

//! \brief Same, fully opaque.
//! \param[in] p_color red, green and blue between 0 and 1.
void clear(compages::Vector3f const& p_color);

//! \brief Same, from braces: three numbers are opaque, four give the alpha.
//! \param[in] p_color three or four numbers between 0 and 1.
void clear(std::initializer_list<float> p_color);

// ----------------------------------------------------------------------------
//! \brief Forget the distances recorded in the open pass, so that anything
//! drawn next passes the depth test. Used between two layers of a frame, such
//! as a scene and a 3D overlay that must always show.
//!
//! \param[in] p_depth the distance to start from; 1 is the farthest.
// ----------------------------------------------------------------------------
void clearDepth(float p_depth = 1.0f);

// ----------------------------------------------------------------------------
//! \brief Clear both colour and depth at once, what a 3D frame starts with.
//!
//! \param[in] p_color red, green and blue between 0 and 1, fully opaque.
//! \param[in] p_depth the distance to start from; 1 is the farthest.
// ----------------------------------------------------------------------------
void clear(compages::Vector3f const& p_color, float p_depth);

// ----------------------------------------------------------------------------
//! \brief Is a pass open right now?
//!
//! What a draw call checks before agreeing to draw, since drawing outside a
//! pass goes somewhere nobody chose.
//! \return true when a pass is open.
// ----------------------------------------------------------------------------
[[nodiscard]] bool inRenderPass();

// ----------------------------------------------------------------------------
//! \brief Where the open pass is drawing, or a description of zero size when
//! none is open.
//! \return the innermost open pass, or an empty description.
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
