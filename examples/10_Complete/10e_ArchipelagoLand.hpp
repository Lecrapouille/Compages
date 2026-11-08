// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The land of 10e_Archipelago, made on the CPU from a seed, in
//! metres. Nothing here touches the GPU, so it can run in a thread.
//!
//! The method is the one Chréage asked for (I. Aberkane, "Mind Ergonomy for
//! the Knowledge Economy", 2016, chapter 11), after A. Patel's "Polygonal
//! Map Generation": the map is a Voronoi diagram of a few thousand cells;
//! each cell is land or sea; the relief of the cells grows by uplift and
//! river erosion on the graph of the cells (Cordonnier et al. 2016).
//!
//! The thesis maps the contents "as loci along a strand": a river is a
//! strand of places, each place a strand of smaller places, down to the
//! last ones, which hold the contents. So here:
//! - an island (a theme) is structured by its main river, the largest basin
//!   the erosion made;
//! - a region (a topic) is a tributary of that river, with the cells around
//!   it, in the order of the river from its mouth to its source;
//! - a link is a cell on the bank of the tributary.
//! Only these rivers are drawn; they dig their bed and their valley in the
//! smooth ground between the cells.
// ****************************************************************************

#include "Compages/Core/Vector.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <vector>

namespace examples::archipelago
{

//! \brief The side of the map, in metres, centred on the origin, X east, -Z
//! north.
inline constexpr float MAP = 16000.0f;
//! \brief Voronoi cells along a side: 96 x 96 cells of 167 m.
inline constexpr int CELLS = 96;
inline constexpr float CELL_SIZE = MAP / float(CELLS);
//! \brief The highest ground and the deepest sea of the map.
inline constexpr float PEAK = 520.0f;
inline constexpr float SEABED = 90.0f;
//! \brief The alpha of the picture says what the shader of the map draws:
//! the sea from 0 (the shore) to SEA_ALPHA (SEABED deep and more), a river
//! at RIVER_ALPHA, the land at 1.
inline constexpr float SEA_ALPHA = 0.9f;
inline constexpr float RIVER_ALPHA = 0.94f;

//! \brief What a map is made from: the islands, and how many regions and
//! parts each one has.
struct Recipe
{
    std::uint32_t seed;
    //! \brief Where each island is and how large, in metres, before the
    //! seed moves it a little.
    std::vector<compages::Vector2f> centres;
    std::vector<float> radii;
    //! \brief Per region: its island, and how many parts it is cut into.
    std::vector<std::size_t> region_islands;
    std::vector<std::size_t> region_parts;
    //! \brief Steps of the erosion (40 000 years each): older land, deeper
    //! valleys.
    int erosion_steps;
};

//! \brief A link of a region: a cell on the bank of its tributary.
struct Part
{
    //! \brief Its cell, and the site of the cell, in metres (x, z).
    int cell;
    compages::Vector2f site;
};

//! \brief A region of an island: a tributary of its main river and the
//! cells around it.
struct Region
{
    std::size_t island;
    //! \brief The middle of its tributary: where its name is written.
    compages::Vector2f site;
    //! \brief Its outline, two points per segment, over the ground.
    std::vector<compages::Vector3f> border;
    //! \brief Its links, from the mouth of its tributary to its source.
    std::vector<Part> parts;
};

//! \brief An island, as the seed made it.
struct Island
{
    compages::Vector2f centre;
    float radius;
    //! \brief Its highest point.
    compages::Vector3f peak;
};

//! \brief A river from its source to the sea or to the river it joins: a
//! smooth spline through the cells it flows through, then its meanders.
struct River
{
    std::vector<compages::Vector2f> points;
    //! \brief Its width at each point, in metres.
    std::vector<float> widths;
};

//! \brief Where a point is: its cell, its region, its link (-1 for none).
struct Place
{
    int cell = -1;
    int region = -1;
    int part = -1;
};

//! \brief A square of the map, ready to become a mesh and a texture.
struct Patch
{
    //! \brief Its corner (smallest x and z) and its side, in metres.
    float x;
    float z;
    float size;
    //! \brief Quads along a side. The heights are (cells + 3)^2: one more
    //! ring of corners around it, for the normals of its edges.
    int cells;
    std::vector<float> heights;
    //! \brief Pixels along a side, and the picture: sRGB, first row at the
    //! smallest z, alpha as SEA_ALPHA and RIVER_ALPHA say.
    int pixels;
    std::vector<std::byte> picture;

    //! \brief The height of corner (i, j) of the patch, i and j from -1 to
    //! cells + 1.
    [[nodiscard]] float height(int p_i, int p_j) const
    {
        return heights[std::size_t(((p_j + 1) * (cells + 3)) + p_i + 1)];
    }
};

//! \brief How far a long job has gone, from 0 to 1, and what it does.
using Progress = std::function<void(float, char const*)>;

// ****************************************************************************
//! \brief A whole map: the cells, the regions, the rivers.
// ****************************************************************************
struct Land
{
    std::uint32_t seed = 0u;
    //! \brief Per cell (CELLS x CELLS, row after row along +Z): its site, its
    //! height, its island (-1: none), its region and its link (-1: none).
    std::vector<compages::Vector2f> sites;
    std::vector<float> elevations;
    std::vector<std::int8_t> islands_of;
    std::vector<int> regions_of;
    std::vector<int> parts_of;
    std::vector<Island> islands;
    std::vector<Region> regions;
    //! \brief The main river of each island and the tributary of each
    //! region: the only rivers of the map.
    std::vector<River> rivers;

    //! \brief One segment of a river, for the rivers near a point.
    struct Segment
    {
        compages::Vector2f a;
        compages::Vector2f b;
        float width_a;
        float width_b;
    };
    std::vector<Segment> segments;
    //! \brief The segments whose valley reaches each square of a 64 x 64
    //! grid over the map.
    std::vector<std::vector<int>> buckets;

    //! \brief The ground at a point: the cells, smoothly, a little roughness,
    //! and the bed and the valley of the rivers dug in it. The sea is below
    //! zero. The same at any resolution.
    [[nodiscard]] float heightAt(float p_x, float p_z) const;

    //! \brief The cell, the region and the link a point is in.
    [[nodiscard]] Place placeOf(float p_x, float p_z) const;

    //! \brief The corners of a Voronoi cell in metres (x, z), clockwise seen
    //! from above (+Y).
    [[nodiscard]] std::vector<compages::Vector2f> cellPolygon(int p_cell) const;

    //! \brief The nearest river: the distance to its middle line, and its
    //! half width there. Far away: a large distance.
    void nearestRiver(float p_x, float p_z, float& p_distance, float& p_half) const;
};

// ----------------------------------------------------------------------------
//! \brief Make a map.
//! \param[in] p_recipe The islands, the regions, the seed.
//! \param[in] p_progress Told how far it has gone.
//! \return The map; the same recipe gives the same map.
// ----------------------------------------------------------------------------
[[nodiscard]] Land makeLand(Recipe const& p_recipe, Progress const& p_progress);

// ----------------------------------------------------------------------------
//! \brief The heights and the picture of a square of the map.
//! \param[in] p_x, p_z Its corner (smallest x and z), in metres.
//! \param[in] p_size Its side, in metres.
//! \param[in] p_cells Quads along a side.
//! \param[in] p_pixels Pixels along a side.
//! \param[in] p_progress Told how far it has gone (may be empty).
// ----------------------------------------------------------------------------
[[nodiscard]] Patch makePatch(Land const& p_land,
                              float p_x,
                              float p_z,
                              float p_size,
                              int p_cells,
                              int p_pixels,
                              Progress const& p_progress);

} // namespace examples::archipelago
