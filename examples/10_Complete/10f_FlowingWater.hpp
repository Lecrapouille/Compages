// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"
#include "10_Complete/FlowingWater/FlowBake.hpp"
#include "10_Complete/FlowingWater/Spout.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

#include <array>
#include <string>
#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Water that flows round what stands in it, by a flow map: the
//! technique of Portal 2 (A. Grimes, "Making and Using Non-Standard
//! Textures", GDC 2011; A. Vlachos, "Water Flow in Portal 2", SIGGRAPH 2010;
//! https://cdn.fastly.steamstatic.com/apps/valve/2011/gdc_2011_grimes_nonstandard_textures.pdf,
//! https://www.valvesoftware.com/fr/publications),
//! in a concrete settling pool lit by sodium lamps.
//!
//! The flow map is baked on the CPU from the scene (FlowingWater/FlowBake):
//! where the geometry crosses the water, the distance to it, a general
//! current that slides along the walls, upwellings and vortices, the foam.
//! It is a Field<Vector4f> sent as it is to the GPU (Texture::fromField);
//! the water shader (FlowingWater/Shaders.hpp) moves its pictures with it.
//! The same Field moves the floating crates on the CPU (Field::sample).
//!
//! \code
//! m_bake.footprint(m_solids, m_scene.assets(), WATER_LEVEL);
//! m_bake.bake(m_recipe, m_clicked);
//! COMPAGES_TRY(flow_texture.write(std::as_bytes(m_bake.packed().values())));
//! \endcode
// ****************************************************************************
class FlowingWater final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "10f_FlowingWater";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;
    [[nodiscard]] std::string hud() const override;

private:

    //! \brief Something floating, carried by the flow.
    struct Floater
    {
        compages::world::Entity body;
        compages::Vector2f at{ 0.0f, 0.0f };
        float yaw = 0.0f;
        float stuck = 0.0f;
        //! \brief Half its width: how close to a wall it may go.
        float radius = 0.4f;
        //! \brief The height of its centre on calm water.
        float height = 0.0f;
    };

    [[nodiscard]] compages::Status makeMaterials();
    [[nodiscard]] compages::Status makeWater();
    //! \brief The pipe that spits water, its jet and its spray.
    [[nodiscard]] compages::Status makeJet();
    //! \brief The features added to the random ones: the clicks, and where
    //! the jet falls (the water it brings spreads out).
    [[nodiscard]] std::vector<flowing::Feature> extras() const;
    void buildPool();
    void buildFloaters();
    //! \brief A box or a cylinder of a material, with a tint.
    compages::world::Entity solid(compages::world::Entity p_entity, compages::renderer::MaterialId p_material,
                                  compages::Vector3f p_tint, bool p_touches_water);
    //! \brief Bake the flow map again and send it to the GPU.
    void rebake();
    //! \brief Carry the floaters with the flow for \c p_dt seconds.
    void drift(float p_dt);
    //! \brief A click on the water adds an upwelling or a vortex there.
    void click(compages::world::ViewFrame const& p_frame);
    //! \brief Circles round the features, when asked.
    void drawFeatures();
    void setUniforms();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    flowing::FlowBake m_bake;
    flowing::FlowRecipe m_recipe;
    std::vector<flowing::Feature> m_clicked;

    compages::renderer::TextureAssetId m_noise;
    compages::renderer::TextureAssetId m_ripples;
    compages::renderer::TextureAssetId m_foam;
    compages::renderer::TextureAssetId m_flow_map;
    compages::renderer::MaterialId m_concrete;
    compages::renderer::MaterialId m_metal;
    compages::renderer::MaterialId m_hazard;
    compages::renderer::MaterialId m_water_material;
    compages::renderer::MaterialId m_jet_material;
    compages::renderer::MaterialId m_crown_material;
    //! \brief Out of a pipe over the north edge, 1.85 m above the water,
    //! towards the middle of the pool.
    flowing::Jet m_jet{ .mouth = { -3.5f, 1.85f, 6.3f }, .velocity = { 0.0f, 0.2f, -2.0f }, .radius = 0.22f };
    compages::world::Entity m_jet_body;
    compages::world::Entity m_crown;
    bool m_jet_running = true;

    compages::world::Entity m_water;
    compages::world::Entity m_arrows;
    compages::renderer::ArrowLook m_arrow_look{ .colors = compages::renderer::ColorMap::Viridis,
                                                .low = 0.0f,
                                                .high = 0.7f,
                                                .scale = 0.9f,
                                                .every = 4 };
    std::vector<compages::world::Entity> m_solids;
    std::array<Floater, 3> m_floaters{};

    int m_view = 0;
    float m_time = 0.0f;
    //! \brief Seconds of one phase: longer, the pictures stretch further.
    float m_cycle = 2.0f;
    float m_flow_scale = 1.0f;
    float m_floater_pull = 0.5f;
    bool m_paused = false;
    bool m_show_arrows = false;
    bool m_show_features = false;
    bool m_click_vortex = false;
};

} // namespace examples
