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
//! \brief The same buffer, written by a compute shader and drawn as points.
//!
//! Game of Life computed in a texture because a fragment shader can only write
//! the pixel it covers. A compute shader writes wherever it wants, so the
//! particles live in a storage buffer that the compute shader moves and the
//! drawable reads as its vertices. There is no copy and no trip to the CPU:
//! \code
//! m_points.vertices(m_particles);           // once: read from this buffer
//! ...
//! COMPAGES_TRY(m_step.dispatchItems(COUNT));
//! compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);
//! m_points.draw();
//! \endcode
//!
//! compages::gpu::barrier() is what makes the writes visible to the draw.
//! Forgetting it is a race, not a compile error.
// ****************************************************************************
class ComputeParticles: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "12_ComputeParticles";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    //! \brief The same bytes as the GLSL struct, std430 rules.
    struct Particle
    {
        compages::core::Vector2f position;
        compages::core::Vector2f velocity;
        compages::core::Vector4f color;
    };

    compages::gpu::Buffer<Particle> m_particles;
    compages::gpu::ComputeProgram m_step;
    compages::gpu::Drawable m_points;
};

} // namespace examples
