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

#include "Compages/World/Entity.hpp"
#include "Common/Example.hpp"

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/Renderer/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Forward kinematics of an industrial robot loaded from a URDF file.
//!
//! load() builds one entity per link, with a RevoluteJoint between each link
//! and its parent. Setting an angle is all it takes to move the arm; the World
//! turns the joints into transforms at its next update:
//! \code
//! auto robot = m_scene.load(dataPath("irb2400.urdf"));
//! robot.value().lookup("base_link/link_1").angle(30.0_deg);
//! \endcode
// ****************************************************************************
class RobotArm final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "38_RobotArm";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;
    void controls() override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    //! \brief The jointed links, from the base to the tool.
    std::vector<compages::world::Entity> m_joints;
    compages::world::Entity m_tool;
};

} // namespace examples
