// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#include "units.h"
#pragma GCC diagnostic pop

// nholthaus/units v2.3.3 has angular velocities but no angular acceleration.
namespace units::angular_acceleration
{
using radians_per_second_squared = units::compound_unit<
    units::angle::radians,
    units::inverse<units::squared<units::time::seconds>>>;
using radians_per_second_squared_t = units::unit_t<radians_per_second_squared>;
} // namespace units::angular_acceleration

// ****************************************************************************
//! \brief Short names of the physical quantities, in SI units.
//!
//! A value carries its unit in its type: adding a duration to a length does
//! not compile, and a function asking for an angle cannot be given metres by
//! mistake. Read the plain number back with \c value().
//!
//! \code
//! using namespace compages::si;
//! Seconds const dt(0.01);
//! Radians const angle(1.57);
//! Length const reach(0.8);
//! double const raw = dt.value();
//! \endcode
// ****************************************************************************
namespace compages::si
{
using Seconds = ::units::time::second_t;                       //!< Duration (s).
using Radians = ::units::angle::radian_t;                      //!< Angle (rad).
using Length = ::units::length::meter_t;                       //!< Distance (m).
using LinearVelocity = ::units::velocity::meters_per_second_t; //!< Speed (m/s).
using AngularVelocity =
    ::units::angular_velocity::radians_per_second_t; //!< Turn rate (rad/s).
using AngularAcceleration =
    ::units::angular_acceleration::radians_per_second_squared_t; //!< (rad/s²).
using Force = ::units::force::newton_t;         //!< Force (N).
using Torque = ::units::torque::newton_meter_t; //!< Torque (N.m).
} // namespace compages::si

// ****************************************************************************
//! \brief The unit suffixes: \c 0.5_s, \c 90.0_deg, \c 1.2_rad_per_s, \c 2.0_m.
//!
//! \code
//! using namespace compages::si::literals;
//! cube.rotate(1.2_rad_per_s * frame.elapsed, { 0.0f, 1.0f, 0.0f });
//! routine.after(0.5_s, [] { });
//! \endcode
// ****************************************************************************
namespace compages::si::literals
{
using namespace ::units::literals;
} // namespace compages::si::literals
