// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

#include <concepts>
#include <cstddef>

namespace compages
{

// ****************************************************************************
//! \brief Where a place of the plane falls in a grid: the cell between
//! points (i, j) and (i + 1, j + 1), and how far across it, from 0 to 1, in
//! the two directions of the grid. A field mixes the values of the four
//! corners with these weights (bilinear).
// ****************************************************************************
struct GridCell
{
    int i = 0;
    int j = 0;
    //! \brief From point (i, j) towards (i + 1, j), from 0 to 1.
    float s = 0.0f;
    //! \brief From point (i, j) towards (i, j + 1), from 0 to 1.
    float t = 0.0f;
    //! \brief False when the place is outside the grid: the cell and the
    //! weights are then those of the nearest edge.
    bool inside = false;
};

// ****************************************************************************
//! \brief What a Field needs from a grid of the plane: points numbered row
//! after row (index = j * columns + i), each one somewhere in the plane,
//! and a way to find the cell of any place.
//!
//! Grid2D finds a cell by a division (its points are regular);
//! CurvilinearGrid by a search (each point is placed freely). Field::sample(),
//! compages::streamline() and Scene::show() take any of them.
//!
//! \code
//! template <compages::PlanarGrid G>
//! float mean(compages::Field<float, G> const& p_field);
//! \endcode
// ****************************************************************************
template <typename G>
concept PlanarGrid = requires(G const& p_grid, int p_i, std::size_t p_index, Vector2f p_place) {
    { p_grid.columns } -> std::convertible_to<int>;
    { p_grid.rows } -> std::convertible_to<int>;
    { p_grid.count() } -> std::same_as<std::size_t>;
    { p_grid.index(p_i, p_i) } -> std::same_as<std::size_t>;
    { p_grid.point(p_i, p_i) } -> std::same_as<Vector2f>;
    { p_grid.point(p_index) } -> std::same_as<Vector2f>;
    { p_grid.locate(p_place) } -> std::same_as<GridCell>;
};

} // namespace compages
