// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
// The relief of the land cells: the plate rises more inland (by steps of
// cells from the sea) and on the borders between regions, so the regions
// become basins between ridges, each one drained by its own river. The
// erosion (Geology.hpp) shapes it; the map then gives it its heights.
// ****************************************************************************

#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Making.hpp"
#include "10_Complete/Archipelago/Noise.hpp"

#include <algorithm>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_m;

//! \brief The uplift is (1 + RIDGE) times stronger on the borders between
//! regions: they become ridges, and the regions basins.
static constexpr float RIDGE = 0.8f;
//! \brief A land cell is at least this above the sea; the sea deepens away
//! from the coast below this.
static constexpr float SHORE = compages::toMetres(3.0_m);
static constexpr float SEA_FLOOR = compages::toMetres(6.0_m);

// ----------------------------------------------------------------------------
//! \brief Per cell, its steps of cells from the nearest outlet.
// ----------------------------------------------------------------------------
static std::vector<int> stepsInland(std::vector<char> const& p_outlet)
{
    std::vector<int> inland(CELL_COUNT, -1);
    std::vector<int> queue;
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (p_outlet[c] != 0)
        {
            inland[c] = 0;
            queue.push_back(int(c));
        }
    }
    for (std::size_t head = 0u; head < queue.size(); ++head)
    {
        const int c = queue[head];
        for (const int n : neighbours(c))
        {
            if (inland[std::size_t(n)] < 0)
            {
                inland[std::size_t(n)] = inland[std::size_t(c)] + 1;
                queue.push_back(n);
            }
        }
    }
    return inland;
}

// ----------------------------------------------------------------------------
//! \brief Per cell, 1 on a border between two regions, fading over two
//! cells around.
// ----------------------------------------------------------------------------
static std::vector<float> ridgesOf(std::vector<int> const& p_regions_of)
{
    std::vector<float> ridge(CELL_COUNT, 0.0f);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const int region = p_regions_of[c];
        const Around around = neighbours(int(c));
        if ((region >= 0) &&
            std::any_of(around.begin(),
                        around.end(),
                        [&](int p_n)
                        {
                            const int other = p_regions_of[std::size_t(p_n)];
                            return (other >= 0) && (other != region);
                        }))
        {
            ridge[c] = 1.0f;
        }
    }
    for (int pass = 0; pass < 2; ++pass)
    {
        std::vector<float> wider(ridge);
        for (std::size_t c = 0u; c < CELL_COUNT; ++c)
        {
            for (const int n : neighbours(int(c)))
            {
                wider[c] = std::max(wider[c], 0.5f * ridge[std::size_t(n)]);
            }
        }
        ridge = std::move(wider);
    }
    return ridge;
}

// ----------------------------------------------------------------------------
void raiseRelief(Making& p_making)
{
    Land& land = p_making.land;
    std::vector<char> const& outlet = p_making.outlet;
    const std::uint32_t seed = p_making.recipe.seed;
    const std::vector<int> inland = stepsInland(outlet);
    std::vector<int> deepest(land.islands.size(), 1);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if ((outlet[c] == 0) && (land.islands_of[c] >= 0))
        {
            int& island = deepest[std::size_t(land.islands_of[c])];
            island = std::max(island, inland[c]);
        }
    }
    const std::vector<float> ridge = ridgesOf(land.regions_of);
    p_making.ground.assign(CELL_COUNT, 0.0f);
    std::vector<float> uplift(CELL_COUNT, 0.0f);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (outlet[c] != 0)
        {
            continue;
        }
        const Vector2f p = land.sites[c];
        const float massif =
            0.4f + (1.2f * fbm(p.x / 2200.0f, p.y / 2200.0f, seed + 61u, 3));
        const auto island = std::size_t(std::max(0, int(land.islands_of[c])));
        const float heart =
            smoothstep(0.0f,
                       std::max(2.0f, 0.7f * float(deepest[island])),
                       float(inland[c]));
        uplift[c] = UPLIFT * massif * (0.25f + (0.75f * heart)) *
                    (1.0f + (RIDGE * ridge[c]));
        p_making.ground[c] =
            4.0f * noise(p.x / 400.0f, p.y / 400.0f, seed + 71u);
    }
    p_making.drainage = erode(land.sites,
                              p_making.ground,
                              uplift,
                              outlet,
                              p_making.recipe.erosion_steps,
                              [&p_making](float p_done, char const* p_what)
                              { p_making.progress(0.55f * p_done, p_what); });

    // The shape of the relief is the model's; its height is the map's.
    const float tallest =
        std::max(1.0f, *std::ranges::max_element(p_making.ground));
    land.elevations.resize(CELL_COUNT);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (outlet[c] == 0)
        {
            land.elevations[c] =
                SHORE + (p_making.ground[c] * ((0.92f * PEAK) / tallest));
        }
        else
        {
            land.elevations[c] =
                (-SEABED * smoothstep(0.0f, 0.14f, -p_making.shape[c])) -
                SEA_FLOOR;
            land.islands_of[c] = std::int8_t(-1);
        }
    }
}

// ----------------------------------------------------------------------------
void findPeaks(Making& p_making)
{
    Land& land = p_making.land;
    std::vector<float> highest(land.islands.size(), -1.0e9f);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const int k = land.islands_of[c];
        if ((k >= 0) && (land.elevations[c] > highest[std::size_t(k)]))
        {
            highest[std::size_t(k)] = land.elevations[c];
            land.islands[std::size_t(k)].peak =
                Vector3f(land.sites[c].x, land.elevations[c], land.sites[c].y);
        }
    }
}

} // namespace examples::archipelago
