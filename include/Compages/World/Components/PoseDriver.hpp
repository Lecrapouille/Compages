// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdint>

namespace compages::world
{

// ****************************************************************************
//! \brief Says who writes the local pose of an entity.
//!
//! Three writers can move an entity: the KinematicSystem (from its joint), the
//! AnimationSystem (from a clip) and code outside Compages (a physics engine,
//! a robot controller, a network). Without this component the rule is: a
//! joint wins over a clip, and outside code may always write. With it, only
//! the named writer touches the pose; the two other systems leave it alone.
//!
//! Compages has no physics engine. A physics engine marks its bodies
//! \c External and writes their pose into the entities at each step; the
//! TransformSystem then computes the world matrices and the renderer shows
//! them.
//!
//! \code
//! ball.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });
//! ball.position(engine.positionOf(body)); // each step, before world.update()
//! \endcode
// ****************************************************************************
struct PoseDriver
{
    enum Source : std::uint8_t
    {
        //! \brief The KinematicSystem writes the pose from the joint.
        Kinematic,
        //! \brief The AnimationSystem writes the pose from the playing clip.
        Animation,
        //! \brief Code outside Compages writes the pose; no system does.
        External,
    };

    Source source = Kinematic;
};

//! \brief May this writer change the pose of an entity with this driver?
//!
//! \param[in] p_driver the PoseDriver of the entity, or null when it has none.
//! \param[in] p_writer the system asking.
[[nodiscard]] constexpr bool mayWrite(PoseDriver const* p_driver,
                                      PoseDriver::Source p_writer)
{
    return (p_driver == nullptr) || (p_driver->source == p_writer);
}

} // namespace compages::world
