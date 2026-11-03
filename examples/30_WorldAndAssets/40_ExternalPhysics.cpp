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

//! \brief Below this speed, a touch is resting, not a shock.
constexpr float SHOCK_SPEED = 0.5f;

// ----------------------------------------------------------------------------
void ExternalPhysics::TinyEngine::hit(std::size_t p_a, int p_b, std::array<float, 3> p_normal, float p_speed)
{
    if (p_speed < SHOCK_SPEED)
    {
        return;
    }
    Body const& body = bodies[p_a];
    std::array<float, 3> point{};
    for (std::size_t k = 0u; k < 3u; ++k)
    {
        point[k] = body.position[k] - (body.radius * p_normal[k]);
    }
    hits.push_back(Hit{ p_a, p_b, point, p_normal, p_speed });
}

// ----------------------------------------------------------------------------
void ExternalPhysics::TinyEngine::step(float p_dt)
{
    // Gravity, a bounce on the floor and on the four walls, then the balls
    // against each other. A real engine does far more; what matters here is
    // that it owns its bodies, and reports its shocks.
    hits.clear();
    for (std::size_t i = 0u; i < bodies.size(); ++i)
    {
        Body& body = bodies[i];
        body.velocity[1] += gravity * p_dt;
        for (std::size_t axis = 0u; axis < 3u; ++axis)
        {
            body.position[axis] += body.velocity[axis] * p_dt;
        }
        if (body.position[1] < body.radius)
        {
            hit(i, -1, { 0.0f, 1.0f, 0.0f }, -body.velocity[1]);
            body.position[1] = body.radius;
            body.velocity[1] = -body.velocity[1] * bounce;
        }
        for (std::size_t axis : { 0u, 2u })
        {
            const float limit = half_box - body.radius;
            if (std::abs(body.position[axis]) > limit)
            {
                std::array<float, 3> normal{ 0.0f, 0.0f, 0.0f };
                normal[axis] = (body.position[axis] > 0.0f) ? -1.0f : 1.0f;
                hit(i, -1, normal, std::abs(body.velocity[axis]));
                body.position[axis] = std::clamp(body.position[axis], -limit, limit);
                body.velocity[axis] = -body.velocity[axis] * bounce;
            }
        }
    }
    collide();
}

// ----------------------------------------------------------------------------
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
            hit(i, int(j), { -normal[0], -normal[1], -normal[2] }, closing);
            const float impulse = 0.5f * (1.0f + bounce) * closing;
            for (std::size_t k = 0u; k < 3u; ++k)
            {
                a.velocity[k] -= impulse * normal[k];
                b.velocity[k] += impulse * normal[k];
            }
        }
    }
}

// ****************************************************************************
//! \brief Steps the engine at a fixed pace, gives its result to the entities,
//! and tells each ball about its shocks.
// ****************************************************************************
struct ExternalPhysics::Physics: compages::world::Routine
{
    Physics(TinyEngine& p_engine, std::vector<compages::world::Entity>& p_balls, compages::world::EntityId p_floor, int& p_shocks)
        : engine(p_engine), balls(p_balls), floor(p_floor), shocks(p_shocks)
    {
    }

    void fixedUpdate(float p_dt) override
    {
        // 1. The engine moves its bodies.
        engine.step(p_dt);

        // 2. Its result goes into the entities.
        for (std::size_t i = 0u; i < balls.size(); ++i)
        {
            auto const& p = engine.bodies[i].position;
            balls[i].position(p[0], p[1], p[2]);
        }

        // 3. Its shocks go to the routines of the bodies: Compages detects
        // no collision itself.
        for (TinyEngine::Hit const& hit : engine.hits)
        {
            const compages::Vector3f point(hit.point[0], hit.point[1], hit.point[2]);
            const compages::Vector3f normal(hit.normal[0], hit.normal[1], hit.normal[2]);
            const compages::world::EntityId other = (hit.b < 0) ? floor : balls[std::size_t(hit.b)].id();
            world().routines().send(balls[hit.a], compages::world::Contact{ other, point, normal, hit.speed });
            if (hit.b >= 0)
            {
                world().routines().send(other, compages::world::Contact{ balls[hit.a], point, -1.0f * normal, hit.speed });
            }
            ++shocks;
        }
    }

    TinyEngine& engine;
    std::vector<compages::world::Entity>& balls;
    compages::world::EntityId floor;
    int& shocks;
};

// ****************************************************************************
//! \brief Swells at a shock, as much as the shock is hard, then shrinks back.
// ****************************************************************************
struct ExternalPhysics::Squash: compages::world::Routine
{
    explicit Squash(float p_size) : size(p_size) {}

    void onMessage(compages::world::Message const& p_message) override
    {
        if (compages::world::Contact const* contact = p_message.as<compages::world::Contact>())
        {
            swell = std::max(swell, std::min(1.0f, contact->speed / 4.0f));
        }
    }

    void update(float p_dt) override
    {
        swell = std::max(0.0f, swell - (3.0f * p_dt));
        entity().scale(size * (1.0f + (0.6f * swell)));
    }

    float size;
    float swell = 0.0f;
};

// ----------------------------------------------------------------------------
std::string ExternalPhysics::description() const
{
    return "Compages has no physics engine: a small engine of the demo moves "
           "the balls (gravity, floor, walls, balls against balls). A Physics "
           "routine steps it in fixedUpdate(), writes the places into entities "
           "marked PoseDriver::External and sends a Contact message for each "
           "shock; a Squash routine on each ball swells at a shock. Right drag "
           "turns the camera.";
}

// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
compages::Status ExternalPhysics::setUp()
{
    m_scene.background(0.05f, 0.06f, 0.09f).ambient(0.15f, 0.15f, 0.18f);
    m_scene.camera().position(0.0f, 4.0f, 9.0f).lookAt(0.0f, 1.0f, 0.0f)
        .add<compages::world::Orbit>();
    m_scene.sun();
    // The plane is built standing in XY: laid down, it is the floor y = 0.
    compages::world::Entity floor =
        m_scene.plane("Floor", compages::renderer::color(0.3f, 0.32f, 0.36f))
            .rotation(-1.5707963f, { 1.0f, 0.0f, 0.0f })
            .scale(2.0f * m_engine.half_box);

    throwBalls();
    for (std::size_t i = 0u; i < BALL_COUNT; ++i)
    {
        const float hue = float(i) / float(BALL_COUNT);
        compages::world::Entity ball = m_scene.sphere(
            "Ball", compages::renderer::color(0.4f + (0.6f * hue), 0.5f, 1.0f - (0.6f * hue)));
        // The engine writes the pose: no Compages system may.
        ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External })
            .add<Squash>(2.0f * m_engine.bodies[i].radius);
        m_balls.push_back(ball);
    }
    m_world.entity("Physics").add<Physics>(m_engine, m_balls, floor.id(), m_shocks);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void ExternalPhysics::draw(compages::world::ViewFrame const& p_frame)
{
    // The Physics routine runs inside: fixedUpdate() at 60 Hz, whatever the
    // frame rate. Compages then computes the world matrices and draws.
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void ExternalPhysics::controls()
{
    if (ImGui::Button("Throw again"))
    {
        throwBalls();
    }
    ImGui::SliderFloat("Bounce", &m_engine.bounce, 0.0f, 1.0f);
    ImGui::Text("Shocks sent as messages: %d", m_shocks);
}

} // namespace examples
