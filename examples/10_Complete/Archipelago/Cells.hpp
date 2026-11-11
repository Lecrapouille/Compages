// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The Voronoi cells of the map: one site per square of a CELLS x
//! CELLS grid, moved a little. The nearest site of a point is so always
//! among the 3 x 3 squares around it: no diagram to build, a cell is found
//! in nine distances.
// ****************************************************************************

#include "10_Complete/Archipelago/Map.hpp"

#include "Compages/Core/Vector.hpp"
#include "Compages/Fields/Field.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace examples::archipelago
{

//! \brief All the cells of the map.
inline constexpr std::size_t CELL_COUNT =
    std::size_t(CELLS) * std::size_t(CELLS);

//! \brief The squares of the map, one cell each: point (i, j) is the centre
//! of square (i, j), the site of its cell is near it.
inline const compages::Grid2D CELL_GRID{
    .columns = CELLS,
    .rows = CELLS,
    .origin = compages::Vector2f((-0.5f * MAP) + (0.5f * CELL_SIZE),
                                 (-0.5f * MAP) + (0.5f * CELL_SIZE)),
    .spacing = CELL_SIZE
};

//! \brief The grid square a coordinate falls in.
[[nodiscard]] int squareOf(float p_coordinate);

//! \brief The sites: one per grid square, moved from its centre by up to
//! 0.3 square, from the seed.
[[nodiscard]] compages::Field<compages::Vector2f>
scatterSites(std::uint32_t p_seed);

//! \brief The distance from a point to the bisector of the sites of two
//! cells, positive on the side of the first.
[[nodiscard]] float bisector(compages::Vector2f p_own,
                             compages::Vector2f p_other,
                             compages::Vector2f p_point);

//! \brief The cell a point is in (the nearest site), and the nearest border
//! of that cell: the cell across it and how far it is.
struct CellHit
{
    int cell = 0;
    int across = -1;
    float edge = 1.0e9f;
};

[[nodiscard]] CellHit hitCell(compages::Field<compages::Vector2f> const& p_sites,
                              compages::Vector2f p_point);

//! \brief The corners of a cell, clockwise seen from above (+Y): a square
//! around its site cut by the bisectors with the sites around it.
[[nodiscard]] std::vector<compages::Vector2f>
cellPolygon(compages::Field<compages::Vector2f> const& p_sites, int p_cell);

//! \brief The distance from the border of a group of cells, positive inside:
//! where the outline of the group is zero.
//! \param[in] p_member Tells if a cell is in the group.
template <typename Member>
[[nodiscard]] float groupField(compages::Field<compages::Vector2f> const& p_sites,
                               compages::Vector2f p_point,
                               Member const& p_member)
{
    const CellHit hit = hitCell(p_sites, p_point);
    const bool inside = p_member(hit.cell);
    const compages::Vector2f own = p_sites[std::size_t(hit.cell)];
    float nearest = CELL_SIZE;
    for (const std::size_t other : CELL_GRID.neighbours8(std::size_t(hit.cell)))
    {
        if (p_member(int(other)) != inside)
        {
            nearest = std::min(nearest, bisector(own, p_sites[other], p_point));
        }
    }
    return inside ? nearest : -nearest;
}

} // namespace examples::archipelago
