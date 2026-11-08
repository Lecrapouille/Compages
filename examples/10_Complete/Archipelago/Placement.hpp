// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Core/Vector.hpp"

#include <vector>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief The layout of the graph, fitted to the map, in metres.
//!
//! A force directed layout puts the islands far apart and their links close
//! together: drawn as it is, the map would be all sea with a few dots. So
//! each island is given a disc whose area is its share of the links (of
//! about LAND of the map); the discs keep the directions of the layout but
//! are pushed apart until a strait of sea separates them; the nodes of an
//! island are then scaled into its disc.
// ****************************************************************************
struct Placement
{
    //! \brief Per island: its disc, and its own node.
    std::vector<compages::Vector2f> centres;
    std::vector<float> radii;
    std::vector<compages::Vector2f> islands;
    //! \brief Per region: its node and the nodes of its links.
    std::vector<compages::Vector2f> regions;
    std::vector<std::vector<compages::Vector2f>> links;
};

// ----------------------------------------------------------------------------
//! \brief Fit the layout of a recipe to the map.
//! \return The same recipe gives the same placement.
// ----------------------------------------------------------------------------
[[nodiscard]] Placement place(Recipe const& p_recipe);

} // namespace examples::archipelago
