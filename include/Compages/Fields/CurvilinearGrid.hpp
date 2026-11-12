// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Fields/Grid2D.hpp"
#include "Compages/Fields/PlanarGrid.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string>
#include <vector>

namespace compages
{

// ****************************************************************************
//! \brief A grid of the plane whose points are placed freely: the same
//! columns and rows as a Grid2D, the same (i, j) neighbours, the values of a
//! Field in the same order (so they go to the GPU as they are), but point
//! (i, j) is wherever you put it.
//!
//! What it is for: a grid fitted round a shape (body-fitted), tight where
//! things change fast (along a wall) and loose far away. Round a column, i
//! goes round it and j goes away from it (an O-grid): the wall is a line of
//! the grid, not a staircase of square cells.
//!
//! Finding the cell of a place (locate()) is a search, not a division: the
//! cells are sorted into buckets of a regular grid once, then a place is
//! tested against the few cells of its bucket (inverse bilinear, Newton).
//!
//! A ring that closes on itself repeats its first column as its last one:
//! the cells between them exist, and a field gives both the same values.
//!
//! \code
//! // i round the column, j away from it.
//! auto grid = compages::CurvilinearGrid::create(97, 40, [](int i, int j) {
//!     const float angle = 2.0f * pi * float(i) / 96.0f;
//!     const float r = 0.5f * std::pow(6.0f, float(j) / 39.0f);
//!     return compages::Vector2f(r * std::cos(angle), r * std::sin(angle));
//! });
//! compages::Field<float, compages::CurvilinearGrid> pressure(grid.value(), 0.0f);
//! \endcode
// ****************************************************************************
class CurvilinearGrid
{
public:

    using Neighbours = Grid2D::Neighbours;

    //! \brief Points along i and along j. Set by create(): do not change.
    int columns = 0;
    int rows = 0;

    //! \brief A grid of no point.
    CurvilinearGrid() = default;

    // ------------------------------------------------------------------------
    //! \brief A grid of \c p_columns x \c p_rows points, point (i, j) at
    //! \c p_place(i, j).
    //! \return The grid, or why not: fewer than 2 x 2 points.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<CurvilinearGrid>
    create(int p_columns, int p_rows, std::function<Vector2f(int, int)> const& p_place)
    {
        if ((p_columns < 2) || (p_rows < 2))
        {
            return failure("a CurvilinearGrid needs at least 2 x 2 points, not " + std::to_string(p_columns) +
                           " x " + std::to_string(p_rows));
        }
        CurvilinearGrid grid;
        grid.columns = p_columns;
        grid.rows = p_rows;
        grid.m_points.reserve(std::size_t(p_columns) * std::size_t(p_rows));
        for (int j = 0; j < p_rows; ++j)
        {
            for (int i = 0; i < p_columns; ++i)
            {
                grid.m_points.push_back(p_place(i, j));
            }
        }
        grid.sortCells();
        return grid;
    }

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
        return m_points[index(p_i, p_j)];
    }

    //! \brief Where a point of the grid is in the plane.
    [[nodiscard]] Vector2f point(std::size_t p_index) const
    {
        return m_points[p_index];
    }

    //! \brief The corners of the box round every point.
    [[nodiscard]] Vector2f low() const
    {
        return m_low;
    }

    [[nodiscard]] Vector2f high() const
    {
        return m_high;
    }

    //! \brief The points left, right, below and above, inside the grid (a
    //! closed ring does not wrap round: its seam is two columns).
    [[nodiscard]] Neighbours neighbours4(std::size_t p_index) const
    {
        constexpr std::array<std::array<int, 2>, 4> steps = { { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 } } };
        return around(p_index, steps);
    }

    //! \brief The eight points around, inside the grid.
    [[nodiscard]] Neighbours neighbours8(std::size_t p_index) const
    {
        constexpr std::array<std::array<int, 2>, 8> steps = {
            { { -1, -1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 } }
        };
        return around(p_index, steps);
    }

    // ------------------------------------------------------------------------
    //! \brief The cell a place falls in, and where in it. Outside the grid,
    //! the cell of the nearest point, its weights clamped to that corner.
    // ------------------------------------------------------------------------
    [[nodiscard]] GridCell locate(Vector2f p_place) const
    {
        if (m_points.empty())
        {
            return GridCell{};
        }
        const int bx = bucketOf(p_place.x, m_low.x, m_bucket_columns);
        const int by = bucketOf(p_place.y, m_low.y, m_bucket_rows);
        if ((bx >= 0) && (by >= 0))
        {
            for (std::uint32_t c : m_buckets[std::size_t((by * m_bucket_columns) + bx)])
            {
                const int i = int(c % std::uint32_t(columns - 1));
                const int j = int(c / std::uint32_t(columns - 1));
                if (GridCell found = inside(i, j, p_place); found.inside)
                {
                    return found;
                }
            }
        }
        return nearestCorner(p_place);
    }

    //! \brief The point nearest to a place.
    [[nodiscard]] Vector2i nearest(Vector2f p_place) const
    {
        const GridCell c = locate(p_place);
        return Vector2i(c.i + ((c.s > 0.5f) ? 1 : 0), c.j + ((c.t > 0.5f) ? 1 : 0));
    }

private:

    template <std::size_t N>
    [[nodiscard]] Neighbours around(std::size_t p_index, std::array<std::array<int, 2>, N> const& p_steps) const
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

    //! \brief The bucket of a coordinate, -1 outside.
    [[nodiscard]] int bucketOf(float p_at, float p_low, int p_count) const
    {
        const float at = (p_at - p_low) / m_bucket_size;
        if ((at < 0.0f) || (at >= float(p_count)))
        {
            return -1;
        }
        return std::min(int(at), p_count - 1);
    }

    // ------------------------------------------------------------------------
    //! \brief Sort the cells into buckets of a regular grid over the box of
    //! the points, about one cell a bucket.
    // ------------------------------------------------------------------------
    void sortCells()
    {
        m_low = m_points.front();
        m_high = m_points.front();
        for (Vector2f const& p : m_points)
        {
            m_low = Vector2f(std::min(m_low.x, p.x), std::min(m_low.y, p.y));
            m_high = Vector2f(std::max(m_high.x, p.x), std::max(m_high.y, p.y));
        }
        const float width = std::max(m_high.x - m_low.x, 1.0e-6f);
        const float depth = std::max(m_high.y - m_low.y, 1.0e-6f);
        const float cells = float((columns - 1) * (rows - 1));
        m_bucket_size = std::max(std::sqrt((width * depth) / cells), 1.0e-6f);
        m_bucket_columns = std::clamp(int(std::ceil(width / m_bucket_size)), 1, 1024);
        m_bucket_rows = std::clamp(int(std::ceil(depth / m_bucket_size)), 1, 1024);
        m_bucket_size = std::max(width / float(m_bucket_columns), depth / float(m_bucket_rows)) * 1.0001f;
        m_buckets.assign(std::size_t(m_bucket_columns * m_bucket_rows), {});
        for (int j = 0; j + 1 < rows; ++j)
        {
            for (int i = 0; i + 1 < columns; ++i)
            {
                Vector2f a = point(i, j);
                Vector2f b = a;
                for (Vector2f const& p : { point(i + 1, j), point(i, j + 1), point(i + 1, j + 1) })
                {
                    a = Vector2f(std::min(a.x, p.x), std::min(a.y, p.y));
                    b = Vector2f(std::max(b.x, p.x), std::max(b.y, p.y));
                }
                const int x0 = std::clamp(int((a.x - m_low.x) / m_bucket_size), 0, m_bucket_columns - 1);
                const int x1 = std::clamp(int((b.x - m_low.x) / m_bucket_size), 0, m_bucket_columns - 1);
                const int y0 = std::clamp(int((a.y - m_low.y) / m_bucket_size), 0, m_bucket_rows - 1);
                const int y1 = std::clamp(int((b.y - m_low.y) / m_bucket_size), 0, m_bucket_rows - 1);
                const auto cell = std::uint32_t((j * (columns - 1)) + i);
                for (int y = y0; y <= y1; ++y)
                {
                    for (int x = x0; x <= x1; ++x)
                    {
                        m_buckets[std::size_t((y * m_bucket_columns) + x)].push_back(cell);
                    }
                }
            }
        }
    }

    // ------------------------------------------------------------------------
    //! \brief Is a place in cell (i, j)? Solve p = a + (b - a) s + (c - a) t
    //! + (a - b - c + d) s t for (s, t) by Newton's method (the inverse of
    //! the bilinear map of the cell).
    // ------------------------------------------------------------------------
    [[nodiscard]] GridCell inside(int p_i, int p_j, Vector2f p_place) const
    {
        const Vector2f a = point(p_i, p_j);
        const Vector2f b = point(p_i + 1, p_j);
        const Vector2f c = point(p_i, p_j + 1);
        const Vector2f d = point(p_i + 1, p_j + 1);
        const Vector2f ab = b - a;
        const Vector2f ac = c - a;
        const Vector2f e = a - b - c + d;
        float s = 0.5f;
        float t = 0.5f;
        for (int step = 0; step < 8; ++step)
        {
            const Vector2f f = a + (ab * s) + (ac * t) + (e * (s * t)) - p_place;
            const Vector2f ds = ab + (e * t);
            const Vector2f dt = ac + (e * s);
            const float det = (ds.x * dt.y) - (ds.y * dt.x);
            if (std::abs(det) < 1.0e-12f)
            {
                break;
            }
            s -= ((f.x * dt.y) - (f.y * dt.x)) / det;
            t -= ((ds.x * f.y) - (ds.y * f.x)) / det;
        }
        constexpr float EPS = 1.0e-4f;
        const bool in = (s >= -EPS) && (s <= 1.0f + EPS) && (t >= -EPS) && (t <= 1.0f + EPS);
        return GridCell{ .i = p_i, .j = p_j, .s = std::clamp(s, 0.0f, 1.0f), .t = std::clamp(t, 0.0f, 1.0f), .inside = in };
    }

    //! \brief Outside the grid: the nearest point, as a corner of a cell.
    [[nodiscard]] GridCell nearestCorner(Vector2f p_place) const
    {
        std::size_t best = 0u;
        float closest = std::numeric_limits<float>::max();
        for (std::size_t k = 0u; k < m_points.size(); ++k)
        {
            const Vector2f d = m_points[k] - p_place;
            const float d2 = (d.x * d.x) + (d.y * d.y);
            if (d2 < closest)
            {
                closest = d2;
                best = k;
            }
        }
        const int i = column(best);
        const int j = row(best);
        const int ci = std::min(i, columns - 2);
        const int cj = std::min(j, rows - 2);
        return GridCell{ .i = ci, .j = cj, .s = float(i - ci), .t = float(j - cj), .inside = false };
    }

    std::vector<Vector2f> m_points;
    Vector2f m_low{ 0.0f, 0.0f };
    Vector2f m_high{ 0.0f, 0.0f };
    float m_bucket_size = 1.0f;
    int m_bucket_columns = 0;
    int m_bucket_rows = 0;
    //! \brief The cells (j * (columns - 1) + i) touching each bucket.
    std::vector<std::vector<std::uint32_t>> m_buckets;
};

static_assert(PlanarGrid<Grid2D>);
static_assert(PlanarGrid<CurvilinearGrid>);

} // namespace compages
