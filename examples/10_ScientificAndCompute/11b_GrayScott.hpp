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
//! \brief The same ping-pong, with numbers that are not colours.
//!
//! Game of Life stored a bit per cell. This stores two concentrations as
//! 32-bit floats, because a reaction-diffusion is an equation, not a rule.
//! RGBA32F is what says so: the texture holds values, and the display pass
//! turns them into a picture afterwards.
//!
//! Two chemicals, U and V. U is fed, V is killed, and where they meet they
//! make more V. The patterns that grow from a seed in the middle are why
//! people keep writing this equation.
// ****************************************************************************
class GrayScott: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11b_GrayScott";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::gpu::Texture m_field[2];
    compages::gpu::Framebuffer m_target[2];
    int m_current = 0;

    compages::gpu::Drawable m_step;
    compages::gpu::Drawable m_show;
};

} // namespace examples
