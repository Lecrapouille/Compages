// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
// The outline of each region: traced on a grid of 31 m around its cells,
// where the distance to the border of the region is zero (marching squares),
// on the ground as the meshes show it.
// ****************************************************************************

#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Making.hpp"

#include <algorithm>
#include <array>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_m;

//! \brief The grid the outlines are traced on: 31 m.
static constexpr int GRID = 512;
static constexpr float STEP = MAP / float(GRID);
static constexpr std::size_t SIDE = std::size_t(GRID + 1);
//! \brief An outline floats this far over the ground.
static constexpr float LIFT = compages::toMetres(4.0_m);

//! \brief The corners of the grid to look at: i0, j0, i1, j1.
using Box = std::array<int, 4>;

static Vector2f cornerAt(int p_i, int p_j)
{
    return { (-0.5f * MAP) + (float(p_i) * STEP),
             (-0.5f * MAP) + (float(p_j) * STEP) };
}

// ----------------------------------------------------------------------------
//! \brief The segments of the outline in one square of the grid: where the
//! field changes sign along its edges.
// ----------------------------------------------------------------------------
static void traceSquare(Land const& p_land,
                        std::vector<float> const& p_field,
                        int p_i,
                        int p_j,
                        std::vector<Vector3f>& p_segments)
{
    const std::array<std::array<int, 2>, 4> corners = { { { p_i, p_j },
                                                          { p_i + 1, p_j },
                                                          { p_i + 1, p_j + 1 },
                                                          { p_i, p_j + 1 } } };
    std::array<Vector3f, 4> crossings;
    std::size_t found = 0u;
    for (std::size_t e = 0u; e < 4u; ++e)
    {
        const std::array<int, 2> a = corners[e];
        const std::array<int, 2> b = corners[(e + 1u) % 4u];
        const float fa =
            p_field[(std::size_t(a[1]) * SIDE) + std::size_t(a[0])];
        const float fb =
            p_field[(std::size_t(b[1]) * SIDE) + std::size_t(b[0])];
        if ((fa > 0.0f) != (fb > 0.0f))
        {
            const Vector2f p = cornerAt(a[0], a[1]) +
                               ((cornerAt(b[0], b[1]) - cornerAt(a[0], a[1])) *
                                (fa / (fa - fb)));
            crossings[found] = Vector3f(
                p.x, std::max(0.0f, p_land.heightAt(p.x, p.y)) + LIFT, p.y);
            ++found;
        }
    }
    for (std::size_t k = 0u; (k + 1u) < found; k += 2u)
    {
        p_segments.push_back(crossings[k]);
        p_segments.push_back(crossings[k + 1u]);
    }
}

// ----------------------------------------------------------------------------
//! \brief The corners of the grid around the cells of a region, with a
//! margin; empty (i1 < i0) for a region without cells.
// ----------------------------------------------------------------------------
static Box boxOf(Land const& p_land, int p_region)
{
    Box box = { GRID, GRID, 0, 0 };
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (p_land.regions_of[c] == p_region)
        {
            const auto i = int((p_land.sites[c].x + (0.5f * MAP)) / STEP);
            const auto j = int((p_land.sites[c].y + (0.5f * MAP)) / STEP);
            box = { std::min(box[0], i),
                    std::min(box[1], j),
                    std::max(box[2], i),
                    std::max(box[3], j) };
        }
    }
    const int margin = int(CELL_SIZE / STEP) + 2;
    return { std::max(0, box[0] - margin),
             std::max(0, box[1] - margin),
             std::min(GRID, box[2] + margin),
             std::min(GRID, box[3] + margin) };
}

// ----------------------------------------------------------------------------
void traceOutlines(Making& p_making)
{
    Land& land = p_making.land;
    std::vector<float> field(SIDE * SIDE, -1.0f);
    for (std::size_t r = 0u; r < land.regions.size(); ++r)
    {
        p_making.progress(0.75f +
                              (0.25f * float(r) / float(land.regions.size())),
                          "Outlines");
        const auto region = int(r);
        const Box box = boxOf(land, region);
        if (box[2] < box[0])
        {
            continue;
        }
        auto member = [&land, region](int p_c)
        { return land.regions_of[std::size_t(p_c)] == region; };
        for (int j = box[1]; j <= box[3]; ++j)
        {
            for (int i = box[0]; i <= box[2]; ++i)
            {
                field[(std::size_t(j) * SIDE) + std::size_t(i)] =
                    groupField(land.sites, cornerAt(i, j), member);
            }
        }
        std::vector<Vector3f>& segments = land.regions[r].border;
        for (int j = box[1]; j < box[3]; ++j)
        {
            for (int i = box[0]; i < box[2]; ++i)
            {
                traceSquare(land, field, i, j, segments);
            }
        }
    }
}

} // namespace examples::archipelago
