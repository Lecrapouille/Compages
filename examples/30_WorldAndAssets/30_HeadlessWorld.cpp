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

#include "30_WorldAndAssets/30_HeadlessWorld.hpp"
#include "Compages/World/Entity.hpp"

namespace examples
{

struct Position
{
    compages::core::Vector3f value{ 0.0f, 0.0f, 0.0f };
};

struct Velocity
{
    compages::core::Vector3f value{ 0.0f, 0.0f, 0.0f };
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

//------------------------------------------------------------------------------
std::string HeadlessWorld::description() const
{
    return "A World without a Scene: entities made in one chain, a system "
           "moving what has a Position and a Velocity, a behavior counting "
           "frames. Nothing is drawn; the frame fails if a system did not run.";
}

//------------------------------------------------------------------------------
compages::Status HeadlessWorld::setUp()
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
               ? compages::success()
               : compages::failure("the worker was not found under the root");
}

//------------------------------------------------------------------------------
void HeadlessWorld::draw(compages::world::ViewFrame const& p_frame)
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
        compages::gpu::reportError(
            "the movement system did not move the player");
    }
    if (m_world.behaviorCount(m_world.lookup("Simulation/Worker")) != 1u)
    {
        compages::gpu::reportError("the worker lost its behavior");
    }
}

} // namespace examples
