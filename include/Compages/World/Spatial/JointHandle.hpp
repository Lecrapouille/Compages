// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/World/EntityId.hpp"
#include "Compages/World/Spatial/Joint.hpp"

#include <limits>

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief Handle on the joint of one entity: make it, set it, read it.
//!
//! Given by \c Entity::joint(). Like Entity, it is a small value (a World and
//! an id) that can be copied freely. The joint itself is a RevoluteJoint or a
//! PrismaticJoint component of the entity.
//!
//! \code
//! compages::world::Entity arm = base.child("Arm").position(0, 0, 0.5f)
//!                                   .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
//! arm.joint().angle(30.0_deg);
//! arm.joint().angle();          // applied, inside the limits
//! arm.joint().commandedAngle(); // as set
//! \endcode
// ****************************************************************************
class JointHandle
{
public:

    JointHandle(World& p_world, EntityId p_id) : m_world(&p_world), m_id(p_id) {}

    // ------------------------------------------------------------------------
    //! \brief Make the entity turn around an axis, within an interval of
    //! angles. Its current place becomes the place of the joint at zero. A
    //! zero axis makes a joint that does not move.
    // ------------------------------------------------------------------------
    JointHandle& revolute(compages::core::Vector3f const& p_axis,
                          units::angle::radian_t p_min = units::angle::radian_t(
                              -std::numeric_limits<double>::infinity()),
                          units::angle::radian_t p_max = units::angle::radian_t(
                              std::numeric_limits<double>::infinity()));

    // ------------------------------------------------------------------------
    //! \brief Make the entity slide along an axis, within an interval of
    //! offsets. Its current place becomes the place of the joint at zero.
    // ------------------------------------------------------------------------
    JointHandle& prismatic(compages::core::Vector3f const& p_axis,
                           units::length::meter_t p_min = units::length::meter_t(
                               -std::numeric_limits<double>::infinity()),
                           units::length::meter_t p_max = units::length::meter_t(
                               std::numeric_limits<double>::infinity()));

    //! \brief Does the entity have a joint (revolute or prismatic)?
    [[nodiscard]] bool exists() const
    {
        return origin() != nullptr;
    }

    //! \brief Set the commanded angle of the revolute joint.
    JointHandle& angle(units::angle::radian_t p_angle);
    //! \brief The angle that turns the link: the command kept inside the
    //! limits.
    [[nodiscard]] units::angle::radian_t angle() const;
    //! \brief The angle as last set, maybe outside the limits.
    [[nodiscard]] units::angle::radian_t commandedAngle() const;

    //! \brief Set the commanded offset of the prismatic joint.
    JointHandle& offset(units::length::meter_t p_offset);
    //! \brief The offset that slides the link: the command kept inside the
    //! limits.
    [[nodiscard]] units::length::meter_t offset() const;
    //! \brief The offset as last set, maybe outside the limits.
    [[nodiscard]] units::length::meter_t commandedOffset() const;

    // ------------------------------------------------------------------------
    //! \brief The place of the joint frame at zero, or nullptr without a
    //! joint. Entity::position(), rotation() and scale() change this one on a
    //! joint, since the joint overwrites the local place at each update.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransform* origin() const;

private:

    //! \brief The origin of a new joint: that of the joint being replaced, or
    //! the local place. Removes the joint being replaced.
    [[nodiscard]] LocalTransform takeOrigin() const;

    World* m_world;
    EntityId m_id;
};

} // namespace compages::world
