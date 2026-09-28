// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "30_WorldAndAssets/30_HeadlessWorld.hpp"

namespace examples
{

struct Position
{
    Vector3f value{ 0.0f, 0.0f, 0.0f };
};

struct Velocity
{
    Vector3f value{ 0.0f, 0.0f, 0.0f };
};

//! \brief How many frames a behavior has seen.
struct Ticks: compages::world::Behavior
{
    void update(float) override
    {
        ++count;
    }

    std::size_t count = 0u;
};

std::string HeadlessWorld::description() const
{
    return "A World without a Scene: entities made in one chain, a system "
           "moving what has a Position and a Velocity, a behavior counting "
           "frames. Nothing is drawn; the frame fails if a system did not run.";
}

compages::gpu::Status HeadlessWorld::setUp()
{
    // One chain per entity: a name, then the components it carries. The rock
    // has no velocity, so the movement system below never sees it.
    m_world.entity("player")
        .set(Position{})
        .set(Velocity{ { 1.0f, 0.0f, 0.0f } });
    m_world.entity("rock").set(Position{ { 0.0f, 5.0f, 0.0f } });

    // A hierarchy: the worker lives under the simulation root and is found
    // back by its path.
    compages::world::Entity root = m_world.entity("Simulation");
    root.child("Worker").add<Ticks>();
    return m_world.lookup("Simulation/Worker")
               ? compages::gpu::success()
               : compages::gpu::failure("the worker was not found under the root");
}

void HeadlessWorld::draw(Frame const& p_frame)
{
    compages::gpu::clear({ 0.05f, 0.05f, 0.07f });

    // A system is a loop: every entity having both components, and only them.
    m_world.each<Position, Velocity>(
        [&](compages::world::Entity, Position& p_position, Velocity& p_velocity)
        { p_position.value += p_velocity.value * p_frame.elapsed; });
    m_world.update(p_frame); // the behaviors, then the transforms

    // The gallery has no scene to look at, so the checks are the picture:
    // the player must have moved, and the worker must still have its behavior.
    compages::world::Entity player = m_world.lookup("player");
    if ((p_frame.total > 0.5f) && (player.get<Position>().value.x <= 0.0f))
    {
        compages::gpu::reportError("the movement system did not move the player");
    }
    if (m_world.behaviorCount(m_world.lookup("Simulation/Worker")) != 1u)
    {
        compages::gpu::reportError("the worker lost its behavior");
    }
}

} // namespace examples
