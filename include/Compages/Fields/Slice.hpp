// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Fields/Field.hpp"

namespace compages
{

//! \brief An axis of a Grid3D: i along x, j along y, k along z.
enum class Axis
{
    X,
    Y,
    Z,
};

// ----------------------------------------------------------------------------
//! \brief One layer of a volume, across an axis, as a 2D field: what a cut
//! through it shows.
//!
//! The 2D grid keeps the two other axes in order: across Y (a horizontal
//! cut), its points are (x, z), as Scene::show() lays a field on the ground;
//! across X they are (y, z), across Z (x, y).
//! \code
//! compages::Field<float> cut = compages::slice(smoke, compages::Axis::Y, 12);
//! scene.show(cut, look, "Cut").position(0.0f, smoke.grid().point(0, 12, 0).y, 0.0f);
//! \endcode
//! \param[in] p_volume The volume.
//! \param[in] p_axis Across which axis.
//! \param[in] p_index Which layer along it (clamped to the grid).
//! \return The values of that layer.
// ----------------------------------------------------------------------------
template <typename T>
[[nodiscard]] Field<T> slice(Field<T, Grid3D> const& p_volume, Axis p_axis, int p_index)
{
    Grid3D const& grid = p_volume.grid();
    const int columns = (p_axis == Axis::X) ? grid.rows : grid.columns;
    const int rows = (p_axis == Axis::Z) ? grid.rows : grid.layers;
    const Vector2f origin = (p_axis == Axis::X)   ? Vector2f(grid.origin.y, grid.origin.z)
                            : (p_axis == Axis::Y) ? Vector2f(grid.origin.x, grid.origin.z)
                                                  : Vector2f(grid.origin.x, grid.origin.y);
    const int last = (p_axis == Axis::X) ? grid.columns : (p_axis == Axis::Y) ? grid.rows : grid.layers;
    const int at = std::clamp(p_index, 0, last - 1);
    Field<T> cut(Grid2D{ .columns = columns, .rows = rows, .origin = origin, .spacing = grid.spacing },
                 p_volume[0]);
    for (int b = 0; b < rows; ++b)
    {
        for (int a = 0; a < columns; ++a)
        {
            cut(a, b) = (p_axis == Axis::X)   ? p_volume(at, a, b)
                        : (p_axis == Axis::Y) ? p_volume(a, at, b)
                                              : p_volume(a, b, at);
        }
    }
    return cut;
}

} // namespace compages
