// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Arm.hpp"

#include <algorithm>
#include <cmath>

using namespace compages::si::literals;

namespace examples::workshop
{

namespace
{

//! \brief The height of the shoulder above the floor.
const compages::si::Length SHOULDER_HEIGHT(double(TABLE_TOP) + 0.12);

// ****************************************************************************
//! \brief Moves the tip of the arm along a path on the table: the base turns
//! slowly from side to side, the tip goes away and back, and sometimes lifts.
//!
//! The path is given for the tip, not for the joints. The routine finds the
//! angles of the shoulder and the elbow that put the tip there (inverse
//! kinematics of two links in the plane of the arm). So the tip never goes
//! into the table, whatever the path.
// ****************************************************************************
struct FollowPath: compages::world::Routine
{
    FollowPath(compages::world::Entity p_shoulder, compages::world::Entity p_elbow)
        : shoulder(p_shoulder), elbow(p_elbow)
    {
    }

    void update(compages::si::Seconds) override
    {
        using compages::si::Length;
        using compages::si::Radians;
        const compages::si::Seconds t = frame().total;

        // Where the tip goes: how far from the base, how high, and the turn
        // of the base. It touches the table, and lifts off now and then.
        const Length reach = 0.62_m + 0.08_m * units::math::sin(0.7_rad_per_s * t);
        const Length lift = 0.06_m * std::max(0.0, double(units::math::sin(0.45_rad_per_s * t)));
        const Length height = Length(TABLE_TOP) + 3.0_mm + lift - SHOULDER_HEIGHT;
        entity().joint().angle(0.6_rad * units::math::sin(0.5_rad_per_s * t));

        // Two links L1, L2 reach a point at distance d: the law of cosines
        // gives the bend of the elbow, then the shoulder aims below the point
        // by the angle the bend makes. Angles are measured from the vertical.
        const Length l1(UPPER_ARM);
        const Length l2(FOREARM);
        const Length d = units::math::hypot(reach, height);
        const double cos_bend = std::clamp(double((d * d - l1 * l1 - l2 * l2) / (2.0 * l1 * l2)), -1.0, 1.0);
        const Radians bend = units::math::acos(units::dimensionless::scalar_t(cos_bend));
        const Radians aim = units::math::atan2(reach, height);
        const Radians lean = aim - units::math::atan2(l2 * units::math::sin(bend), l1 + l2 * units::math::cos(bend));

        // The joints turn round +z: a negative angle leans toward +x.
        shoulder.joint().angle(-lean);
        elbow.joint().angle(-bend);
    }

    compages::world::Entity shoulder;
    compages::world::Entity elbow;
};

} // namespace

// ----------------------------------------------------------------------------
Arm buildArm(compages::world::World& p_world)
{
    // A revolute joint turns round one axis, in the axes of its parent. The
    // base turns round the vertical (y); the shoulder and the elbow round z,
    // so the arm swings in its own vertical plane.
    Arm arm;
    arm.base = p_world.entity("Arm").position(-0.45f, TABLE_TOP, 0.0f).revolute({ 0.0f, 1.0f, 0.0f });
    arm.shoulder = arm.base.child("Shoulder").position(0.0f, 0.12f, 0.0f).revolute({ 0.0f, 0.0f, 1.0f });
    arm.elbow = arm.shoulder.child("Elbow").position(0.0f, UPPER_ARM, 0.0f).revolute({ 0.0f, 0.0f, 1.0f });
    arm.tool = arm.elbow.child("Tool").position(0.0f, FOREARM, 0.0f);

    // The routine runs at each update of the World.
    arm.base.add<FollowPath>(arm.shoulder, arm.elbow);
    return arm;
}

} // namespace examples::workshop
