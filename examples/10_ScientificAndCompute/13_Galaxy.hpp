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
//! \brief N-body gravity, tiled shared memory, two buffers that swap.
//!
//! Particles bounced against walls they already knew. These stars pull on
//! every other star, which is why the work is done in tiles loaded into
//! shared memory: each work group reads a slice once and reuses it, the
//! way GPU Gems 3 taught.
//!
//! A PingPong is two storage buffers: this frame's output is next frame's
//! input. The drawable is told each frame which of the two to read, which
//! costs nothing:
//! \code
//! m_stars.swap();
//! m_points.vertices(m_stars.input());
//! m_points.draw();
//! \endcode
// ****************************************************************************
class Galaxy: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "13_Galaxy";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Star
    {
        compages::core::Vector2f position;
        compages::core::Vector2f velocity;
        compages::core::Vector4f color;
    };

    compages::gpu::PingPong<Star> m_stars;
    compages::gpu::ComputeProgram m_step;
    compages::gpu::Drawable m_points;
};

} // namespace examples
