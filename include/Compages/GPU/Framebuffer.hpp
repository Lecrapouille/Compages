// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Draw into textures instead of onto the screen.
//!
//! The window is one place to draw. A framebuffer is another: one or more
//! textures wired together so that a pass writes into them instead of onto the
//! screen. What was drawn can then be read as a texture, which is how a blur, a
//! reflection, a shadow and every ping-pong simulation work.
//!
//! The textures are named, not owned. Destroying the framebuffer does not
//! destroy the images; destroying an image while a framebuffer still names it
//! makes the next pass fail with a sentence, rather than draw into a slot the
//! driver no longer has.
//!
//! What it holds:
//! - Framebuffer: the target.
//! - Attachment: one texture, at one level, as part of a target.
//! - liveFramebuffers(): to spot leaks.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/Handle.hpp"
#include "Compages/GPU/Texture.hpp"

#include <span>
#include <vector>

namespace compages::gpu
{

//! \brief Names a target made of textures.
using FramebufferHandle = Handle<struct FramebufferTag>;

// ****************************************************************************
//! \brief One texture, at one level of detail, used as a place to draw.
//!
//! \code
//! compages::gpu::Attachment albedo{ .texture = color.handle() };
//! compages::gpu::Attachment small{ .texture = color.handle(), .level = 2u };
//! \endcode
// ****************************************************************************
struct Attachment
{
    //! \brief The texture drawn into. Not owned: it must outlive the
    //! framebuffer, or the next pass into it fails.
    TextureHandle texture;
    //! \brief Which level of detail to draw into. Zero is the largest image.
    std::uint32_t level = 0u;
};

// ****************************************************************************
//! \brief A sheet of paper to draw on instead of the screen.
//!
//! The picture drawn there is not shown: it stays in its textures, ready to be
//! used by a later draw. That is how a mirror shows the room (draw the room
//! into a texture, then put the texture on the mirror), how a shadow knows
//! what the light sees, and how a blur reads the image it softens.
//!
//! \code
//! auto color = compages::gpu::Texture::create(
//!     compages::gpu::TextureDesc::image(512u, 512u)).take();
//! auto target = compages::gpu::Framebuffer::create(color).take();
//! {
//!     compages::gpu::RenderPass pass(target);   // draws go into color
//!     mesh.draw();
//! }
//! screen_quad["uImage"] = color;                // read back as a texture
//! \endcode
//!
//! The textures are named, not owned: destroying the framebuffer keeps them.
//! A Framebuffer can be moved but not copied.
// ****************************************************************************
class Framebuffer: private compages::NotCopiable
{
public:

    //! \brief No target: valid() is false until attach() succeeds.
    Framebuffer() = default;

    // ------------------------------------------------------------------------
    //! \brief Make a target from colour textures and, optionally, a depth one.
    //!
    //! \param[in] p_colors the colour attachments, in order, starting at zero.
    //! At least one unless a depth attachment is given, which is how a shadow
    //! pass writes distances and no colour.
    //! \param[in] p_depth a depth or depth-stencil texture, or an empty
    //! attachment to write no distances.
    //! \return the target, or why it could not be made: a texture that is gone,
    //! a colour texture used as depth, images of different sizes, or a
    //! combination the driver refuses.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Framebuffer>
    create(std::span<const Attachment> p_colors, Attachment p_depth = {});

    // ------------------------------------------------------------------------
    //! \brief A target of one colour texture, without depth: shapes drawn
    //! into it cannot hide each other, so draw them back to front or add a
    //! depth texture.
    //!
    //! \param[in] p_color a colour texture, level 0.
    //! \return the target, or why it could not be made.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Framebuffer> create(Texture const& p_color);

    // ------------------------------------------------------------------------
    //! \brief A target of one colour texture and one depth texture, the
    //! usual pair for drawing a 3D scene offscreen.
    //!
    //! \param[in] p_color a colour texture, level 0.
    //! \param[in] p_depth a texture with a depth format, of the same size.
    //! \return the target, or why it could not be made: sizes differ, or
    //! p_depth has no depth format.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Framebuffer> create(Texture const& p_color,
                                                    Texture const& p_depth);

    // ------------------------------------------------------------------------
    //! \brief Same as create(), into this object.
    //!
    //! \code
    //! compages::gpu::Texture color, depth;
    //! COMPAGES_TRY(color.allocate({ .width = 512u, .height = 512u }));
    //! COMPAGES_TRY(depth.allocate({ .format =
    //! compages::gpu::PixelFormat::Depth32F,
    //!                               .width = 512u, .height = 512u }));
    //! COMPAGES_TRY(m_target.attach(color, depth));
    //! \endcode
    //!
    //! \param[in] p_colors the colour attachments, in order, starting at zero.
    //! \param[in] p_depth a depth texture, or an empty attachment for none.
    //! \return why not, in which case this object is left as it was.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status attach(std::span<const Attachment> p_colors,
                                Attachment p_depth = {});

    //! \brief Same as attach() with one colour texture.
    //! \param[in] p_color a colour texture, level 0.
    //! \return why not, in which case this object is left as it was.
    [[nodiscard]] Status attach(Texture const& p_color);

    //! \brief Same as attach() with one colour texture and one depth texture.
    //! \param[in] p_color a colour texture, level 0.
    //! \param[in] p_depth a depth texture of the same size.
    //! \return why not, in which case this object is left as it was.
    [[nodiscard]] Status attach(Texture const& p_color, Texture const& p_depth);

    //! \brief Take over the target of p_other, which is left not valid().
    //! \param[in,out] p_other the framebuffer to empty.
    Framebuffer(Framebuffer&& p_other) noexcept;
    //! \brief Release this target, then take over p_other's.
    //! \param[in,out] p_other the framebuffer to empty.
    //! \return this object, now owning the other target.
    Framebuffer& operator=(Framebuffer&& p_other) noexcept;
    //! \brief Release the target; its textures stay.
    ~Framebuffer();

    //! \brief Release the target now; its textures stay and can be attached
    //! again. Safe on an empty framebuffer.
    void release();

    //! \brief False when default constructed, moved from or released.
    //! \return true when the target can be drawn into.
    [[nodiscard]] bool valid() const;
    //! \brief Width in pixels of the attached images, 0 when not valid().
    //! \return the width in pixels.
    [[nodiscard]] std::uint32_t width() const;
    //! \brief Height in pixels of the attached images, 0 when not valid().
    //! \return the height in pixels.
    [[nodiscard]] std::uint32_t height() const;
    //! \brief How many colour textures are attached, what a fragment shader
    //! with several outputs writes to.
    //! \return the number of colour textures, 0 when not valid().
    [[nodiscard]] std::size_t colorCount() const;
    //! \brief The colour texture at an output slot, to read what was drawn.
    //! \param[in] p_index the slot, as in layout(location = N) out.
    //! \return an empty handle when the slot does not exist.
    [[nodiscard]] TextureHandle color(std::size_t p_index = 0u) const;
    //! \brief The depth texture, empty when there is none. A shadow pass reads
    //! the distances from here.
    //! \return the depth texture, or an empty handle.
    [[nodiscard]] TextureHandle depth() const;
    //! \brief Is there a depth texture? Without one, depth_test in a
    //! RenderState has nothing to compare against.
    //! \return true when a depth texture is attached.
    [[nodiscard]] bool hasDepth() const;

    //! \brief The handle naming this target, for code that keeps it without
    //! owning it.
    //! \return the handle; empty when not valid().
    [[nodiscard]] FramebufferHandle handle() const
    {
        return m_handle;
    }

private:

    explicit Framebuffer(FramebufferHandle p_handle) : m_handle(p_handle) {}

    FramebufferHandle m_handle;
};

//! \brief How many framebuffers exist right now, to spot the ones never
//! released.
//! \return the number of live framebuffers.
[[nodiscard]] std::size_t liveFramebuffers();

} // namespace compages::gpu
