// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

#include <array>
#include <cmath>
#include <cstddef>

namespace compages
{

// ****************************************************************************
//! \brief The places of a regular grid in space: \c columns x \c rows x
//! \c layers points, \c spacing apart, point (0, 0, 0) at \c origin.
//!
//! The 3D sister of Grid2D: i along x, j along y, k along z. Points are
//! numbered x first, then y, then z: index = (k * rows + j) * columns + i,
//! which is also the order of the texels of a 3D texture, so a
//! Field<T, Grid3D> goes to the GPU as it is (gpu::Texture::fromField()).
// ****************************************************************************
struct Grid3D
{
    // ************************************************************************
    //! \brief Up to six indices: `for (std::size_t n : grid.neighbours6(c))`.
    // ************************************************************************
    struct Neighbours
    {
        std::array<std::size_t, 6> indices{};
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

    //! \brief Points along x (i), y (j) and z (k).
    int columns = 0;
    int rows = 0;
    int layers = 0;
    //! \brief Where point (0, 0, 0) is.
    Vector3f origin{ 0.0f, 0.0f, 0.0f };
    //! \brief The distance between two neighbour points.
    float spacing = 1.0f;

    //! \brief How many points.
    [[nodiscard]] std::size_t count() const
    {
        return std::size_t(columns) * std::size_t(rows) * std::size_t(layers);
    }

    //! \brief True when (i, j, k) is a point of the grid.
    [[nodiscard]] bool contains(int p_i, int p_j, int p_k) const
    {
        return (p_i >= 0) && (p_j >= 0) && (p_k >= 0) && (p_i < columns) && (p_j < rows) &&
               (p_k < layers);
    }

    //! \brief The index of point (i, j, k), which must be in the grid.
    [[nodiscard]] std::size_t index(int p_i, int p_j, int p_k) const
    {
        return (((std::size_t(p_k) * std::size_t(rows)) + std::size_t(p_j)) * std::size_t(columns)) +
               std::size_t(p_i);
    }

    //! \brief The column (i) of an index.
    [[nodiscard]] int column(std::size_t p_index) const
    {
        return int(p_index % std::size_t(columns));
    }

    //! \brief The row (j) of an index.
    [[nodiscard]] int row(std::size_t p_index) const
    {
        return int((p_index / std::size_t(columns)) % std::size_t(rows));
    }

    //! \brief The layer (k) of an index.
    [[nodiscard]] int layer(std::size_t p_index) const
    {
        return int(p_index / (std::size_t(columns) * std::size_t(rows)));
    }

    //! \brief Where point (i, j, k) is in space.
    [[nodiscard]] Vector3f point(int p_i, int p_j, int p_k) const
    {
        return Vector3f(origin.x + (float(p_i) * spacing),
                        origin.y + (float(p_j) * spacing),
                        origin.z + (float(p_k) * spacing));
    }

    //! \brief Where a point of the grid is in space.
    [[nodiscard]] Vector3f point(std::size_t p_index) const
    {
        return point(column(p_index), row(p_index), layer(p_index));
    }

    //! \brief A place in grid units: (0, 0, 0) at point (0, 0, 0); not
    //! rounded, not clamped.
    [[nodiscard]] Vector3f coordinates(Vector3f p_place) const
    {
        return Vector3f((p_place.x - origin.x) / spacing,
                        (p_place.y - origin.y) / spacing,
                        (p_place.z - origin.z) / spacing);
    }

    //! \brief The point nearest to a place (maybe outside: check with
    //! contains()).
    [[nodiscard]] Vector3i nearest(Vector3f p_place) const
    {
        const Vector3f at = coordinates(p_place);
        return Vector3i(int(std::lround(at.x)), int(std::lround(at.y)), int(std::lround(at.z)));
    }

    //! \brief The six points around (left, right, below, above, behind, in
    //! front), inside the grid.
    [[nodiscard]] Neighbours neighbours6(std::size_t p_index) const
    {
        constexpr std::array<std::array<int, 3>, 6> steps = {
            { { -1, 0, 0 }, { 1, 0, 0 }, { 0, -1, 0 }, { 0, 1, 0 }, { 0, 0, -1 }, { 0, 0, 1 } }
        };
        const int i = column(p_index);
        const int j = row(p_index);
        const int k = layer(p_index);
        Neighbours result;
        for (auto const& [di, dj, dk] : steps)
        {
            if (contains(i + di, j + dj, k + dk))
            {
                result.indices[result.size++] = index(i + di, j + dj, k + dk);
            }
        }
        return result;
    }
};

} // namespace compages
