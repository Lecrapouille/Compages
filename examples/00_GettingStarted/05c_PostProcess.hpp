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

    [[nodiscard]] std::string name() const override { return "05c_PostProcess"; }
    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    [[nodiscard]] compages::gpu::Status ensureTarget(std::uint32_t p_width,
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
