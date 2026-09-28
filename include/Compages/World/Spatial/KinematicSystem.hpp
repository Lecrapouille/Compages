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

#include "Compages/World/Entt.hpp"

namespace compages::world
{

class TransformStore;

// ****************************************************************************
//! \brief Turns the joints of kinematic chains into local transforms.
//!
//! For every RevoluteJoint and PrismaticJoint it writes the LocalTransform of
//! the entity from the joint origin and its clamped position, and marks the
//! entity dirty only when that transform changed. It runs before the
//! TransformSystem, which then propagates the world matrices down the chain.
//!
//! Stateless, like the TransformSystem.
//!
//! \code
//! compages::world::KinematicSystem kin;
//! kin.update(registry, world.transforms());
//! \endcode
// ****************************************************************************
class KinematicSystem
{
public:

    //! \brief Apply every revolute/prismatic joint to its entity's local TRS.
    //! \param[in] p_registry EnTT registry holding joint components.
    //! \param[in,out] p_transforms store to write and mark dirty.
    void update(entt::registry& p_registry, TransformStore& p_transforms) const;
};

} // namespace compages::world
