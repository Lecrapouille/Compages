// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A scene drawn into a framebuffer as big as the window, then shown
//! through a wavy full screen effect.
//!
//! The framebuffer follows the size of the window: when it changes, the
//! textures are allocated again in place, and the drawables sampling them
//! keep reading the right ones since they refer to the texture objects, not
//! to what the device made of them.
// ****************************************************************************
class PostProcess: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "05c_PostProcess";
    }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    [[nodiscard]] compages::Status ensureTarget(std::uint32_t p_width,
                                                std::uint32_t p_height);

    //! \brief Declared before the drawables sampling them, so destroyed after.
    compages::gpu::Texture m_color_target;
    compages::gpu::Texture m_depth_target;
    compages::gpu::Framebuffer m_fbo;
    compages::gpu::Texture m_crate_texture;
    compages::gpu::Texture m_floor_texture;
    compages::gpu::Drawable m_cube;
    compages::gpu::Drawable m_floor;
    compages::gpu::Drawable m_screen;
    std::uint32_t m_target_width = 0u;
    std::uint32_t m_target_height = 0u;
};

} // namespace examples
