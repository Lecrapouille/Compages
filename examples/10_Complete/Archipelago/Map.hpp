// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The sizes of the map of 10e_Archipelago, shared by every file of
//! the land. The World is in metres: each size is written with its unit
//! (units::length) and turned into metres by compages::toMetres().
// ****************************************************************************

#include "Compages/Core/Maths.hpp"

#include <functional>

namespace examples::archipelago
{

//! \brief The side of the map (16 km), centred on the origin, X east, -Z
//! north.
inline constexpr float MAP =
    compages::toMetres(units::length::kilometer_t(16.0));
//! \brief Voronoi cells along a side: 96 x 96 cells of 167 m.
inline constexpr int CELLS = 96;
inline constexpr float CELL_SIZE = MAP / float(CELLS);
//! \brief The highest ground and the deepest sea of the map.
inline constexpr float PEAK = compages::toMetres(units::length::meter_t(520.0));
inline constexpr float SEABED =
    compages::toMetres(units::length::meter_t(90.0));
//! \brief The alpha of the picture says what the shader of the map draws:
//! the sea from SEA_ALPHA (the shore) down to 0 (SEABED deep and more), a
//! river at RIVER_ALPHA, the land at 1. The sea grows toward the land: the
//! GPU blends the pixels of the picture, and along a coast the blend of the
//! shore and of the land must still read as shallow water. (The other way
//! round, shore 0 and land 1, the blend went through every depth: a dark
//! line of deep water, in steps of pixels, along every coast.)
inline constexpr float SEA_ALPHA = 0.9f;
inline constexpr float RIVER_ALPHA = 0.94f;

//! \brief How far a long job has gone, from 0 to 1, and what it does.
using Progress = std::function<void(float, char const*)>;

} // namespace examples::archipelago
