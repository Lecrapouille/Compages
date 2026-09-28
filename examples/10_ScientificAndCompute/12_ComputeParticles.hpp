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
//! compages::gpu::barrier() is what makes the writes visible to the draw. Forgetting it
//! is a race, not a compile error.
// ****************************************************************************
class ComputeParticles: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "12_ComputeParticles";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    //! \brief The same bytes as the GLSL struct, std430 rules.
    struct Particle
    {
        Vector2f position;
        Vector2f velocity;
        Vector4f color;
    };

    compages::gpu::Buffer<Particle> m_particles;
    compages::gpu::ComputeProgram m_step;
    compages::gpu::Drawable m_points;
};

} // namespace examples
