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
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::gpu::Texture m_field[2];
    compages::gpu::Framebuffer m_target[2];
    int m_current = 0;

    compages::gpu::Drawable m_step;
    compages::gpu::Drawable m_show;
};

} // namespace examples
