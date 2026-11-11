// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

// ****************************************************************************
// The islands around the graph of the contents. Each island gets a disc
// where the layout put it (Placement.hpp). The land is made of shapes on the
// graph: a core on the middle of the disc, discs on the nodes, capsules
// along the edges (island to region, region to link); many links, smaller
// shapes, so the island keeps its size. Each cell takes the highest shape,
// on coordinates bent by noise (Patel): the coasts wave. Each land cell then
// belongs to the region of the nearest link of its island.
// ****************************************************************************

#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Making.hpp"
#include "10_Complete/Archipelago/Noise.hpp"

#include <algorithm>
#include <cmath>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_m;

//! \brief The noise that bends the coasts (at most: less on small
//! islands), and the borders between regions.
static constexpr float COAST_BEND = compages::toMetres(1100.0_m);
static constexpr float BORDER_BEND = compages::toMetres(220.0_m);
//! \brief The sea keeps this far from the edges of the map.
static constexpr float EDGE_SEA = compages::toMetres(1200.0_m);

//! \brief A round shape of land: a disc (a == b) or a capsule.
struct Shape
{
    Vector2f a;
    Vector2f b;
    float radius;
    std::size_t island;
};

// ----------------------------------------------------------------------------
//! \brief The shapes of the land: per island its core and its node, per
//! region its node and its edge to the island, per link its node and its
//! edge to the region.
// ----------------------------------------------------------------------------
static std::vector<Shape> shapesOf(Recipe const& p_recipe,
                                   Placement const& p_placement)
{
    std::vector<Shape> shapes;
    std::vector<float> links(p_placement.centres.size(), 0.0f);
    for (Recipe::Group const& region : p_recipe.regions)
    {
        links[region.island] += float(region.links.size());
    }
    // The size of the shape of a link: the island shared by its links.
    std::vector<float> sizes(links.size());
    for (std::size_t k = 0u; k < links.size(); ++k)
    {
        const float rho = p_placement.radii[k];
        const float smallest = 2.0f * CELL_SIZE;
        sizes[k] = std::clamp((1.5f * rho) / std::sqrt(links[k] + 1.0f),
                              smallest,
                              std::max(smallest, 0.6f * rho));
        shapes.push_back(Shape{
            p_placement.centres[k], p_placement.centres[k], 0.5f * rho, k });
        shapes.push_back(Shape{ p_placement.islands[k],
                                p_placement.islands[k],
                                1.2f * sizes[k],
                                k });
    }
    for (std::size_t r = 0u; r < p_recipe.regions.size(); ++r)
    {
        const std::size_t k = p_recipe.regions[r].island;
        const Vector2f node = p_placement.regions[r];
        shapes.push_back(
            Shape{ p_placement.islands[k], node, 0.8f * sizes[k], k });
        shapes.push_back(Shape{ node, node, 1.3f * sizes[k], k });
        for (const Vector2f link : p_placement.links[r])
        {
            shapes.push_back(Shape{ node, link, 0.7f * sizes[k], k });
            shapes.push_back(Shape{ link, link, sizes[k], k });
        }
    }
    return shapes;
}

// ----------------------------------------------------------------------------
//! \brief How much a shape covers a point: 1 on its axis, 0 at its radius.
// ----------------------------------------------------------------------------
static float cover(Shape const& p_shape, Vector2f p_point)
{
    const Vector2f ab = p_shape.b - p_shape.a;
    const float ab2 = std::max((ab.x * ab.x) + (ab.y * ab.y), 1.0e-6f);
    const Vector2f ap = p_point - p_shape.a;
    const float t =
        std::clamp(((ap.x * ab.x) + (ap.y * ab.y)) / ab2, 0.0f, 1.0f);
    const Vector2f d = ap - (ab * t);
    const float t2 =
        ((d.x * d.x) + (d.y * d.y)) / (p_shape.radius * p_shape.radius);
    return (t2 < 1.0f) ? ((1.0f - t2) * (1.0f - t2)) : 0.0f;
}

// ----------------------------------------------------------------------------
void shapeIslands(Making& p_making)
{
    Land& land = p_making.land;
    const std::uint32_t seed = p_making.recipe.seed;
    for (std::size_t k = 0u; k < p_making.placement.centres.size(); ++k)
    {
        land.islands.push_back(Island{ p_making.placement.centres[k],
                                       p_making.placement.radii[k],
                                       Vector3f(0.0f, 0.0f, 0.0f) });
    }
    p_making.bends = compages::Field<compages::Vector2f>(CELL_GRID, compages::Vector2f(0.0f, 0.0f));
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const Vector2f p = land.sites[c];
        p_making.bends[c] = Vector2f(
            (2.0f * fbm(p.x / 2600.0f, p.y / 2600.0f, seed + 11u, 4)) - 1.0f,
            (2.0f * fbm(p.x / 2600.0f, p.y / 2600.0f, seed + 12u, 4)) - 1.0f);
    }

    // Each shape covers the cells of its box; a cell keeps the highest cover.
    std::vector<float> bumps(CELL_COUNT, 0.0f);
    land.islands_of = compages::Field<std::int8_t>(CELL_GRID, std::int8_t(-1));
    for (Shape const& shape : shapesOf(p_making.recipe, p_making.placement))
    {
        const float bend =
            std::min(COAST_BEND, 0.35f * land.islands[shape.island].radius);
        const float reach = shape.radius + bend + CELL_SIZE;
        const int i0 = squareOf(std::min(shape.a.x, shape.b.x) - reach);
        const int i1 = squareOf(std::max(shape.a.x, shape.b.x) + reach);
        const int j0 = squareOf(std::min(shape.a.y, shape.b.y) - reach);
        const int j1 = squareOf(std::max(shape.a.y, shape.b.y) + reach);
        for (int j = j0; j <= j1; ++j)
        {
            for (int i = i0; i <= i1; ++i)
            {
                const auto c = CELL_GRID.index(i, j);
                if (const float f = cover(
                        shape, land.sites[c] + (p_making.bends[c] * bend));
                    f > bumps[c])
                {
                    bumps[c] = f;
                    land.islands_of[c] = std::int8_t(shape.island);
                }
            }
        }
    }
    // Rough coasts; deep sea along the edges of the map.
    p_making.shape = compages::Field<float>(CELL_GRID, 0.0f);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const Vector2f p = land.sites[c];
        const float edge =
            (0.5f * MAP) - std::max(std::abs(p.x), std::abs(p.y));
        const float rough = 0.12f + (0.4f * std::min(1.0f, 3.0f * bumps[c]));
        p_making.shape[c] =
            bumps[c] +
            (rough * (fbm(p.x / 1500.0f, p.y / 1500.0f, seed, 5) - 0.5f)) -
            0.12f - (0.6f * std::max(0.0f, 1.0f - (edge / EDGE_SEA)));
    }
}

// ----------------------------------------------------------------------------
//! The ocean is the water reached from the edge (4 neighbours); water closed
//! in by land would be a lake: it is filled (no lakes on this map). The
//! water leaves the land by the ocean and by the edges of the map.
// ----------------------------------------------------------------------------
void floodSea(Making& p_making)
{
    std::vector<char> sea(CELL_COUNT, 0);
    std::vector<int> open;
    auto reach = [&](int p_i, int p_j)
    {
        if ((p_i < 0) || (p_j < 0) || (p_i >= CELLS) || (p_j >= CELLS))
        {
            return;
        }
        const auto c = int(CELL_GRID.index(p_i, p_j));
        if ((sea[std::size_t(c)] == 0) &&
            (p_making.shape[std::size_t(c)] <= 0.0f))
        {
            sea[std::size_t(c)] = 1;
            open.push_back(c);
        }
    };
    for (int k = 0; k < CELLS; ++k)
    {
        reach(k, 0);
        reach(k, CELLS - 1);
        reach(0, k);
        reach(CELLS - 1, k);
    }
    while (!open.empty())
    {
        const int c = open.back();
        open.pop_back();
        const int ci = c % CELLS;
        const int cj = c / CELLS;
        reach(ci + 1, cj);
        reach(ci - 1, cj);
        reach(ci, cj + 1);
        reach(ci, cj - 1);
    }
    p_making.outlet = compages::Field<char>(CELL_GRID, char(0));
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        const auto i = int(c) % CELLS;
        const auto j = int(c) / CELLS;
        const bool edge =
            (i == 0) || (j == 0) || (i == CELLS - 1) || (j == CELLS - 1);
        p_making.outlet[c] = ((sea[c] != 0) || edge) ? 1 : 0;
    }
}

// ----------------------------------------------------------------------------
//! On coordinates a little bent, so the borders wave. A region is so the
//! ground of its links, where the layout put them.
// ----------------------------------------------------------------------------
void divideRegions(Making& p_making)
{
    Land& land = p_making.land;
    struct Node
    {
        Vector2f at;
        int region;
    };
    std::vector<std::vector<Node>> nodes(land.islands.size());
    for (std::size_t r = 0u; r < p_making.recipe.regions.size(); ++r)
    {
        land.regions.push_back(
            Region{ .island = p_making.recipe.regions[r].island,
                    .site = {},
                    .border = {},
                    .parts = {} });
        for (const Vector2f link : p_making.placement.links[r])
        {
            nodes[p_making.recipe.regions[r].island].push_back(
                Node{ link, int(r) });
        }
    }
    land.regions_of = compages::Field<int>(CELL_GRID, -1);
    land.parts_of = compages::Field<int>(CELL_GRID, -1);
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if ((p_making.outlet[c] != 0) || (land.islands_of[c] < 0))
        {
            continue;
        }
        const Vector2f p = land.sites[c] + (p_making.bends[c] * BORDER_BEND);
        float best = 1.0e30f;
        for (Node const& node : nodes[std::size_t(land.islands_of[c])])
        {
            const Vector2f d = p - node.at;
            if (const float d2 = (d.x * d.x) + (d.y * d.y); d2 < best)
            {
                best = d2;
                land.regions_of[c] = node.region;
            }
        }
    }
}

} // namespace examples::archipelago
