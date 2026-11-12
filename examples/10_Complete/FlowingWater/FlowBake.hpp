// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Fields/Field.hpp"
#include "Compages/World/Entity.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace compages::renderer
{
class AssetManager;
} // namespace compages::renderer

// ****************************************************************************
//! \file
//! \brief The flow map of 10f_FlowingWater, made on the CPU from the scene as
//! Valve made the flow maps of Portal 2 in Houdini (A. Grimes, "Making and
//! Using Non-Standard Textures", GDC 2011): no fluid simulation, but features
//! made one by one and added up, each one under control.
// ****************************************************************************

namespace examples::flowing
{

// ****************************************************************************
//! \brief One feature of the water: an upwelling (water rising and spreading
//! out) or a vortex (water turning round).
// ****************************************************************************
struct Feature
{
    compages::Vector2f centre{ 0.0f, 0.0f };
    float radius = 1.0f;
    //! \brief The speed it adds, in metres per second, at its strongest.
    float strength = 0.5f;
    bool vortex = false;
    //! \brief +1 turns anticlockwise seen from above, -1 clockwise.
    float spin = 1.0f;
};

// ****************************************************************************
//! \brief What the artist chooses: everything else follows from the scene.
// ****************************************************************************
struct FlowRecipe
{
    //! \brief The general direction of the current, in degrees (0 = +X).
    float direction = 0.0f;
    //! \brief Its speed, in metres per second.
    float speed = 0.35f;
    //! \brief From 0 (straight across) to 1 (round the pool): the slow motion
    //! that sweeps the whole pool, the part an artist paints in the paper.
    float circulation = 0.45f;
    int upwellings = 6;
    float upwelling_radius = 2.6f;
    float upwelling_strength = 0.45f;
    int vortices = 12;
    float vortex_radius = 0.9f;
    float vortex_strength = 0.5f;
    //! \brief How much the upwellings and vortices count, where the
    //! turbulence mask lets them.
    float turbulence = 1.0f;
    //! \brief How far from a wall the water slows down, in metres.
    float edge_width = 1.2f;
    //! \brief How much the walls throw the current back.
    float deflection = 0.6f;
    std::uint32_t seed = 7u;
};

// ****************************************************************************
//! \brief The fields of the flow map, all on the same Grid2D over the water.
//!
//! \code
//! FlowBake bake(grid);
//! bake.footprint(world_entities, assets, 0.0f);   // once: what the water touches
//! bake.bake(recipe, clicked_features);            // each change of the recipe
//! texture.write(std::as_bytes(bake.packed().values()));
//! \endcode
// ****************************************************************************
class FlowBake
{
public:

    FlowBake() = default;
    explicit FlowBake(compages::Grid2D const& p_grid);

    // ------------------------------------------------------------------------
    //! \brief Find where the scene meets the water. The triangles of each
    //! entity are clipped to a thin slab around the level of the water, points
    //! are scattered over what is left and dropped onto the grid (the paper:
    //! "clip the geometry around the water surface, scatter hundreds of
    //! points, project them"). Then the distance of every point of the water
    //! to the nearest of them, and the direction away from it.
    //! \param[in] p_solids The entities that may touch the water, their world
    //! matrices up to date (World::update()).
    //! \param[in] p_assets Where their meshes are.
    //! \param[in] p_level The height of the water.
    // ------------------------------------------------------------------------
    void footprint(std::span<const compages::world::Entity> p_solids,
                   compages::renderer::AssetManager const& p_assets,
                   float p_level);

    // ------------------------------------------------------------------------
    //! \brief Make the flow, the foam and the masks from the recipe and the
    //! features clicked by the user (added to the random ones).
    // ------------------------------------------------------------------------
    void bake(FlowRecipe const& p_recipe, std::span<const Feature> p_extra);

    //! \brief The speed of the water, in metres per second.
    [[nodiscard]] compages::Field<compages::Vector2f> const& flow() const
    {
        return m_flow;
    }

    //! \brief How much foam has gathered, from 0 to 1.
    [[nodiscard]] compages::Field<float> const& foam() const
    {
        return m_foam;
    }

    //! \brief Where upwellings and vortices may be, from 0 to 1.
    [[nodiscard]] compages::Field<float> const& turbulence() const
    {
        return m_turbulence;
    }

    //! \brief 0 against a wall, 1 in open water.
    [[nodiscard]] compages::Field<float> const& open() const
    {
        return m_open;
    }

    //! \brief Distance to the nearest wall, in metres.
    [[nodiscard]] compages::Field<float> const& distance() const
    {
        return m_distance;
    }

    //! \brief The direction away from the nearest wall.
    [[nodiscard]] compages::Field<compages::Vector2f> const& away() const
    {
        return m_away;
    }

    //! \brief The flow (RG), the foam (B), the open water (A): the texture of
    //! the paper (it kept R, G for the flow and A for the foam).
    [[nodiscard]] compages::Field<compages::Vector4f> const& packed() const
    {
        return m_packed;
    }

    //! \brief The features of the last bake.
    [[nodiscard]] std::vector<Feature> const& features() const
    {
        return m_features;
    }

private:

    //! \brief Drop the points of a triangle that are in the slab.
    void scatter(compages::Vector3f const& p_a, compages::Vector3f const& p_b, compages::Vector3f const& p_c,
                 float p_level);
    //! \brief Distance to the nearest scattered point (two passes).
    void distanceTransform();
    //! \brief The current without features: one direction, a circulation,
    //! turned along the walls and thrown back from them.
    [[nodiscard]] compages::Vector2f current(std::size_t p_index, FlowRecipe const& p_recipe) const;
    //! \brief Random features, away from the walls.
    void placeFeatures(FlowRecipe const& p_recipe);

    compages::Grid2D m_grid;
    std::vector<bool> m_solid;
    std::vector<compages::Vector2f> m_nearest;
    compages::Field<float> m_distance;
    compages::Field<compages::Vector2f> m_away;
    compages::Field<compages::Vector2f> m_flow;
    compages::Field<float> m_foam;
    compages::Field<float> m_turbulence;
    compages::Field<float> m_open;
    compages::Field<compages::Vector4f> m_packed;
    std::vector<Feature> m_features;
};

} // namespace examples::flowing
