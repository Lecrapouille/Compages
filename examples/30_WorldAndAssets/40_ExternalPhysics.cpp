// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/40_ExternalPhysics.hpp"

#include "Compages/World/Components/PoseDriver.hpp"
#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/World/Entity.hpp"

#include "Common/Gui.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

namespace
{

constexpr std::size_t BALL_COUNT = 12u;

} // namespace

void ExternalPhysics::TinyEngine::step(float p_dt)
{
    // Gravity, a bounce on the floor and on the four walls, then the balls
    // against each other. A real engine does far more; what matters here is
    // that it owns its bodies.
    for (Body& body : bodies)
    {
        body.velocity[1] += gravity * p_dt;
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            body.position[axis] += body.velocity[axis] * p_dt;
        }
        if (body.position[1] < body.radius)
        {
            body.position[1] = body.radius;
            body.velocity[1] = -body.velocity[1] * bounce;
        }
        for (std::size_t axis : { 0u, 2u })
        {
            const float limit = half_box - body.radius;
            if (std::abs(body.position[axis]) > limit)
            {
                body.position[axis] = std::clamp(body.position[axis], -limit, limit);
                body.velocity[axis] = -body.velocity[axis] * bounce;
            }
        }
    }
    collide();
}

void ExternalPhysics::TinyEngine::collide()
{
    // Every pair of balls, same mass: push the two apart until they touch,
    // then exchange the speeds along the line between their centres.
    for (std::size_t i = 0u; i < bodies.size(); ++i)
    {
        for (std::size_t j = i + 1u; j < bodies.size(); ++j)
        {
            Body& a = bodies[i];
            Body& b = bodies[j];
            std::array<float, 3> normal{};
            float distance2 = 0.0f;
            for (std::size_t k = 0u; k < 3u; ++k)
            {
                normal[k] = b.position[k] - a.position[k];
                distance2 += normal[k] * normal[k];
            }
            const float touch = a.radius + b.radius;
            if ((distance2 >= touch * touch) || (distance2 < 1.0e-12f))
            {
                continue;
            }
            const float distance = std::sqrt(distance2);
            float closing = 0.0f;
            for (std::size_t k = 0u; k < 3u; ++k)
            {
                normal[k] /= distance;
                const float push = 0.5f * (touch - distance) * normal[k];
                a.position[k] -= push;
                b.position[k] += push;
                closing += (a.velocity[k] - b.velocity[k]) * normal[k];
            }
            if (closing <= 0.0f)
            {
                continue; // already moving apart
            }
            const float impulse = 0.5f * (1.0f + bounce) * closing;
            for (std::size_t k = 0u; k < 3u; ++k)
            {
                a.velocity[k] -= impulse * normal[k];
                b.velocity[k] += impulse * normal[k];
            }
        }
    }
}

std::string ExternalPhysics::description() const
{
    return "Compages has no physics engine: a small engine of the demo moves "
           "the balls (gravity, floor, walls, balls against balls), and writes their places into entities marked "
           "PoseDriver::External. Compages only computes the world matrices "
           "and draws. Right drag turns the camera.";
}

void ExternalPhysics::throwBalls()
{
    m_engine.bodies.clear();
    for (std::size_t i = 0u; i < BALL_COUNT; ++i)
    {
        // Spread on a circle, thrown up and outward at different speeds.
        const float turn = 6.2831853f * float(i) / float(BALL_COUNT);
        const float speed = 1.0f + (0.25f * float(i % 4u));
        m_engine.bodies.push_back(TinyEngine::Body{
            .position = { std::cos(turn), 1.0f + (0.3f * float(i)), std::sin(turn) },
            .velocity = { speed * std::cos(turn), 2.0f, speed * std::sin(turn) },
            .radius = 0.25f });
    }
}

compages::Status ExternalPhysics::setUp()
{
    m_scene.background(0.05f, 0.06f, 0.09f).ambient(0.15f, 0.15f, 0.18f);
    m_scene.camera().position(0.0f, 4.0f, 9.0f).lookAt(0.0f, 1.0f, 0.0f)
        .add<compages::world::Orbit>();
    m_scene.sun();
    // The plane is built standing in XY: laid down, it is the floor y = 0.
    m_scene.plane("Floor", compages::renderer::color(0.3f, 0.32f, 0.36f))
        .rotation(-1.5707963f, { 1.0f, 0.0f, 0.0f })
        .scale(2.0f * m_engine.half_box);

    throwBalls();
    for (std::size_t i = 0u; i < BALL_COUNT; ++i)
    {
        const float hue = float(i) / float(BALL_COUNT);
        compages::world::Entity ball = m_scene.sphere(
            "Ball", compages::renderer::color(0.4f + (0.6f * hue), 0.5f, 1.0f - (0.6f * hue)));
        ball.scale(2.0f * m_engine.bodies[i].radius);
        // The engine writes the pose: no Compages system may.
        ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });
        m_balls.push_back(ball);
    }
    return m_scene.prepare();
}

void ExternalPhysics::draw(compages::world::ViewFrame const& p_frame)
{
    // 1. The engine moves its bodies, with a small fixed step for stability.
    const float dt = std::min(p_frame.elapsed, 1.0f / 30.0f);
    m_engine.step(dt);

    // 2. Its result goes into the entities.
    for (std::size_t i = 0u; i < m_balls.size(); ++i)
    {
        auto const& p = m_engine.bodies[i].position;
        m_balls[i].position(p[0], p[1], p[2]);
    }

    // 3. Compages computes the world matrices and draws.
    m_scene.draw(p_frame);
}

void ExternalPhysics::controls()
{
    if (ImGui::Button("Throw again"))
    {
        throwBalls();
    }
    ImGui::SliderFloat("Bounce", &m_engine.bounce, 0.0f, 1.0f);
}

} // namespace examples
