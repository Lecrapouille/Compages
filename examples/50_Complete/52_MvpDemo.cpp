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

#include "50_Complete/52_MvpDemo.hpp"
#include "Compages/World/Entity.hpp"

#include "Compages/Core/AABB.hpp"

#include <array>
#include <cmath>

namespace examples
{

//! \brief What a unit box covers, around its own origin.
const compages::core::AABB UNIT =
    compages::core::AABB::fromCorners({ -0.5f, -0.5f, -0.5f },
                                      { 0.5f, 0.5f, 0.5f });

//! \brief Falls, and bounces on the floor losing a part of its speed, then
//! starts again from where it was.
struct Bounce: compages::world::Behavior
{
    void start() override
    {
        from = entity().position();
    }

    void update(float p_dt) override
    {
        // Gravity, then a bounce that loses speed. Once it barely moves, it
        // is put back where it started.
        compages::core::Vector3f position = entity().position();
        speed -= 9.81f * p_dt;
        position.y += speed * p_dt;
        if (position.y < 0.5f)
        {
            position.y = 0.5f;
            speed = -0.7f * speed;
            if (std::abs(speed) < 0.5f)
            {
                position = from;
                speed = 0.0f;
            }
        }
        entity().position(position).rotate(Radians(p_dt), { 1.0f, 0.0f, 0.0f });
    }

    compages::core::Vector3f from;
    float speed = 0.0f;
};

//! \brief Goes round the vertical axis, at its height and its distance.
struct Circle: compages::world::Behavior
{
    void update(float) override
    {
        // A circle at a fixed height, from the total time.
        const float angle = 0.8f * frame().total;
        entity().position(6.0f * std::cos(angle), 6.0f, 6.0f * std::sin(angle));
    }
};

//! \brief Turns on itself.
struct MvpDemo::Spin: compages::world::Behavior
{
    void update(float p_dt) override
    {
        transform().rotateY(Radians(0.9f * p_dt));
    }
};

std::string MvpDemo::description() const
{
    return "Falling cubes bouncing on a floor, a spinning block and a lamp "
           "going round, all driven by behaviors, with debug boxes drawn "
           "around the cubes.";
}

compages::Status MvpDemo::setUp()
{
    // A sun, and a lamp that goes round on its own.
    m_scene.background(0.04f, 0.06f, 0.10f).ambient(0.08f, 0.09f, 0.12f);
    m_scene.camera()
        .position(0.0f, 12.0f, 28.0f)
        .add<compages::world::Orbit>(
            compages::core::Vector3f(0.0f, 2.0f, 0.0f));
    m_scene.sun("Sun", { 1.0f, 0.96f, 0.88f }, 0.8f);
    m_scene.lamp("Lamp", { 1.0f, 0.85f, 0.5f }, 2.0f, 20.0f).add<Circle>();

    // The floor is a thin box. The spinner and the falling cubes sit on it.
    m_scene.box("Floor", compages::renderer::color(0.2f, 0.5f, 0.95f))
        .position(0.0f, -0.5f, 0.0f)
        .scale(40.0f, 1.0f, 40.0f);
    m_scene.box("Spinner", compages::renderer::color(0.2f, 0.5f, 0.95f))
        .position(0.0f, 1.0f, -6.0f)
        .add<Spin>();

    const std::array<compages::core::Vector3f, 4u> starts{
        compages::core::Vector3f(-4.0f, 8.0f, -2.0f),
        compages::core::Vector3f(0.0f, 10.0f, 1.0f),
        compages::core::Vector3f(3.0f, 12.0f, -1.0f),
        compages::core::Vector3f(-2.0f, 14.0f, 3.0f)
    };
    for (compages::core::Vector3f const& start : starts)
    {
        m_cubes.emplace_back(
            m_scene.box("Cube", compages::renderer::color(0.9f, 0.2f, 0.2f))
                .position(start)
                .add<Bounce>());
    }
    return m_scene.prepare();
}

void MvpDemo::draw(compages::world::ViewFrame const& p_frame)
{
    // Moved first, so that the boxes are drawn where the cubes are now.
    m_scene.update(p_frame);
    for (compages::world::Entity& cube : m_cubes)
    {
        m_scene.debug().box(UNIT, cube.worldMatrix(), { 1.0f, 0.8f, 0.2f });
    }
    m_scene.render();
}

} // namespace examples
