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
//! \brief A curve that grows a point per step, and only the new points travel.
//!
//! HeightMap rewrote every vertex. This one appends: emplace_back() marks the
//! new end of the vertices, so the next draw sends the points added since the
//! last one rather than the whole trail:
//! \code
//! m_trail.emplace_back(Vertex{ point, color });
//! m_trail.draw();                   // a line strip through every point
//! \endcode
//!
//! The GPU buffer grows by doubling, so twenty thousand points cost about
//! fifteen reallocations, not twenty thousand.
// ****************************************************************************
class Lorenz: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "15_Lorenz";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Vertex
    {
        compages::core::Vector3f position;
        compages::core::Vector3f color;
    };

    //! \brief One step of the attractor, appended to the trail.
    void step();

    compages::gpu::Drawable m_trail;
    compages::core::Vector3f m_state{ 0.1f, 0.0f, 0.0f };
};

} // namespace examples
