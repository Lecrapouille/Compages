// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
// "Noems as loci along a strand" (the thesis): the main river is the
// highest place of an island; the tributaries are the places of its regions;
// the cells along a tributary hold the links.
//
// - The main river of an island goes up from its mouth (the coast cell that
//   drains most), always to the cell that drains most.
// - The tributary of a region runs down from its highest cell, along the
//   drainage, until a river or the sea. The highest regions first: a lower
//   one may join them.
// - The links of a region take its cells nearest the tributary, both sorted
//   along it: neighbours in the graph stay neighbours on the map.
// ****************************************************************************

#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Making.hpp"

#include <algorithm>
#include <numeric>

namespace examples::archipelago
{

using compages::Vector2f;

//! \brief The main river goes up while it drains at least five cells.
static constexpr float STRAND_AREA = 5.0f * CELL_SIZE * CELL_SIZE;

// ----------------------------------------------------------------------------
//! \brief A river through cells given from its mouth up, to an end point.
// ----------------------------------------------------------------------------
static void
addRiver(Making& p_making, std::vector<int> const& p_upstream, Vector2f p_end)
{
    std::vector<Vector2f> points;
    std::vector<float> widths;
    for (auto it = p_upstream.rbegin(); it != p_upstream.rend(); ++it)
    {
        points.push_back(p_making.land.sites[std::size_t(*it)]);
        widths.push_back(riverWidth(p_making.drainage.areas[std::size_t(*it)]));
    }
    points.push_back(p_end);
    widths.push_back(widths.back());
    p_making.land.rivers.push_back(
        makeRiver(points, widths, p_making.recipe.seed));
}

// ----------------------------------------------------------------------------
//! \brief Where a river ends in the sea: past the coast, between its last
//! cell and the sea cell it drains to.
// ----------------------------------------------------------------------------
static Vector2f mouthOf(Making const& p_making, int p_last)
{
    const int sea = p_making.drainage.receivers[std::size_t(p_last)];
    return (p_making.land.sites[std::size_t(p_last)] * 0.3f) +
           (p_making.land.sites[std::size_t(sea)] * 0.7f);
}

// ----------------------------------------------------------------------------
//! \brief Where a tributary meets the river it joins: the point of the
//! rivers already traced nearest the centre of the cell it flows into. The
//! rivers meander away from the centres of their cells (by up to about two
//! widths), so a tributary ending on the centre itself would stop short of
//! the water.
// ----------------------------------------------------------------------------
static Vector2f confluence(std::vector<River> const& p_rivers, Vector2f p_point)
{
    Vector2f best = p_point;
    float nearest = 1.0e30f;
    for (River const& river : p_rivers)
    {
        for (const Vector2f q : river.points)
        {
            const Vector2f d = q - p_point;
            if (const float d2 = (d.x * d.x) + (d.y * d.y); d2 < nearest)
            {
                nearest = d2;
                best = q;
            }
        }
    }
    return best;
}

// ----------------------------------------------------------------------------
//! \brief The coast cell of an island that drains most, -1 for none.
// ----------------------------------------------------------------------------
static int mouthCell(Making const& p_making, std::size_t p_island)
{
    int mouth = -1;
    float largest = 0.0f;
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const int receiver = p_making.drainage.receivers[c];
        const bool coast =
            (receiver >= 0) && (p_making.outlet[std::size_t(receiver)] != 0);
        if ((p_making.land.islands_of[c] == std::int8_t(p_island)) &&
            (p_making.outlet[c] == 0) && coast &&
            (p_making.drainage.areas[c] > largest))
        {
            largest = p_making.drainage.areas[c];
            mouth = int(c);
        }
    }
    return mouth;
}

// ----------------------------------------------------------------------------
//! \brief From a cell up the river: always the donor that drains most, while
//! it drains at least STRAND_AREA.
// ----------------------------------------------------------------------------
static std::vector<int> upstream(Making& p_making,
                                 std::vector<std::vector<int>> const& p_donors,
                                 int p_from)
{
    std::vector<int> path{ p_from };
    p_making.on_river[std::size_t(p_from)] = 1;
    for (int c = p_from;;)
    {
        int best = -1;
        float area = STRAND_AREA;
        for (const int d : p_donors[std::size_t(c)])
        {
            if ((p_making.on_river[std::size_t(d)] == 0) &&
                (p_making.drainage.areas[std::size_t(d)] >= area))
            {
                area = p_making.drainage.areas[std::size_t(d)];
                best = d;
            }
        }
        if (best < 0)
        {
            return path;
        }
        path.push_back(best);
        p_making.on_river[std::size_t(best)] = 1;
        c = best;
    }
}

// ----------------------------------------------------------------------------
//! \brief The main river of each island.
// ----------------------------------------------------------------------------
static void traceMainRivers(Making& p_making)
{
    std::vector<std::vector<int>> donors(CELL_COUNT);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (const int receiver = p_making.drainage.receivers[c];
            (p_making.outlet[c] == 0) && (receiver >= 0))
        {
            donors[std::size_t(receiver)].push_back(int(c));
        }
    }
    for (std::size_t k = 0u; k < p_making.land.islands.size(); ++k)
    {
        if (const int mouth = mouthCell(p_making, k); mouth >= 0)
        {
            addRiver(p_making,
                     upstream(p_making, donors, mouth),
                     mouthOf(p_making, mouth));
        }
    }
}

// ----------------------------------------------------------------------------
//! \brief The tributary of a region, from its source down to a river or the
//! sea; kept from its mouth to its source.
// ----------------------------------------------------------------------------
static void traceTributary(Making& p_making, std::size_t p_region, int p_source)
{
    std::vector<int> down;
    for (int c = p_source;
         (c >= 0) && (p_making.on_river[std::size_t(c)] == 0) &&
         (p_making.outlet[std::size_t(c)] == 0);
         c = p_making.drainage.receivers[std::size_t(c)])
    {
        down.push_back(c);
    }
    if (down.empty())
    {
        return;
    }
    for (const int cell : down)
    {
        p_making.on_river[std::size_t(cell)] = 1;
    }
    const int last = down.back();
    const int into = p_making.drainage.receivers[std::size_t(last)];
    const bool joins = (into >= 0) && (p_making.outlet[std::size_t(into)] == 0);
    p_making.paths[p_region].assign(down.rbegin(), down.rend());
    addRiver(p_making,
             p_making.paths[p_region],
             joins ? confluence(p_making.land.rivers,
                                p_making.land.sites[std::size_t(into)])
                   : mouthOf(p_making, last));
}

// ----------------------------------------------------------------------------
void traceRivers(Making& p_making)
{
    p_making.progress(0.6f, "Rivers");
    p_making.on_river = compages::Field<char>(CELL_GRID, char(0));
    p_making.paths.assign(p_making.land.regions.size(), {});
    traceMainRivers(p_making);

    // The source of each region: its highest cell off the rivers.
    compages::Field<float> const& ground = p_making.ground;
    std::vector<int> sources(p_making.land.regions.size(), -1);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const int r = p_making.land.regions_of[c];
        if ((r < 0) || (p_making.on_river[c] != 0))
        {
            continue;
        }
        if (int& source = sources[std::size_t(r)];
            (source < 0) || (ground[c] > ground[std::size_t(source)]))
        {
            source = int(c);
        }
    }
    std::vector<std::size_t> order;
    for (std::size_t r = 0u; r < sources.size(); ++r)
    {
        if (sources[r] >= 0)
        {
            order.push_back(r);
        }
    }
    std::ranges::sort(order,
                      [&](std::size_t p_a, std::size_t p_b)
                      {
                          return ground[std::size_t(sources[p_a])] >
                                 ground[std::size_t(sources[p_b])];
                      });
    for (const std::size_t r : order)
    {
        traceTributary(p_making, r, sources[r]);
    }
    p_making.land.river_index.build(p_making.land.rivers);
}

// ----------------------------------------------------------------------------
//! \brief Per cell of a region, its steps of cells from the strand, inside
//! the region; a large number where the strand is not reached.
// ----------------------------------------------------------------------------
static compages::Field<int>
stepsFrom(Land const& p_land, std::vector<int> const& p_strand, int p_region)
{
    compages::Field<int> steps(CELL_GRID, int(CELL_COUNT));
    std::vector<int> queue(p_strand);
    for (const int c : p_strand)
    {
        steps[std::size_t(c)] = 0;
    }
    for (std::size_t head = 0u; head < queue.size(); ++head)
    {
        const int c = queue[head];
        for (const std::size_t n : CELL_GRID.neighbours8(std::size_t(c)))
        {
            if ((p_land.regions_of[n] == p_region) &&
                (steps[n] > steps[std::size_t(c)] + 1))
            {
                steps[n] = steps[std::size_t(c)] + 1;
                queue.push_back(int(n));
            }
        }
    }
    return steps;
}

// ----------------------------------------------------------------------------
//! \brief Where a point is along a strand: the rank of its nearest cell.
// ----------------------------------------------------------------------------
static std::size_t
along(Land const& p_land, std::vector<int> const& p_strand, Vector2f p_point)
{
    std::size_t best = 0u;
    float nearest = 1.0e30f;
    for (std::size_t k = 0u; k < p_strand.size(); ++k)
    {
        const Vector2f d = p_point - p_land.sites[std::size_t(p_strand[k])];
        if (const float d2 = (d.x * d.x) + (d.y * d.y); d2 < nearest)
        {
            nearest = d2;
            best = k;
        }
    }
    return best;
}

// ----------------------------------------------------------------------------
//! \brief The links of one region on its cells nearest its tributary.
// ----------------------------------------------------------------------------
static void placeRegionLinks(Making& p_making, std::size_t p_region)
{
    Land& land = p_making.land;
    const auto r = int(p_region);
    std::vector<int> strand;
    std::ranges::copy_if(p_making.paths[p_region],
                         std::back_inserter(strand),
                         [&](int p_c)
                         { return land.regions_of[std::size_t(p_c)] == r; });
    std::vector<int> cells;
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (land.regions_of[c] == r)
        {
            cells.push_back(int(c));
        }
    }
    // A region the land has no room for: the cell under its node.
    if (cells.empty())
    {
        cells.push_back(
            hitCell(land.sites, p_making.placement.regions[p_region]).cell);
    }
    if (strand.empty())
    {
        strand.push_back(cells.front());
    }
    // As many cells as links, the nearest to the strand; both sorted along it.
    const compages::Field<int> steps = stepsFrom(land, strand, r);
    std::ranges::stable_sort(
        cells, {}, [&](int p_c) { return steps[std::size_t(p_c)]; });
    std::vector<Vector2f> const& links = p_making.placement.links[p_region];
    cells.resize(std::min(cells.size(), links.size()));
    std::ranges::stable_sort(
        cells,
        {},
        [&](int p_c)
        { return along(land, strand, land.sites[std::size_t(p_c)]); });
    std::vector<std::size_t> ranks(links.size());
    std::iota(ranks.begin(), ranks.end(), std::size_t(0));
    std::ranges::stable_sort(ranks,
                             {},
                             [&](std::size_t p_k)
                             { return along(land, strand, links[p_k]); });
    // More links than cells: the cells are shared, in turn.
    Region& region = land.regions[p_region];
    region.parts.resize(links.size());
    for (std::size_t n = 0u; n < ranks.size(); ++n)
    {
        const int cell = cells[(n * cells.size()) / ranks.size()];
        region.parts[ranks[n]] = Part{ cell, land.sites[std::size_t(cell)] };
        if (land.parts_of[std::size_t(cell)] < 0)
        {
            land.parts_of[std::size_t(cell)] = int(ranks[n]);
        }
    }
    region.site = land.sites[std::size_t(strand[strand.size() / 2u])];
}

// ----------------------------------------------------------------------------
void placeLinks(Making& p_making)
{
    p_making.progress(0.7f, "Regions");
    for (std::size_t r = 0u; r < p_making.land.regions.size(); ++r)
    {
        placeRegionLinks(p_making, r);
    }
}

} // namespace examples::archipelago
