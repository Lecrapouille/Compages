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
//! \brief Compute in a texture, by drawing into another.
//!
//! Each cell is a pixel. A shader covering the texture reads the current
//! generation and writes the next into a second texture. The two then swap,
//! which is why this is called ping-pong: a shader cannot read the picture it
//! is writing, so the next generation has to live somewhere else.
//! \code
//! m_step["previous"] = m_field[m_current];
//! {
//!     compages::gpu::RenderPass into(m_target[next], { .clear_color = false
//!     }); m_step.draw(3u);
//! }
//! m_current = next;
//! \endcode
//!
//! This is how glumpy computed on the GPU, and it works with nothing but a
//! framebuffer. A compute shader does the same with less ceremony in
//! 12_ComputeParticles.
// ****************************************************************************
class GameOfLife: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11a_GameOfLife";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief Declared before the drawables sampling them, so destroyed after.
    compages::gpu::Texture m_field[2];
    compages::gpu::Framebuffer m_target[2];
    int m_current = 0;

    //! \brief Computes the next generation, one pixel per cell.
    compages::gpu::Drawable m_step;
    //! \brief Paints the current generation on the screen.
    compages::gpu::Drawable m_show;
};

} // namespace examples
