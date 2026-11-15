// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Arm.hpp"

#include <algorithm>
#include <cmath>

namespace examples::workshop
{

namespace
{

//! \brief The height of the shoulder above the floor.
constexpr double SHOULDER_HEIGHT = double(TABLE_TOP) + 0.12;

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

    void update(float) override
    {
        using units::angle::radian_t;
        const double t = frame().total;

        // Where the tip goes: how far from the base, how high, and the turn
        // of the base. It touches the table, and lifts off now and then.
        const double reach = 0.62 + 0.08 * std::sin(0.7 * t);
        const double lift = 0.06 * std::max(0.0, std::sin(0.45 * t));
        const double height = double(TABLE_TOP) + 0.003 + lift - SHOULDER_HEIGHT;
        entity().joint().angle(radian_t(0.6 * std::sin(0.5 * t)));

        // Two links L1, L2 reach a point at distance d: the law of cosines
        // gives the bend of the elbow, then the shoulder aims below the point
        // by the angle the bend makes. Angles are measured from the vertical.
        const double l1 = double(UPPER_ARM);
        const double l2 = double(FOREARM);
        const double d = std::hypot(reach, height);
        const double cos_bend = std::clamp((d * d - l1 * l1 - l2 * l2) / (2.0 * l1 * l2), -1.0, 1.0);
        const double bend = std::acos(cos_bend);
        const double aim = std::atan2(reach, height);
        const double lean = aim - std::atan2(l2 * std::sin(bend), l1 + l2 * cos_bend);

        // The joints turn round +z: a negative angle leans toward +x.
        shoulder.joint().angle(radian_t(-lean));
        elbow.joint().angle(radian_t(-bend));
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
