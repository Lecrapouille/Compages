// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Land.hpp"
#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Making.hpp"
#include "10_Complete/Archipelago/Noise.hpp"

#include <algorithm>

namespace examples::archipelago
{

using compages::Vector2f;
using units::literals::operator""_m;

// ----------------------------------------------------------------------------
//! \brief The heights of the cells, read smoothly between their sites (a
//! Gaussian of the distance, over the 3 x 3 cells around).
// ----------------------------------------------------------------------------
static float cellHeight(Land const& p_land, float p_x, float p_z)
{
    constexpr float SPREAD = 1.0f / (0.62f * CELL_SIZE * 0.62f * CELL_SIZE);
    const int gi = squareOf(p_x);
    const int gj = squareOf(p_z);
    float sum = 0.0f;
    float weights = 0.0f;
    for (int j = std::max(0, gj - 1); j <= std::min(CELLS - 1, gj + 1); ++j)
    {
        for (int i = std::max(0, gi - 1); i <= std::min(CELLS - 1, gi + 1); ++i)
        {
            const auto c = std::size_t(cellAt(i, j));
            const Vector2f d = Vector2f(p_x, p_z) - p_land.sites[c];
            const float w = std::exp(-((d.x * d.x) + (d.y * d.y)) * SPREAD);
            sum += w * p_land.elevations[c];
            weights += w;
        }
    }
    return sum / std::max(weights, 1.0e-12f);
}

// ----------------------------------------------------------------------------
//! The cells smoothly, a little roughness on the slopes (none on the
//! beaches); then the river digs its valley (a smooth V that narrows to it,
//! as deep as the river is large) and its bed (banks a few metres high).
// ----------------------------------------------------------------------------
float Land::heightAt(float p_x, float p_z) const
{
    const float smooth = cellHeight(*this, p_x, p_z);
    float ground =
        smooth +
        (7.0f * (fbm(p_x / 140.0f, p_z / 140.0f, seed + 81u, 2) - 0.5f) *
         smoothstep(8.0f, 60.0f, smooth));
    float distance = 0.0f;
    float half = 0.0f;
    nearestRiver(p_x, p_z, distance, half);
    if (const float reach = valleyReach(half); distance < reach)
    {
        const float valley = 1.0f - (distance / reach);
        ground -= (compages::toMetres(6.0_m) + (0.6f * half)) * valley *
                  valley * (3.0f - (2.0f * valley));
        if (const float banks = half + compages::toMetres(4.0_m);
            distance < banks)
        {
            const float bed = distance / banks;
            ground -= (compages::toMetres(2.0_m) + (0.2f * half)) *
                      (1.0f - (bed * bed * bed * bed));
        }
    }
    return ground;
}

// ----------------------------------------------------------------------------
Place Land::placeOf(float p_x, float p_z) const
{
    const int cell = hitCell(sites, Vector2f(p_x, p_z)).cell;
    return { cell, regions_of[std::size_t(cell)], parts_of[std::size_t(cell)] };
}

// ----------------------------------------------------------------------------
std::vector<Vector2f> Land::cellPolygon(int p_cell) const
{
    return archipelago::cellPolygon(sites, p_cell);
}

// ----------------------------------------------------------------------------
Land makeLand(Recipe const& p_recipe, Progress const& p_progress)
{
    p_progress(0.0f, "Voronoi cells");
    Making making{ .recipe = p_recipe,
                   .progress = p_progress,
                   .placement = place(p_recipe),
                   .land = {},
                   .bends = {},
                   .shape = {},
                   .outlet = {},
                   .ground = {},
                   .drainage = {},
                   .on_river = {},
                   .paths = {} };
    making.land.seed = p_recipe.seed;
    making.land.sites = scatterSites(p_recipe.seed);
    shapeIslands(making);
    floodSea(making);
    divideRegions(making);
    raiseRelief(making);
    traceRivers(making);
    placeLinks(making);
    findPeaks(making);
    traceOutlines(making);
    p_progress(1.0f, "Map ready");
    return std::move(making.land);
}

} // namespace examples::archipelago
