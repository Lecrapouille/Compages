// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The stages of makeLand(), in their order, and what they share.
//! Only Land.cpp and the files of the stages include this.
// ****************************************************************************

#include "10_Complete/Archipelago/Geology.hpp"
#include "10_Complete/Archipelago/Land.hpp"
#include "10_Complete/Archipelago/Placement.hpp"

#include <vector>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief A map being made: the recipe, the land so far, and the fields per
//! cell the stages pass to each other.
// ****************************************************************************
struct Making
{
    Recipe const& recipe;
    Progress const& progress;
    Placement placement;
    Land land;
    //! \brief Per cell: how its coordinates are bent (-1 to 1 on each
    //! axis), for the coasts and the borders.
    compages::Field<compages::Vector2f> bends;
    //! \brief Per cell: land where positive, deeper sea where more negative.
    compages::Field<float> shape;
    //! \brief Per cell: the water leaves the land there (sea, edge of the map).
    compages::Field<char> outlet;
    //! \brief Per cell: the relief the erosion made (its own scale).
    compages::Field<float> ground;
    Drainage drainage;
    //! \brief Per cell: a river flows through it.
    compages::Field<char> on_river;
    //! \brief Per region: its tributary, from its mouth to its source.
    std::vector<std::vector<int>> paths;
};

// Shores.cpp: the islands around the graph, the sea, the regions.
void shapeIslands(Making& p_making);
void floodSea(Making& p_making);
void divideRegions(Making& p_making);

// Relief.cpp: uplift and erosion, the heights, the peaks.
void raiseRelief(Making& p_making);
void findPeaks(Making& p_making);

// Strands.cpp: the rivers, then the links along them.
void traceRivers(Making& p_making);
void placeLinks(Making& p_making);

// Outlines.cpp: the outline of each region.
void traceOutlines(Making& p_making);

} // namespace examples::archipelago
