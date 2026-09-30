// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/31_MovingRobot.hpp"
#include "Compages/World/Entity.hpp"

#include <cmath>

namespace examples
{

//! \brief Turns the robot, glances its head and swings its arms, from the
//! total time rather than by adding a little each frame.
struct MovingRobot::Walk: compages::world::Behavior
{
    explicit Walk(float p_phase) : phase(p_phase) {}

    void start() override
    {
        head = entity().lookup("Body/Head");
        left = entity().lookup("Body/LeftShoulder");
        right = entity().lookup("Body/RightShoulder");
    }

    void update(float) override
    {
        // Posed from the total time, so a pause and a resume do not drift,
        // and the three robots stay out of step by their phase.
        const float t = frame().total + phase;
        const float swing = 0.95f * std::sin(3.2f * t);
        entity().rotation(0.6f * t, { 0.0f, 1.0f, 0.0f });
        head.rotation(0.5f * std::sin(1.4f * t), { 0.0f, 1.0f, 0.0f });
        left.rotation(swing, { 1.0f, 0.0f, 0.0f });
        right.rotation(-swing, { 1.0f, 0.0f, 0.0f });
    }

    float phase;
    compages::world::Entity head;
    compages::world::Entity left;
    compages::world::Entity right;
};

std::string MovingRobot::description() const
{
    return "Three robots made of boxes hung under joints: turning a joint "
           "turns every part under it. A Walk behavior on each robot poses "
           "its joints from the total time.";
}

void MovingRobot::makeRobot(char const* p_name, float p_x, float p_phase)
{
    const compages::renderer::Look wood =
        compages::renderer::color(0.62f, 0.42f, 0.24f);
    const compages::renderer::Look dark =
        compages::renderer::color(0.35f, 0.24f, 0.14f);
    const compages::renderer::Look light =
        compages::renderer::color(0.85f, 0.66f, 0.42f);

    // The joints: empty entities where the parts turn. The body sits on the
    // legs, so that the feet touch the ground.
    compages::world::Entity robot =
        m_world.entity(p_name).position(p_x, 0.0f, 0.0f);
    compages::world::Entity body =
        robot.child("Body").position(0.0f, 41.0f, 0.0f);
    compages::world::Entity head =
        body.child("Head").position(0.0f, 20.0f, 0.0f);
    compages::world::Entity left =
        body.child("LeftShoulder").position(-13.0f, 15.0f, 0.0f);
    compages::world::Entity right =
        body.child("RightShoulder").position(13.0f, 15.0f, 0.0f);

    // The boxes, hung under their joint. An arm hangs half its length below
    // its shoulder, so that it pivots there and not around its middle.
    m_scene.box("BodyMesh", wood).parent(body).scale(20.0f, 30.0f, 10.0f);
    m_scene.box("HeadMesh", light).parent(head).scale(10.0f);
    m_scene.box("LeftArm", dark)
        .parent(left)
        .position(0.0f, -12.0f, 0.0f)
        .scale(6.0f, 24.0f, 6.0f);
    m_scene.box("RightArm", dark)
        .parent(right)
        .position(0.0f, -12.0f, 0.0f)
        .scale(6.0f, 24.0f, 6.0f);
    m_scene.box("LeftLeg", dark)
        .parent(body)
        .position(-5.0f, -28.0f, 0.0f)
        .scale(6.0f, 26.0f, 6.0f);
    m_scene.box("RightLeg", dark)
        .parent(body)
        .position(5.0f, -28.0f, 0.0f)
        .scale(6.0f, 26.0f, 6.0f);

    robot.add<Walk>(p_phase);
}

compages::Status MovingRobot::setUp()
{
    // The orbit looks at the robots' chests, not at the origin under their
    // feet.
    m_scene.background(0.05f, 0.08f, 0.20f);
    m_scene.camera()
        .position(0.0f, 45.0f, 120.0f)
        .lookAt(0.0f, 30.0f, 0.0f)
        .add<compages::world::Orbit>(
            compages::core::Vector3f(0.0f, 30.0f, 0.0f));
    m_scene.sun();

    makeRobot("Robot1", -34.0f, 0.0f);
    makeRobot("Robot2", 0.0f, 0.7f);
    makeRobot("Robot3", 34.0f, 1.4f);
    return m_scene.prepare();
}

void MovingRobot::draw(compages::world::ViewFrame const& p_frame)
{
    // update() of the Walk behaviors runs inside the draw.
    m_scene.draw(p_frame);
}

} // namespace examples
