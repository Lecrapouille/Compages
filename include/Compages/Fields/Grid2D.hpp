// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"
#include "Compages/Fields/PlanarGrid.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <utility>

namespace compages
{

// ****************************************************************************
//! \brief The places of a regular grid in a plane: \c columns x \c rows
//! points, \c spacing apart, point (0, 0) at \c origin.
//!
//! A grid holds no value: a Field does. The plane is the ground seen from
//! above, so a Vector2f is (x, z) in a 3D scene. Points are numbered row
//! after row: index = j * columns + i.
//!
//! \code
//! const compages::Grid2D grid{ .columns = 256, .rows = 256,
//!                              .origin = { -128.0f, -128.0f },
//!                              .spacing = 1.0f };
//! for (std::size_t n : grid.neighbours4(grid.index(10, 20))) { ... }
//! \endcode
// ****************************************************************************
struct Grid2D
{
    // ************************************************************************
    //! \brief Up to eight indices: `for (std::size_t n : grid.neighbours8(c))`.
    // ************************************************************************
    struct Neighbours
    {
        std::array<std::size_t, 8> indices{};
        std::size_t size = 0u;

        [[nodiscard]] std::size_t const* begin() const
        {
            return indices.data();
        }

        [[nodiscard]] std::size_t const* end() const
        {
            return indices.data() + size;
        }
    };

    //! \brief Points along x (i) and along z (j).
    int columns = 0;
    int rows = 0;
    //! \brief Where point (0, 0) is.
    Vector2f origin{ 0.0f, 0.0f };
    //! \brief The distance between two neighbour points.
    float spacing = 1.0f;

    //! \brief How many points.
    [[nodiscard]] std::size_t count() const
    {
        return std::size_t(columns) * std::size_t(rows);
    }

    //! \brief True when (i, j) is a point of the grid.
    [[nodiscard]] bool contains(int p_i, int p_j) const
    {
        return (p_i >= 0) && (p_j >= 0) && (p_i < columns) && (p_j < rows);
    }

    //! \brief The index of point (i, j), which must be in the grid.
    [[nodiscard]] std::size_t index(int p_i, int p_j) const
    {
        return (std::size_t(p_j) * std::size_t(columns)) + std::size_t(p_i);
    }

    //! \brief The column (i) of an index.
    [[nodiscard]] int column(std::size_t p_index) const
    {
        return int(p_index % std::size_t(columns));
    }

    //! \brief The row (j) of an index.
    [[nodiscard]] int row(std::size_t p_index) const
    {
        return int(p_index / std::size_t(columns));
    }

    //! \brief Where point (i, j) is in the plane.
    [[nodiscard]] Vector2f point(int p_i, int p_j) const
    {
        return Vector2f(origin.x + (float(p_i) * spacing),
                        origin.y + (float(p_j) * spacing));
    }

    //! \brief Where a point of the grid is in the plane.
    [[nodiscard]] Vector2f point(std::size_t p_index) const
    {
        return point(column(p_index), row(p_index));
    }

    //! \brief A place of the plane in grid units: (0, 0) at point (0, 0),
    //! (1, 0) at point (1, 0); not rounded, not clamped.
    [[nodiscard]] Vector2f coordinates(Vector2f p_place) const
    {
        return Vector2f((p_place.x - origin.x) / spacing,
                        (p_place.y - origin.y) / spacing);
    }

    //! \brief The cell a place falls in (a division: the points are
    //! regular). Outside the grid, the nearest cell of the edge.
    [[nodiscard]] GridCell locate(Vector2f p_place) const
    {
        const Vector2f at = coordinates(p_place);
        const auto [i, s] = cell(at.x, columns);
        const auto [j, t] = cell(at.y, rows);
        const bool inside = (at.x >= 0.0f) && (at.y >= 0.0f) &&
                            (at.x <= float(columns - 1)) && (at.y <= float(rows - 1));
        return GridCell{ .i = i, .j = j, .s = s, .t = t, .inside = inside };
    }

    //! \brief The point nearest to a place (maybe outside the grid: check
    //! with contains()).
    [[nodiscard]] Vector2i nearest(Vector2f p_place) const
    {
        const Vector2f at = coordinates(p_place);
        return Vector2i(int(std::lround(at.x)), int(std::lround(at.y)));
    }

    //! \brief The points left, right, below and above, inside the grid.
    [[nodiscard]] Neighbours neighbours4(std::size_t p_index) const
    {
        constexpr std::array<std::array<int, 2>, 4> steps = {
            { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } }
        };
        return around(p_index, steps);
    }

    //! \brief The eight points around, inside the grid.
    [[nodiscard]] Neighbours neighbours8(std::size_t p_index) const
    {
        constexpr std::array<std::array<int, 2>, 8> steps = {
            { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 },
              { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } }
        };
        return around(p_index, steps);
    }

private:

    //! \brief The point before a coordinate and how far after it, clamped.
    [[nodiscard]] static std::pair<int, float> cell(float p_at, int p_count)
    {
        const float last = float(std::max(p_count - 1, 0));
        const float at = std::clamp(p_at, 0.0f, last);
        const int before = std::min(int(at), std::max(p_count - 2, 0));
        return { before, at - float(before) };
    }

    template <std::size_t N>
    [[nodiscard]] Neighbours
    around(std::size_t p_index,
           std::array<std::array<int, 2>, N> const& p_steps) const
    {
        const int i = column(p_index);
        const int j = row(p_index);
        Neighbours result;
        for (auto const& [di, dj] : p_steps)
        {
            if (contains(i + di, j + dj))
            {
                result.indices[result.size++] = index(i + di, j + dj);
            }
        }
        return result;
    }
};

} // namespace compages
