// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The relief of the cells grows: the plate rises under the islands
//! (tectonic uplift U) and the rivers cut it down. The stream power law says
//! how fast a river erodes its bed: faster where it drains more land (area
//! A) and where it is steeper (slope S),
//!
//!     dh/dt = U - K A^m S^n,      m = 1/2, n = 1,
//!
//! plus a slow creep of the slopes (hillslope diffusion). With n = 1, a step
//! is solved exactly from the sea upward (Braun and Willett 2013): a cell
//! only needs the new height of the cell it drains to. The pits are filled
//! first (priority flood, Barnes et al. 2014: a lake overflows at its lowest
//! pass), so that the water of every cell reaches the sea. Cordonnier et al.
//! (2016) run this on the graph of a Voronoi diagram, as here.
//!
//! Lengths are in metres; the time of the geology is in years.
// ****************************************************************************

#include "10_Complete/Archipelago/Map.hpp"

#include "Compages/Core/Vector.hpp"
#include "Compages/Fields/Field.hpp"

#include <vector>

namespace examples::archipelago
{

//! \brief How the cells drain: each one to its receiver (-1 for the sea);
//! the order goes from the sea upward; the area each one drains (m2).
struct Drainage
{
    std::vector<int> receivers;
    std::vector<int> order;
    std::vector<float> areas;
};

// ----------------------------------------------------------------------------
//! \brief The drainage of the cells by priority flood: the outlets are
//! reached first, then always the lowest cell not yet reached, from the
//! neighbour it was reached from, which becomes its receiver.
//! \param[in] p_outlet The cells the water leaves the land by (the sea).
// ----------------------------------------------------------------------------
[[nodiscard]] Drainage drain(compages::Field<float> const& p_heights,
                             compages::Field<char> const& p_outlet);

// ----------------------------------------------------------------------------
//! \brief Grow the relief of the cells: uplift, river erosion, creep.
//! \param[in,out] p_heights The heights of the cells, in metres.
//! \param[in] p_uplift The uplift of each cell, in metres a year.
//! \param[in] p_steps Steps of 40 000 years.
//! \return The drainage of the final relief.
// ----------------------------------------------------------------------------
[[nodiscard]] Drainage erode(compages::Field<compages::Vector2f> const& p_sites,
                             compages::Field<float>& p_heights,
                             compages::Field<float> const& p_uplift,
                             compages::Field<char> const& p_outlet,
                             int p_steps,
                             Progress const& p_progress);

//! \brief The uplift at the heart of an island: 1 mm a year.
inline constexpr float UPLIFT =
    compages::toMetres(units::length::millimeter_t(1.0));

} // namespace examples::archipelago
