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

