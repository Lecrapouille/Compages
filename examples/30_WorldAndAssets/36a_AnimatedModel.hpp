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

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/Renderer/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Two walkers going round, their gait made by a behavior.
//!
//! The same pattern as 31_MovingRobot, pushed to a walk cycle: hips and
//! shoulders are joints, the limbs hang under them, and a Walk behavior
//! swings them in opposite phase while it moves its walker along a circle.
//! One class of behavior, two walkers, each with its own radius and phase:
//! \code
//! makeWalker("A", blue).add<Walk>(3.1f, 0.0f);
//! makeWalker("B", red).add<Walk>(2.2f, 1.7f);
//! \endcode
// ****************************************************************************
class AnimatedModel final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "36a_AnimatedModel";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    struct Walk;

    compages::world::Entity makeWalker(char const* p_name, compages::renderer::Look const& p_shirt);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
