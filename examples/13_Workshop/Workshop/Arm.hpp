// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The robot arm of the workshop, in the World only: its joints and
//! the routine that moves them. It needs no GPU and no Scene, so a test or
//! a server can run it (doc/tutorials/03-the-world.md); the Scene gives it
//! shapes later (Workshop.hpp).
// ****************************************************************************

#include "Compages/World/Entity.hpp"

namespace examples::workshop
{

//! \brief The height of the top of the table, in metres.
inline constexpr float TABLE_TOP = 0.78f;
//! \brief The lengths of the two links: shoulder to elbow, elbow to the tip.
inline constexpr float UPPER_ARM = 0.5f;
inline constexpr float FOREARM = 0.46f;

// ----------------------------------------------------------------------------
//! \brief The joints of the arm, from its base to its tool. Each is the child
//! of the one before, so turning one moves everything after it.
// ----------------------------------------------------------------------------
struct Arm
{
    compages::world::Entity base;     //!< on the table, turns round the vertical
    compages::world::Entity shoulder; //!< tilts the upper arm
    compages::world::Entity elbow;    //!< bends the forearm
    compages::world::Entity tool;     //!< the hot tip, at the end of the forearm
};

// ----------------------------------------------------------------------------
//! \brief Make the four joints in \c p_world, on the table, and attach the
//! routine that moves the tip along the table.
// ----------------------------------------------------------------------------
Arm buildArm(compages::world::World& p_world);

} // namespace examples::workshop
