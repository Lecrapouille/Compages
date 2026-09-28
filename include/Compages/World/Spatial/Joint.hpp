// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Units.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

#include <algorithm>
#include <limits>

// ****************************************************************************
//! \file
//! \brief The joints of a kinematic chain: what moves a link relative to its
//! parent link. A joint is a component of the child entity; the hierarchy
//! itself stays in the SpatialGraph.
//!
//! \code
//! compages::world::Entity base = world.entity("Base");
//! compages::world::Entity arm = base.child("Arm").position(0, 0, 0.5f)
//!                         .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
//! arm.angle(30.0_deg);
//! \endcode
//!
//! The KinematicSystem turns each joint into the LocalTransform of its entity
//! before the TransformSystem computes the world matrices:
//! \code
//! Local = Origin * Rotation(axis, q)        // RevoluteJoint
//! Local = Origin * Translation(axis * q)    // PrismaticJoint
//! \endcode
//! A fixed joint needs no component: it is the plain LocalTransform.
// ****************************************************************************

namespace compages::world
{

// ****************************************************************************
//! \brief A value and the interval it must stay in. Unbounded by default.
//! \tparam U a unit type of nholthaus/units, as units::angle::radian_t.
//!
//! \code
//! compages::world::Bounded<units::angle::radian_t> angle{ .value = 1.2_rad, .max = 90.0_deg };
//! angle.clamped();
//! \endcode
// ****************************************************************************
template <class U>
struct Bounded
{
    //! \brief Current joint coordinate (angle or offset).
    U value{ 0.0 };
    //! \brief Lower limit, or unbounded below.
    U min{ -std::numeric_limits<double>::infinity() };
    //! \brief Upper limit, or unbounded above.
    U max{ std::numeric_limits<double>::infinity() };

    //! \brief The value brought back inside [min, max].
    [[nodiscard]] U clamped() const
    {
        return std::clamp(value, min, max);
    }
};

// ****************************************************************************
//! \brief Position, velocity and acceleration of a joint: the (q, v, a) of
//! Pinocchio. Only the position moves the link; the two others are bounds and
//! state for whoever plans or controls the motion.
//!
//! \code
//! compages::world::RevoluteState s;
//! s.position.value = 30.0_deg;
//! s.velocity.max = 2.0_rad_s;
//! \endcode
// ****************************************************************************
template <class Pos, class Vel, class Acc>
struct JointState
{
    Bounded<Pos> position;
    Bounded<Vel> velocity;
    Bounded<Acc> acceleration;
};

//! \brief Angle, angular velocity and acceleration bounds for a revolute joint.
using RevoluteState =
    JointState<units::angle::radian_t,
               units::angular_velocity::radians_per_second_t,
               units::angular_acceleration::radians_per_second_squared_t>;

//! \brief Offset, linear velocity and acceleration bounds for a prismatic joint.
using PrismaticState =
    JointState<units::length::meter_t,
               units::velocity::meters_per_second_t,
               units::acceleration::meters_per_second_squared_t>;

// ****************************************************************************
//! \brief Turns its link around an axis of the joint frame. A URDF
//! "continuous" joint is a revolute joint without bounds.
//!
//! \code
//! entity.set(compages::world::RevoluteJoint{ .axis = { 0, 0, 1 } });
//! \endcode
// ****************************************************************************
struct RevoluteJoint
{
    //! \brief Place of the joint frame relative to the parent link, when the
    //! angle is zero.
    LocalTransform origin{};
    //! \brief Unit axis of rotation, in the joint frame.
    Vector3f axis{ 0.0f, 0.0f, 1.0f };
    RevoluteState state{};
};

// ****************************************************************************
//! \brief Slides its link along an axis of the joint frame.
//!
//! \code
//! slider.set(compages::world::PrismaticJoint{ .axis = { 1, 0, 0 } });
//! \endcode
// ****************************************************************************
struct PrismaticJoint
{
    //! \brief Place of the joint frame relative to the parent link, when the
    //! offset is zero.
    LocalTransform origin{};
    //! \brief Unit axis of translation, in the joint frame.
    Vector3f axis{ 0.0f, 0.0f, 1.0f };
    PrismaticState state{};
};

//! \brief The local place of the link, for the clamped angle of the joint.
//!
//! \param[in] p_joint Revolute joint state and origin.
//! \return TRS written into \c TransformStore by \c KinematicSystem.
[[nodiscard]] LocalTransform jointTransform(RevoluteJoint const& p_joint);

//! \brief The local place of the link, for the clamped offset of the joint.
//!
//! \param[in] p_joint Prismatic joint state and origin.
//! \return TRS written into \c TransformStore by \c KinematicSystem.
[[nodiscard]] LocalTransform jointTransform(PrismaticJoint const& p_joint);

} // namespace compages::world
