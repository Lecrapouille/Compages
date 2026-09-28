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

#include "Compages/World/World.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A World on its own: entities, components and systems, no drawing.
//!
//! The World is an ECS in the flecs way. A component is any plain struct; an
//! entity is made, named, and given components in one chain; a system is a
//! loop over the entities having some components:
//! \code
//! struct Position { Vector3f value; };
//! struct Velocity { Vector3f value; };
//!
//! m_world.entity("player").set(Position{}).set(Velocity{ { 1, 0, 0 } });
//!
//! m_world.each<Position, Velocity>([&](compages::world::Entity, Position& p, Velocity& v)
//! {
//!     p.value += v.value * dt;
//! });
//! \endcode
//!
//! Nothing here needs a GPU: the same code runs in a test or on a server.
//! This example draws nothing and checks every frame that the systems ran.
// ****************************************************************************
class HeadlessWorld final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "30_HeadlessWorld";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
};

} // namespace examples
