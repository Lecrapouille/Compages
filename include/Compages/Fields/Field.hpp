// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Fields/Grid2D.hpp"

#include <algorithm>
#include <span>
#include <utility>
#include <vector>

namespace compages
{

// ****************************************************************************
//! \brief One value per point of a Grid2D: a height, a temperature, a wind.
//!
//! The values are contiguous, row after row, like the grid numbers its
//! points: they can be sent to the GPU as they are (values()). A field of a
//! million points is one object, never a million entities.
//!
//! \code
//! compages::Field<float> altitude(grid, 0.0f);
//! altitude(10, 20) = 35.0f;
//! const float h = altitude.sample({ 3.2f, -7.5f }); // bilinear
//! \endcode
// ****************************************************************************
template <typename T>
class Field
{
public:

    //! \brief An empty field, on a grid of no point.
    Field() = default;

    // ------------------------------------------------------------------------
    //! \brief A field where every point holds the same value.
    //! \param[in] p_grid The points.
    //! \param[in] p_value The value of each.
    // ------------------------------------------------------------------------
    Field(Grid2D const& p_grid, T const& p_value)
        : m_grid(p_grid), m_values(p_grid.count(), p_value)
    {
    }

    //! \brief The points.
    [[nodiscard]] Grid2D const& grid() const
    {
        return m_grid;
    }

    //! \brief How many values.
    [[nodiscard]] std::size_t size() const
    {
        return m_values.size();
    }

    //! \brief The value of a point by its index.
    [[nodiscard]] T& operator[](std::size_t p_index)
    {
        return m_values[p_index];
    }

    //! \brief The value of a point by its index.
    [[nodiscard]] T const& operator[](std::size_t p_index) const
    {
        return m_values[p_index];
    }

    //! \brief The value of point (i, j), which must be in the grid.
    [[nodiscard]] T& operator()(int p_i, int p_j)
    {
        return m_values[m_grid.index(p_i, p_j)];
    }

    //! \brief The value of point (i, j), which must be in the grid.
    [[nodiscard]] T const& operator()(int p_i, int p_j) const
    {
        return m_values[m_grid.index(p_i, p_j)];
    }

    //! \brief All the values, row after row: to send to the GPU or to a
    //! library.
    [[nodiscard]] std::span<T> values()
    {
        return m_values;
    }

    //! \brief All the values, row after row.
    [[nodiscard]] std::span<T const> values() const
    {
        return m_values;
    }

    [[nodiscard]] auto begin()
    {
        return m_values.begin();
    }

    [[nodiscard]] auto end()
    {
        return m_values.end();
    }

    [[nodiscard]] auto begin() const
    {
        return m_values.begin();
    }

    [[nodiscard]] auto end() const
    {
        return m_values.end();
    }

    //! \brief Give every point the same value.
    void fill(T const& p_value)
    {
        std::fill(m_values.begin(), m_values.end(), p_value);
    }

    // ------------------------------------------------------------------------
    //! \brief The value at a place of the plane, mixed from the four points
    //! around it (bilinear). Outside the grid, the value of its edge.
    //!
    //! For values that can be added and scaled: float, Vector2f, Vector3f...
    //! \param[in] p_place Where, in the plane.
    // ------------------------------------------------------------------------
    [[nodiscard]] T sample(Vector2f p_place) const
    {
        const Vector2f at = m_grid.coordinates(p_place);
        const auto [i, s] = cell(at.x, m_grid.columns);
        const auto [j, t] = cell(at.y, m_grid.rows);
        const int i1 = std::min(i + 1, m_grid.columns - 1);
        const int j1 = std::min(j + 1, m_grid.rows - 1);
        return ((*this)(i, j) * ((1.0f - s) * (1.0f - t))) +
               ((*this)(i1, j) * (s * (1.0f - t))) +
               ((*this)(i, j1) * ((1.0f - s) * t)) + ((*this)(i1, j1) * (s * t));
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

    Grid2D m_grid;
    std::vector<T> m_values;
};

} // namespace compages
