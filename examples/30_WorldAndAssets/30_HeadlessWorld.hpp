// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"
#include "Compages/World/Entity.hpp"

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
//! struct Position { compages::core::Vector3f value; };
//! struct Velocity { compages::core::Vector3f value; };
//!
//! m_world.entity("player").set(Position{}).set(Velocity{ { 1, 0, 0 } });
//!
//! m_world.each<Position, Velocity>([&](compages::world::Entity, Position& p,
//! Velocity& v)
//! {
//!     p.value += v.value * dt;
//! });
//! \endcode
//!
//! Nothing here needs a GPU: the same code runs in a test or on a server.
//! This example draws nothing and checks every frame that the systems ran.
// ****************************************************************************
class HeadlessWorld final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "30_HeadlessWorld";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
};

} // namespace examples
