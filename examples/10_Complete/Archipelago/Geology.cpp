// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Geology.hpp"
#include "10_Complete/Archipelago/Cells.hpp"

#include <algorithm>
#include <cmath>
#include <queue>

namespace examples::archipelago
{

using compages::Vector2f;

//! \brief Erodibility K (per year, for m = 1/2) and hillslope creep D (m2 a
//! year).
static constexpr float ERODIBILITY = 2.0e-5f;
static constexpr float CREEP = 0.05f;
//! \brief One step: 40 000 years.
static constexpr float YEARS = 4.0e4f;

// ----------------------------------------------------------------------------
Drainage drain(compages::Field<float> const& p_heights,
               compages::Field<char> const& p_outlet)
{
    const std::size_t count = p_heights.size();
    Drainage result;
    result.receivers.assign(count, -1);
    result.order.reserve(count);
    std::vector<float> level(p_heights.begin(), p_heights.end());
    std::vector<char> reached(count, 0);
    using Entry = std::pair<float, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> open;
    for (std::size_t c = 0u; c < count; ++c)
    {
        if (p_outlet[c] != 0)
        {
            reached[c] = 1;
            open.emplace(p_heights[c], int(c));
        }
    }
    while (!open.empty())
    {
        const int c = open.top().second;
        open.pop();
        result.order.push_back(c);
        for (const std::size_t n : CELL_GRID.neighbours8(std::size_t(c)))
        {
            if (reached[n] == 0)
            {
                reached[n] = 1;
                level[n] = std::max(p_heights[n], level[std::size_t(c)]);
                result.receivers[n] = c;
                open.emplace(level[n], int(n));
            }
        }
    }
    // Each cell gives its water to its receiver, from the sources down.
    result.areas.assign(count, CELL_SIZE * CELL_SIZE);
    for (auto it = result.order.rbegin(); it != result.order.rend(); ++it)
    {
        if (const int receiver = result.receivers[std::size_t(*it)];
            receiver >= 0)
        {
            result.areas[std::size_t(receiver)] +=
                result.areas[std::size_t(*it)];
        }
    }
    return result;
}

// ----------------------------------------------------------------------------
//! From the sea upward: the receiver already has its new height,
//!   h = (h + U dt + F h_receiver) / (1 + F),  F = K dt sqrt(A) / distance.
// ----------------------------------------------------------------------------
static void incise(compages::Field<Vector2f> const& p_sites,
                   compages::Field<float>& p_heights,
                   compages::Field<float> const& p_uplift,
                   compages::Field<char> const& p_outlet,
                   Drainage const& p_drainage)
{
    for (const int c : p_drainage.order)
    {
        const int receiver = p_drainage.receivers[std::size_t(c)];
        if ((receiver < 0) || (p_outlet[std::size_t(c)] != 0))
        {
            continue;
        }
        const Vector2f d =
            p_sites[std::size_t(c)] - p_sites[std::size_t(receiver)];
        const float distance = std::sqrt((d.x * d.x) + (d.y * d.y));
        const float factor = ERODIBILITY * YEARS *
                             std::sqrt(p_drainage.areas[std::size_t(c)]) /
                             distance;
        const float raised =
            p_heights[std::size_t(c)] + (p_uplift[std::size_t(c)] * YEARS);
        const float bed = p_heights[std::size_t(receiver)];
        // A lake (the receiver is higher) is not eroded: it fills.
        p_heights[std::size_t(c)] =
            (bed >= raised) ? raised
                            : ((raised + (factor * bed)) / (1.0f + factor));
    }
}

// ----------------------------------------------------------------------------
//! The slopes creep: D dt times the Laplacian of the 4 neighbours.
// ----------------------------------------------------------------------------
static void creep(compages::Field<float>& p_heights,
                  compages::Field<char> const& p_outlet)
{
    constexpr float RATE = CREEP * YEARS / (CELL_SIZE * CELL_SIZE);
    std::vector change(p_heights.size(), 0.0f);
    for (int j = 1; j < CELLS - 1; ++j)
    {
        for (int i = 1; i < CELLS - 1; ++i)
        {
            const auto c = CELL_GRID.index(i, j);
            if (p_outlet[c] == 0)
            {
                change[c] = RATE * (p_heights(i - 1, j) + p_heights(i + 1, j) +
                                    p_heights(i, j - 1) + p_heights(i, j + 1) -
                                    (4.0f * p_heights[c]));
            }
        }
    }
    for (std::size_t c = 0u; c < p_heights.size(); ++c)
    {
        p_heights[c] += change[c];
    }
}

// ----------------------------------------------------------------------------
Drainage erode(compages::Field<Vector2f> const& p_sites,
               compages::Field<float>& p_heights,
               compages::Field<float> const& p_uplift,
               compages::Field<char> const& p_outlet,
               int p_steps,
               Progress const& p_progress)
{
    for (int step = 0; step < p_steps; ++step)
    {
        incise(
            p_sites, p_heights, p_uplift, p_outlet, drain(p_heights, p_outlet));
        creep(p_heights, p_outlet);
        p_progress(float(step + 1) / float(std::max(p_steps, 1)),
                   "Uplift and erosion");
    }
    return drain(p_heights, p_outlet);
}

} // namespace examples::archipelago
