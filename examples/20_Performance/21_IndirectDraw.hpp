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
//! \brief The CPU never asks how many points survived.
//!
//! A compute pass keeps the points that fall inside a circle and writes the
//! four words a drawIndirect command is. The draw reads that command from the
//! device: there is no read-back, and no count to keep in step on the CPU.
//!
//! \code
//! m_command.write(compages::gpu::DrawIndirectCommand{ 0u, 1u, 0u, 0u }, 0u);
//! // count = 0 COMPAGES_TRY(m_cull.dispatchItems(COUNT));        // counts the
//! survivors compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib |
//! compages::gpu::Barrier::Command); m_kept_points.drawIndirect(m_command); //
//! draws that many
//! \endcode
//!
//! 12 wrote particles and then drew all of them. Here the count itself is a
//! GPU result. Forgetting the command barrier is a race, the same way
//! forgetting the vertex barrier in 12 is.
// ****************************************************************************
class IndirectDraw: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "21_IndirectDraw";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Dot
    {
        compages::core::Vector2f position;
    };

    compages::gpu::Buffer<Dot> m_all;
    compages::gpu::Buffer<Dot> m_kept;
    compages::gpu::Buffer<compages::gpu::DrawIndirectCommand> m_command;
    compages::gpu::ComputeProgram m_cull;
    compages::gpu::Drawable m_kept_points;
};

} // namespace examples
