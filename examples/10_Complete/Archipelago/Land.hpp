// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The land of 10e_Archipelago, made on the CPU from a graph of
//! contents and a seed, in metres. Nothing here touches the GPU, so it can
//! run in a thread.
//!
//! The method is the one Chréage asked for (I. Aberkane, "Mind Ergonomy for
//! the Knowledge Economy", 2016, chapter 11), after A. Patel's "Polygonal
//! Map Generation": the map is a Voronoi diagram of a few thousand cells;
//! each cell is land or sea; the relief of the cells grows by uplift and
//! river erosion on the graph of the cells (Cordonnier et al. 2016).
//!
//! The land is built around the graph of the contents (Placement.hpp): each
//! island gets a disc as large as its number of links, where the layout of
//! the graph put it; the land is the union of round shapes on its links and
//! along the edges of the graph; each land cell belongs to the region of the
//! nearest link. Related links are so on the same island, side by side.
//!
//! The thesis maps the contents "as loci along a strand": a river is a
//! strand of places, each place a strand of smaller places, down to the
//! last ones, which hold the contents. So here:
//! - an island (a theme) is structured by its main river, the largest basin
//!   the erosion made;
//! - a region (a topic) is a basin: the uplift is stronger on the borders
//!   between regions, so they become ridges; its tributary runs down from
//!   its highest cell to the main river;
//! - a link is a cell along the tributary, in the order of the layout.
//! Only these rivers are drawn; they dig their bed and their valley in the
//! smooth ground between the cells.
// ****************************************************************************

#include "10_Complete/Archipelago/Map.hpp"
#include "10_Complete/Archipelago/Rivers.hpp"

#include "Compages/Core/Vector.hpp"
#include "Compages/Fields/Field.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace examples::archipelago
{

//! \brief What a map is made from: the graph of the contents, as a force
//! directed layout placed it (IslandedBrowser, see Knowledge.hpp). Its unit
//! does not matter: the islands are fitted to the map.
struct Recipe
{
    std::uint32_t seed;
    //! \brief The node of each island in the layout.
    std::vector<compages::Vector2f> islands;
    //! \brief A region: its island, its node, the nodes of its links.
    struct Group
    {
        std::size_t island;
        compages::Vector2f node;
        std::vector<compages::Vector2f> links;
    };
    std::vector<Group> regions;
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

//! \brief An island, as the graph placed it: the disc it was given, which
//! its coast follows loosely.
struct Island
{
    compages::Vector2f centre;
    float radius;
    //! \brief Its highest point.
    compages::Vector3f peak;
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
    //! \brief Quads along a side. The heights are on (cells + 3)^2 corners:
    //! one more ring around it, for the normals of its edges.
    int cells;
    compages::Field<float> heights;
    //! \brief Pixels along a side, and the picture: sRGB, first row at the
    //! smallest z, alpha as SEA_ALPHA and RIVER_ALPHA say.
    int pixels;
    std::vector<std::byte> picture;

    //! \brief The height of corner (i, j) of the patch, i and j from -1 to
    //! cells + 1.
    [[nodiscard]] float height(int p_i, int p_j) const
    {
        return heights(p_i + 1, p_j + 1);
    }

    //! \brief The height between the corners (bilinear), as its mesh shows
    //! it; clamped to the patch and its ring.
    [[nodiscard]] float heightAt(float p_x, float p_z) const
    {
        return heights.sample(compages::Vector2f(p_x, p_z));
    }
};

// ****************************************************************************
//! \brief A whole map: the cells, the regions, the rivers.
// ****************************************************************************
struct Land
{
    std::uint32_t seed = 0u;
    //! \brief Per cell (CELLS x CELLS, row after row along +Z): its site, its
    //! height, its island (-1: none), its region and its link (-1: none).
    compages::Field<compages::Vector2f> sites;
    compages::Field<float> elevations;
    compages::Field<std::int8_t> islands_of;
    compages::Field<int> regions_of;
    compages::Field<int> parts_of;
    std::vector<Island> islands;
    std::vector<Region> regions;
    //! \brief The main river of each island and the tributary of each
    //! region: the only rivers of the map.
    std::vector<River> rivers;

    //! \brief Their segments, to find the nearest river of a point.
    RiverIndex river_index;

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
    void
    nearestRiver(float p_x, float p_z, float& p_distance, float& p_half) const
    {
        river_index.nearest(p_x, p_z, p_distance, p_half);
    }
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
