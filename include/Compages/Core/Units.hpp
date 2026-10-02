//=====================================================================
// Compages: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

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
using radians_per_second_squared =
    units::compound_unit<units::angle::radians,
                         units::inverse<units::squared<units::time::seconds>>>;
using radians_per_second_squared_t = units::unit_t<radians_per_second_squared>;
} // namespace units::angular_acceleration

//! Length in SI meters (@c units::length::meter_t).
using Length = units::length::meter_t;

//! Duration or time instant in SI seconds (@c units::time::second_t).
using Seconds = units::time::second_t;

//! Plane angle in SI radians (@c units::angle::radian_t).
using Radians = units::angle::radian_t;

//! Angular velocity (rad/s).
using AngularVelocity = units::angular_velocity::radians_per_second_t;

//! Linear velocity (m/s).
using LinearVelocity = units::velocity::meters_per_second_t;

//! Torque (N·m).
using Torque = units::torque::newton_meter_t;

//! Force (N).
using Force = units::force::newton_t;