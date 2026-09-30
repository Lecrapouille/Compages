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

#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Texture.hpp"

#include <span>
#include <vector>

// ****************************************************************************
//! \file
//! \brief A picture that can be drawn into, rather than shown.
//!
//! The window is one target. A framebuffer is another: one or more textures
//! wired together so that a pass writes into them instead of onto the screen.
//! What was drawn can then be read as a texture, which is how a blur, a
//! reflection, a shadow and every ping-pong simulation work.
//!
//! The textures are named, not owned. Destroying the framebuffer does not
//! destroy the images; destroying an image while a framebuffer still names it
//! makes the next pass fail with a sentence, rather than draw into a slot the
//! driver no longer has.
// ****************************************************************************

namespace compages::gpu
{

//! \brief Names a target made of textures.
using FramebufferHandle = Handle<struct FramebufferTag>;

// ****************************************************************************
//! \brief One texture, at one level of detail, used as a place to draw.
// ****************************************************************************
struct Attachment
{
    TextureHandle texture;
    //! \brief Which level of detail to draw into. Zero is the largest image.
    std::uint32_t level = 0u;
};

// ****************************************************************************
//! \brief Textures wired together as a place to draw.
// ****************************************************************************
class Framebuffer
{
public:

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
    //! \brief A target of one colour texture.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Framebuffer> create(Texture const& p_color);

    // ------------------------------------------------------------------------
    //! \brief A target of one colour texture and one depth texture.
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
    //! \return why not, in which case this object is left as it was.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status attach(std::span<const Attachment> p_colors,
                                Attachment p_depth = {});

    //! \brief Same as attach() with one colour texture.
    [[nodiscard]] Status attach(Texture const& p_color);

    //! \brief Same as attach() with one colour texture and one depth texture.
    [[nodiscard]] Status attach(Texture const& p_color, Texture const& p_depth);

    Framebuffer(Framebuffer&& p_other) noexcept;
    Framebuffer& operator=(Framebuffer&& p_other) noexcept;
    Framebuffer(Framebuffer const&) = delete;
    Framebuffer& operator=(Framebuffer const&) = delete;
    ~Framebuffer();

    void release();

    [[nodiscard]] bool valid() const;
    [[nodiscard]] std::uint32_t width() const;
    [[nodiscard]] std::uint32_t height() const;
    [[nodiscard]] std::size_t colorCount() const;
    [[nodiscard]] TextureHandle color(std::size_t p_index = 0u) const;
    [[nodiscard]] TextureHandle depth() const;
    [[nodiscard]] bool hasDepth() const;

    [[nodiscard]] FramebufferHandle handle() const
    {
        return m_handle;
    }

private:

    explicit Framebuffer(FramebufferHandle p_handle) : m_handle(p_handle) {}

    FramebufferHandle m_handle;
};

[[nodiscard]] std::size_t liveFramebuffers();

} // namespace compages::gpu
