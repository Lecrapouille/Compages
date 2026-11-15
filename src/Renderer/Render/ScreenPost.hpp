// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/Drawable.hpp"
#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Components/ScreenEffects.hpp"
#include "Compages/Renderer/Render/RenderSnapshot.hpp"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief The screen effects of a view (ScreenEffects): the linear picture
//! the view is drawn into, the filters, the bloom, and the last pass that
//! gives the tone curve and the screen colours. Used by the Scene only.
//!
//! \code
//! compages::gpu::Framebuffer const* linear = nullptr;
//! COMPAGES_TRY_ASSIGN(linear, post.target(width, height));
//! { RenderPass pass(*linear, ...); draw the view with linear_target = true; }
//! post.finish(effects, snapshot);   // into the pass open on the screen
//! \endcode
// ****************************************************************************
class ScreenPost
{
public:

    //! \brief The linear picture (RGBA16F, Depth32F) a view of this size is
    //! drawn into. Kept from one frame to the next while the size stays.
    [[nodiscard]] Result<compages::gpu::Framebuffer const*> target(std::uint32_t p_width, std::uint32_t p_height);

    //! \brief The light that the tone curve turns into the colour p_shown: a
    //! background keeps its colour with the effects on.
    [[nodiscard]] static compages::Vector4f linearClear(compages::Vector4f p_shown, ToneMapping p_tone);

    //! \brief Run the filters and the bloom on the linear picture, then draw
    //! it into the pass open now, with the tone curve and the sRGB encoding.
    [[nodiscard]] Status finish(ScreenEffects const& p_effects, RenderSnapshot const& p_view);

private:

    //! \brief A colour picture with its framebuffer.
    struct Picture
    {
        compages::gpu::Texture color;
        compages::gpu::Framebuffer framebuffer;
        std::uint32_t width = 0u;
        std::uint32_t height = 0u;
    };

    [[nodiscard]] static Status make(Picture& p_picture, std::uint32_t p_width, std::uint32_t p_height);
    [[nodiscard]] Status ensurePrograms();
    [[nodiscard]] Result<compages::gpu::Drawable*> filter(ScreenFilter const& p_filter);
    [[nodiscard]] Status bloom(Bloom const& p_bloom, compages::gpu::Texture const& p_source);

    //! The picture the view is drawn into.
    compages::gpu::Texture m_color;
    compages::gpu::Texture m_depth;
    compages::gpu::Framebuffer m_framebuffer;
    std::uint32_t m_width = 0u;
    std::uint32_t m_height = 0u;
    //! Two pictures the filters pass from one to the other.
    std::array<Picture, 2> m_turns;
    //! The bloom: half the size, then half again.
    std::vector<Picture> m_levels;
    std::size_t m_level_count = 0u;
    compages::gpu::Drawable m_down;
    compages::gpu::Drawable m_up;
    compages::gpu::Drawable m_present;
    //! One program per filter text, compiled the first time it is used.
    std::unordered_map<std::string, compages::gpu::Drawable> m_filters;
    bool m_ready = false;
};

} // namespace compages::renderer
