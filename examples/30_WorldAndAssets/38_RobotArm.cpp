// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/38_RobotArm.hpp"
#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"

#include <algorithm>
#include <cmath>
#include <functional>

namespace examples
{

namespace
{

//! \brief Where an unbounded (continuous) joint slider stops, in degrees.
constexpr float FULL_TURN = 360.0f;

float degrees(units::angle::radian_t p_angle, float p_fallback)
{
    const float value = units::angle::degree_t(p_angle).to<float>();
    return std::isfinite(value) ? value : p_fallback;
}

float meters(units::length::meter_t p_offset, float p_fallback)
{
    const float value = p_offset.to<float>();
    return std::isfinite(value) ? value : p_fallback;
}

} // namespace

std::string RobotArm::description() const
{
    return "An ABB IRB 2400 loaded from its URDF file: one entity per link, a "
           "revolute joint between each. The sliders of Try it set the joint "
           "angles within their limits, and the World computes where every "
           "link and the tool end up: forward kinematics. Right drag turns "
           "around the robot. Needs irb2400.urdf in "
           "external/Compages-data/.";
}

compages::Status RobotArm::setUp()
{
    const std::string path = dataPath("irb2400.urdf");
    if (path.empty())
    {
        return compages::failure(
            "irb2400.urdf is missing: run make download in "
            "external/, or set COMPAGES_DATA_PATH");
    }
    m_scene.background(0.12f, 0.14f, 0.18f);
    auto robot = m_scene.load(path);
    if (!robot)
    {
        return compages::failure(robot.error());
    }

    // The joints in the order of the chain, for the sliders.
    std::function<void(compages::world::Entity)> collect =
        [&](compages::world::Entity p_link)
    {
        if (p_link.has<compages::world::RevoluteJoint>() ||
            p_link.has<compages::world::PrismaticJoint>())
        {
            m_joints.push_back(p_link);
        }
        if (p_link.name() == "tool0")
        {
            m_tool = p_link;
        }
        p_link.children(collect);
    };
    collect(robot.value());

    const compages::core::Vector3f middle = m_scene.frameAll();
    m_scene.activeCamera().add<compages::world::Orbit>(middle);
    return m_scene.prepare();
}

void RobotArm::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.draw(p_frame);
}

void RobotArm::controls()
{
    for (compages::world::Entity& joint : m_joints)
    {
        if (auto* revolute = joint.find<compages::world::RevoluteJoint>())
        {
            auto& position = revolute->state.position;
            float angle = degrees(position.value, 0.0f);
            if (ImGui::SliderFloat(joint.name().c_str(),
                                   &angle,
                                   degrees(position.min, -FULL_TURN),
                                   degrees(position.max, FULL_TURN),
                                   "%.1f deg"))
            {
                joint.angle(units::angle::degree_t(angle));
            }
        }
        else if (auto* prismatic =
                     joint.find<compages::world::PrismaticJoint>())
        {
            auto& position = prismatic->state.position;
            float offset = meters(position.value, 0.0f);
            if (ImGui::SliderFloat(joint.name().c_str(),
                                   &offset,
                                   meters(position.min, -1.0f),
                                   meters(position.max, 1.0f),
                                   "%.3f m"))
            {
                joint.offset(units::length::meter_t(offset));
            }
        }
    }

    if (ImGui::Button("Home"))
    {
        for (compages::world::Entity& joint : m_joints)
        {
            if (auto* revolute = joint.find<compages::world::RevoluteJoint>())
            {
                revolute->state.position.value = units::angle::radian_t(0.0);
                revolute->state.position.value =
                    revolute->state.position.clamped();
            }
            else if (auto* prismatic =
                         joint.find<compages::world::PrismaticJoint>())
            {
                prismatic->state.position.value = units::length::meter_t(0.0);
                prismatic->state.position.value =
                    prismatic->state.position.clamped();
            }
        }
    }

    if (m_tool)
    {
        const compages::core::Vector3f tool = m_tool.worldPosition();
        ImGui::Text("tool0 in the world: %.3f %.3f %.3f",
                    double(tool.x),
                    double(tool.y),
                    double(tool.z));
    }
}

} // namespace examples
