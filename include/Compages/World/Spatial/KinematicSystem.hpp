// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
