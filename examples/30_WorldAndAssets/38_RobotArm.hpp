// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
