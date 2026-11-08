// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Cells.hpp"

#include <algorithm>
#include <cmath>
#include <random>

namespace examples::archipelago
{

using compages::Vector2f;

//! \brief How far the sites move from the centres of their grid squares, in
//! squares: under 0.3, the nearest site of a point is always among the 3 x 3
//! squares around it.
static constexpr float JITTER = 0.3f;

// ----------------------------------------------------------------------------
int squareOf(float p_coordinate)
{
    return std::clamp(
        int(std::floor((p_coordinate + (0.5f * MAP)) / CELL_SIZE)),
        0,
        CELLS - 1);
}

// ----------------------------------------------------------------------------
Around neighbours(int p_cell)
{
    Around around;
    const int ci = p_cell % CELLS;
    const int cj = p_cell / CELLS;
    for (int j = std::max(0, cj - 1); j <= std::min(CELLS - 1, cj + 1); ++j)
    {
        for (int i = std::max(0, ci - 1); i <= std::min(CELLS - 1, ci + 1); ++i)
        {
            if ((i != ci) || (j != cj))
            {
                around.cells[around.size] = cellAt(i, j);
                ++around.size;
            }
        }
    }
    return around;
}

// ----------------------------------------------------------------------------
std::vector<Vector2f> scatterSites(std::uint32_t p_seed)
{
    // A procedural map, not a secret: any generator that repeats will do.
    std::mt19937 random(p_seed); // NOSONAR
    std::uniform_real_distribution<float> unit(-JITTER, JITTER);
    std::vector sites(CELL_COUNT, Vector2f(0.0f, 0.0f));
    for (int j = 0; j < CELLS; ++j)
    {
        for (int i = 0; i < CELLS; ++i)
        {
            const float x = float(i) + 0.5f + unit(random);
            const float z = float(j) + 0.5f + unit(random);
            sites[std::size_t(cellAt(i, j))] =
                Vector2f((-0.5f * MAP) + (x * CELL_SIZE),
                         (-0.5f * MAP) + (z * CELL_SIZE));
        }
    }
    return sites;
}

// ----------------------------------------------------------------------------
float bisector(Vector2f p_own, Vector2f p_other, Vector2f p_point)
{
    const Vector2f across = p_other - p_own;
    const Vector2f middle = (p_own + p_other) * 0.5f;
    return (((middle.x - p_point.x) * across.x) +
            ((middle.y - p_point.y) * across.y)) /
           std::sqrt((across.x * across.x) + (across.y * across.y));
}

// ----------------------------------------------------------------------------
CellHit hitCell(std::vector<Vector2f> const& p_sites, Vector2f p_point)
{
    const int gi = squareOf(p_point.x);
    const int gj = squareOf(p_point.y);
    CellHit hit;
    float best = 1.0e30f;
    for (int j = std::max(0, gj - 1); j <= std::min(CELLS - 1, gj + 1); ++j)
    {
        for (int i = std::max(0, gi - 1); i <= std::min(CELLS - 1, gi + 1); ++i)
        {
            const Vector2f d = p_point - p_sites[std::size_t(cellAt(i, j))];
            if (const float d2 = (d.x * d.x) + (d.y * d.y); d2 < best)
            {
                best = d2;
                hit.cell = cellAt(i, j);
            }
        }
    }
    const Vector2f own = p_sites[std::size_t(hit.cell)];
    for (const int other : neighbours(hit.cell))
    {
        if (const float edge =
                bisector(own, p_sites[std::size_t(other)], p_point);
            edge < hit.edge)
        {
            hit.edge = edge;
            hit.across = other;
        }
    }
    return hit;
}

// ----------------------------------------------------------------------------
//! Sutherland-Hodgman: each bisector keeps the side of the own site. The
//! sites two squares away can still cut a corner.
// ----------------------------------------------------------------------------
std::vector<Vector2f> cellPolygon(std::vector<Vector2f> const& p_sites,
                                  int p_cell)
{
    const Vector2f own = p_sites[std::size_t(p_cell)];
    const float reach = 2.0f * CELL_SIZE;
    std::vector polygon = { own + Vector2f(-reach, -reach),
                            own + Vector2f(reach, -reach),
                            own + Vector2f(reach, reach),
                            own + Vector2f(-reach, reach) };
    auto cut = [&](Vector2f p_site)
    {
        std::vector<Vector2f> kept;
        for (std::size_t k = 0u; k < polygon.size(); ++k)
        {
            const Vector2f a = polygon[k];
            const Vector2f b = polygon[(k + 1u) % polygon.size()];
            const float da = bisector(own, p_site, a);
            const float db = bisector(own, p_site, b);
            if (da >= 0.0f)
            {
                kept.push_back(a);
            }
            if ((da >= 0.0f) != (db >= 0.0f))
            {
                kept.push_back(a + ((b - a) * (da / (da - db))));
            }
        }
        polygon = std::move(kept);
    };
    const int ci = p_cell % CELLS;
    const int cj = p_cell / CELLS;
    for (int j = std::max(0, cj - 2); j <= std::min(CELLS - 1, cj + 2); ++j)
    {
        for (int i = std::max(0, ci - 2); i <= std::min(CELLS - 1, ci + 2); ++i)
        {
            if (cellAt(i, j) != p_cell)
            {
                cut(p_sites[std::size_t(cellAt(i, j))]);
            }
        }
    }
    return polygon;
}

} // namespace examples::archipelago
