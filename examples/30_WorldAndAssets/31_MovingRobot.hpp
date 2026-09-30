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

#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Three robots of cubes: a hierarchy of transforms, posed by a
//! behavior.
//!
//! A robot is a tree. Its joints are empty entities placed where a part
//! turns, a shoulder or a neck, and the boxes hang under them; turning a
//! joint turns everything under it, and the head does not know it is on a
//! turning body:
//! \code
//! compages::world::Entity shoulder = body.child("LeftShoulder").position(-13,
//! 15, 0); m_scene.box("LeftArm", dark).parent(shoulder).position(0, -12,
//! 0).scale(6, 24, 6);
//! \endcode
//!
//! Only the boxes are scaled, never the joints: a scale on a joint would
//! squash every part under it. The motion is a behavior on the root of each
//! robot, written from the total time so that it never drifts.
// ****************************************************************************
class MovingRobot final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "31_MovingRobot";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Walk;

    void makeRobot(char const* p_name, float p_x, float p_phase);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
