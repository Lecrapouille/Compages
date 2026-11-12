// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10f_FlowingWater.hpp"
#include "10_Complete/FlowingWater/Pictures.hpp"
#include "10_Complete/FlowingWater/Shaders.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <span>

namespace examples
{

using compages::Vector2f;
using compages::Vector3f;
using compages::Vector4f;

namespace
{

//! \brief The pool: 24 x 14 m of water at height 0, walls 0.6 m thick, their
//! top 1 m above the water.
constexpr float HALF_X = 12.0f;
constexpr float HALF_Z = 7.0f;
constexpr float LEVEL = 0.0f;
//! \brief One point of the flow map every 12.5 cm: 193 x 113.
constexpr float SPACING = 0.125f;
constexpr float PI = std::numbers::pi_v<float>;
//! \brief The colour of sodium lamps, and of the overcast sky.
const Vector3f SODIUM(1.0f, 0.6f, 0.24f);
const Vector3f SKY(0.30f, 0.34f, 0.35f);

//! \brief Where the floaters come back, in front of the inlet pipe.
const Vector2f INLET(-10.4f, 2.0f);

constexpr char const* VIEWS = "Water\0"
                              "Flow map (RG = flow * 0.5 + 0.5)\0"
                              "1. One phase: the pop\0"
                              "2. Phase offset by noise: crawling edges\0"
                              "3. Two phases blended: seamless\0"
                              "Noise texture (phase offset)\0"
                              "Foam mask (B)\0"
                              "Open water (A)\0";

} // namespace

// ----------------------------------------------------------------------------
std::string FlowingWater::description() const
{
    return "Water flowing round what stands in it, by a flow map, as in "
           "Portal 2 (GDC 2011, \"Making and Using Non-Standard Textures\"). "
           "The flow map is baked on the CPU from the scene: the geometry "
           "crossing the water, the distance to it, a current that slides "
           "along the walls, upwellings, vortices, the foam, and where the jet "
           "of a drain pipe falls. It is a Field<Vector4f> sent as it is to "
           "the GPU; the shader moves the ripples and the foam with it, in two "
           "phases blended by a noise. The same Field carries the crates. Try "
           "it: the views (each step of the paper), the arrows, a click on the "
           "water, the pipe.";
}

// ----------------------------------------------------------------------------
compages::Status FlowingWater::setUp()
{
    m_scene.background(SKY.x, SKY.y, SKY.z).ambient(0.20f, 0.23f, 0.24f);
    m_scene.environment().fog_color = SKY;
    m_scene.environment().fog_density = 0.018f;
    m_scene.sun("Overcast", Vector3f(0.65f, 0.72f, 0.80f), 0.75f)
        .position(-6.0f, 20.0f, -10.0f)
        .lookAt(0.0f, 0.0f, 0.0f);
    m_scene.camera()
        .position(-4.0f, 4.2f, -11.5f)
        .add<compages::world::Orbit>(Vector3f(-2.0f, 0.0f, 0.5f));

    COMPAGES_TRY(makeMaterials());
    buildPool();
    buildFloaters();

    // What the water touches, from the scene as it stands.
    m_world.update();
    m_bake = flowing::FlowBake(
        compages::Grid2D{ .columns = int(2.0f * HALF_X / SPACING) + 1,
                          .rows = int(2.0f * HALF_Z / SPACING) + 1,
                          .origin = { -HALF_X, -HALF_Z },
                          .spacing = SPACING });
    m_bake.footprint(m_solids, m_scene.assets(), LEVEL);
    m_bake.bake(m_recipe, extras());
    COMPAGES_TRY(makeWater());
    COMPAGES_TRY(makeJet());

    m_arrows = m_scene.show(m_bake.flow(), m_arrow_look, "Flow arrows");
    m_arrows.position(0.0f, LEVEL + 0.04f, 0.0f).enable(false);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! The concrete, the painted metal and the stripes: one shader (triplanar),
//! three materials, each with its picture.
// ----------------------------------------------------------------------------
compages::Status FlowingWater::makeMaterials()
{
    COMPAGES_TRY_ASSIGN(
        m_noise,
        flowing::addPicture(m_scene, "Noise", flowing::noisePicture(), false));
    COMPAGES_TRY_ASSIGN(
        m_ripples,
        flowing::addPicture(
            m_scene, "Ripples", flowing::ripplePicture(), false));
    COMPAGES_TRY_ASSIGN(
        m_foam,
        flowing::addPicture(m_scene, "Foam", flowing::foamPicture(), true));
    compages::renderer::TextureAssetId concrete;
    compages::renderer::TextureAssetId metal;
    compages::renderer::TextureAssetId hazard;
    COMPAGES_TRY_ASSIGN(
        concrete,
        flowing::addPicture(
            m_scene, "Concrete", flowing::concretePicture(), true));
    COMPAGES_TRY_ASSIGN(
        metal,
        flowing::addPicture(
            m_scene, "Painted metal", flowing::paintedMetalPicture(), true));
    COMPAGES_TRY_ASSIGN(
        hazard,
        flowing::addPicture(m_scene, "Hazard", flowing::hazardPicture(), true));

    auto make = [&](std::string const& p_name,
                    compages::renderer::TextureAssetId p_picture)
        -> compages::Result<compages::renderer::MaterialId>
    {
        compages::renderer::shaders::ShaderPair shaders =
            compages::renderer::shaders::pbr();
        shaders.fragment().hook("main", flowing::SOLID_MAIN);
        compages::renderer::Material material;
        COMPAGES_TRY_ASSIGN(
            material,
            compages::renderer::Material::create(
                p_name,
                { .family = compages::renderer::ShaderFamily::PbrMinimal,
                  .shaders = std::move(shaders),
                  .state = { .depth_test = true,
                             .cull = compages::gpu::CullMode::Back },
                  .see_through = std::nullopt }));
        material.textures = { { .sampler = "surfaceMap",
                                .texture = p_picture } };
        return m_scene.assets().addMaterial(material.name, std::move(material));
    };
    COMPAGES_TRY_ASSIGN(m_concrete, make("Concrete", concrete));
    COMPAGES_TRY_ASSIGN(m_metal, make("Painted metal", metal));
    COMPAGES_TRY_ASSIGN(m_hazard, make("Hazard stripes", hazard));
    return compages::success();
}

// ----------------------------------------------------------------------------
//! The flow map goes to the GPU as the Field holds it (RGBA32F, linear,
//! clamped), and the water reads it with three pictures that repeat.
// ----------------------------------------------------------------------------
compages::Status FlowingWater::makeWater()
{
    compages::renderer::TextureAsset flow;
    flow.name = "Flow map";
    COMPAGES_TRY_ASSIGN(flow.texture,
                        compages::gpu::Texture::fromField(m_bake.packed()));
    COMPAGES_TRY_ASSIGN(
        m_flow_map, m_scene.assets().addTexture(flow.name, std::move(flow)));

    compages::renderer::shaders::ShaderPair shaders =
        compages::renderer::shaders::pbr();
    shaders.fragment().hook("main", flowing::WATER_MAIN);
    compages::renderer::Material water;
    COMPAGES_TRY_ASSIGN(
        water,
        compages::renderer::Material::create(
            "Flowing water",
            { .family = compages::renderer::ShaderFamily::PbrMinimal,
              .shaders = std::move(shaders),
              .state = { .depth_test = true,
                         .cull = compages::gpu::CullMode::Back },
              .see_through = std::nullopt }));
    water.textures = { { .sampler = "flowMap", .texture = m_flow_map },
                       { .sampler = "noiseMap", .texture = m_noise },
                       { .sampler = "rippleMap", .texture = m_ripples },
                       { .sampler = "foamMap", .texture = m_foam } };
    COMPAGES_TRY_ASSIGN(
        m_water_material,
        m_scene.assets().addMaterial(water.name, std::move(water)));

    // A square standing in XY, laid on the water.
    m_water = m_scene.plane("Water");
    m_water.rotation(-0.5f * PI, Vector3f(1.0f, 0.0f, 0.0f))
        .scale(2.0f * HALF_X, 2.0f * HALF_Z, 1.0f)
        .position(0.0f, LEVEL, 0.0f);
    m_scene.look(m_water, m_water_material);
    m_water.get<compages::renderer::MeshRenderer>().flags =
        compages::renderer::RenderFlags::NoShadowCaster;
    return compages::success();
}

// ----------------------------------------------------------------------------
//! The jet and its crown are see-through (opacity under one): drawn after
//! the solid things, blended, without writing the depth. Both faces drawn,
//! so the far side of the tube shows through the near one.
// ----------------------------------------------------------------------------
compages::Status FlowingWater::makeJet()
{
    auto make = [&](std::string const& p_name, float p_scroll, float p_fade)
        -> compages::Result<compages::renderer::MaterialId>
    {
        compages::renderer::shaders::ShaderPair shaders =
            compages::renderer::shaders::pbr();
        shaders.fragment().hook("main", flowing::JET_MAIN);
        compages::renderer::Material material;
        COMPAGES_TRY_ASSIGN(
            material,
            compages::renderer::Material::create(
                p_name,
                { .family = compages::renderer::ShaderFamily::PbrMinimal,
                  .shaders = std::move(shaders),
                  .state = { .depth_test = true,
                             .cull = compages::gpu::CullMode::None },
                  .see_through = compages::gpu::RenderState{
                      .depth_test = true,
                      .depth_write = false,
                      .blend = compages::gpu::Blend::alpha(),
                      .cull = compages::gpu::CullMode::None } }));
        material.textures = { { .sampler = "noiseMap", .texture = m_noise },
                              { .sampler = "foamMap", .texture = m_foam } };
        material.program.set("scroll", p_scroll);
        material.program.set("fade", p_fade);
        return m_scene.assets().addMaterial(material.name, std::move(material));
    };
    COMPAGES_TRY_ASSIGN(m_jet_material, make("Water jet", 2.6f, 0.0f));
    COMPAGES_TRY_ASSIGN(m_crown_material, make("Spray", -0.8f, 1.0f));

    auto seeThrough = [&](compages::world::Entity p_entity,
                          compages::renderer::MaterialId p_material)
    {
        m_scene.look(p_entity, p_material);
        compages::renderer::MeshRenderer& renderer =
            p_entity.get<compages::renderer::MeshRenderer>();
        renderer.flags = compages::renderer::RenderFlags::NoShadowCaster;
        if (compages::renderer::MaterialInstance* instance =
                m_scene.assets().materialInstance(renderer.material_instance))
        {
            instance->opacity = 0.99f;
        }
    };
    m_jet_body = m_scene.mesh(flowing::jetMesh(m_jet, LEVEL), "Water jet");
    seeThrough(m_jet_body, m_jet_material);
    const Vector3f fall = m_jet.at(m_jet.fall(LEVEL));
    m_crown = m_scene.mesh(flowing::crownMesh(0.32f, 0.32f), "Spray");
    m_crown.position(fall.x, LEVEL - 0.03f, fall.z);
    seeThrough(m_crown, m_crown_material);
    return compages::success();
}

// ----------------------------------------------------------------------------
compages::world::Entity
FlowingWater::solid(compages::world::Entity p_entity,
                    compages::renderer::MaterialId p_material,
                    Vector3f p_tint,
                    bool p_touches_water)
{
    m_scene.look(p_entity, p_material);
    if (compages::renderer::MaterialInstance* instance =
            m_scene.assets().materialInstance(
                p_entity.get<compages::renderer::MeshRenderer>()
                    .material_instance))
    {
        instance->base_color_factor = compages::renderer::srgbToLinear(p_tint);
    }
    if (p_touches_water)
    {
        m_solids.push_back(p_entity);
    }
    return p_entity;
}

// ----------------------------------------------------------------------------
//! A settling pool of a treatment plant: cast concrete, green painted
//! metal, sodium lamps. What stands in the water shapes the flow map: the
//! walls, the pillars, a round column, the inlet pipe, a pipe lying in the
//! water, the legs of the catwalk, a sluice gate. The catwalk itself and
//! the beams are above the water: the slab of the footprint leaves them out.
// ----------------------------------------------------------------------------
void FlowingWater::buildPool()
{
    const Vector3f grey(1.0f, 1.0f, 1.0f);
    const Vector3f dark(0.75f, 0.75f, 0.73f);
    auto box = [&](char const* p_name,
                   Vector3f p_at,
                   Vector3f p_size,
                   compages::renderer::MaterialId p_material,
                   Vector3f p_tint,
                   bool p_touches)
    {
        return solid(m_scene.box(p_name).position(p_at).scale(p_size),
                     p_material,
                     p_tint,
                     p_touches);
    };
    auto pipe = [&](char const* p_name,
                    Vector3f p_at,
                    float p_across,
                    float p_length,
                    compages::renderer::MaterialId p_material,
                    Vector3f p_tint,
                    bool p_touches)
    {
        // A cylinder stands along Y: turned a quarter round Z, it lies along X.
        return solid(m_scene.cylinder(p_name)
                         .position(p_at)
                         .rotation(0.5f * PI, Vector3f(0.0f, 0.0f, 1.0f))
                         .scale(p_across, p_length, p_across),
                     p_material,
                     p_tint,
                     p_touches);
    };

    // The walls of the pool, from 2 m under the water to 1 m above it.
    box("North wall",
        { 0.0f, -0.5f, HALF_Z + 0.3f },
        { 2.0f * HALF_X + 1.2f, 3.0f, 0.6f },
        m_concrete,
        grey,
        true);
    box("South wall",
        { 0.0f, -0.5f, -HALF_Z - 0.3f },
        { 2.0f * HALF_X + 1.2f, 3.0f, 0.6f },
        m_concrete,
        grey,
        true);
    box("West wall",
        { -HALF_X - 0.3f, -0.5f, 0.0f },
        { 0.6f, 3.0f, 2.0f * HALF_Z },
        m_concrete,
        grey,
        true);
    box("East wall",
        { HALF_X + 0.3f, -0.5f, 0.0f },
        { 0.6f, 3.0f, 2.0f * HALF_Z },
        m_concrete,
        grey,
        true);
    // The floor round the pool, and stripes along its edge.
    box("North deck",
        { 0.0f, 0.9f, 11.1f },
        { 40.0f, 0.2f, 7.0f },
        m_concrete,
        dark,
        false);
    box("South deck",
        { 0.0f, 0.9f, -11.1f },
        { 40.0f, 0.2f, 7.0f },
        m_concrete,
        dark,
        false);
    box("West deck",
        { -16.3f, 0.9f, 0.0f },
        { 7.4f, 0.2f, 15.2f },
        m_concrete,
        dark,
        false);
    box("East deck",
        { 16.3f, 0.9f, 0.0f },
        { 7.4f, 0.2f, 15.2f },
        m_concrete,
        dark,
        false);
    box("North stripes",
        { 0.0f, 1.04f, HALF_Z + 0.15f },
        { 2.0f * HALF_X + 0.6f, 0.08f, 0.3f },
        m_hazard,
        grey,
        false);
    box("South stripes",
        { 0.0f, 1.04f, -HALF_Z - 0.15f },
        { 2.0f * HALF_X + 0.6f, 0.08f, 0.3f },
        m_hazard,
        grey,
        false);
    box("West stripes",
        { -HALF_X - 0.15f, 1.04f, 0.0f },
        { 0.3f, 0.08f, 2.0f * HALF_Z },
        m_hazard,
        grey,
        false);
    box("East stripes",
        { HALF_X + 0.15f, 1.04f, 0.0f },
        { 0.3f, 0.08f, 2.0f * HALF_Z },
        m_hazard,
        grey,
        false);
    // The building behind: a high wall with ribs, another on the west side.
    box("Back wall",
        { 0.0f, 4.5f, 14.9f },
        { 40.0f, 9.0f, 0.6f },
        m_concrete,
        dark,
        false);
    box("Side wall",
        { -20.3f, 4.5f, 3.5f },
        { 0.6f, 9.0f, 22.8f },
        m_concrete,
        dark,
        false);
    for (float x = -18.0f; x <= 18.0f; x += 6.0f)
    {
        box("Rib",
            { x, 4.5f, 14.3f },
            { 0.8f, 9.0f, 0.6f },
            m_concrete,
            grey,
            false);
    }
    pipe("Wall pipe", { 0.0f, 6.8f, 14.2f }, 0.5f, 40.0f, m_metal, grey, false);
    pipe("Wall pipe",
         { 0.0f, 7.5f, 14.2f },
         0.3f,
         40.0f,
         m_metal,
         Vector3f(0.7f, 0.7f, 0.7f),
         false);

    // In the water: pillars carrying beams to the back wall, a round column.
    for (Vector2f at : { Vector2f(-5.0f, -2.6f),
                         Vector2f(2.6f, 2.2f),
                         Vector2f(7.6f, -3.0f) })
    {
        box("Pillar",
            { at.x, 1.5f, at.y },
            { 1.0f, 7.0f, 1.0f },
            m_concrete,
            grey,
            true);
        const float length = 14.6f - at.y;
        box("Beam",
            { at.x, 5.25f, at.y + (0.5f * length) - 0.5f },
            { 0.8f, 0.5f, length },
            m_concrete,
            grey,
            false);
    }
    solid(m_scene.cylinder("Column")
              .position(-1.2f, 1.5f, 4.4f)
              .scale(1.4f, 7.0f, 1.4f),
          m_concrete,
          grey,
          true);

    // The inlet: a big pipe out of the west wall, at the level of the water.
    pipe("Inlet",
         { INLET.x - 1.4f, 0.15f, INLET.y },
         1.6f,
         2.4f,
         m_metal,
         grey,
         true);
    pipe("Inlet flange",
         { INLET.x - 0.3f, 0.15f, INLET.y },
         1.95f,
         0.18f,
         m_metal,
         dark,
         true);
    // A pipe lying in the water, and a sluice gate in the east wall.
    pipe("Aerator",
         { 6.0f, -0.05f, 4.7f },
         0.45f,
         6.5f,
         m_metal,
         Vector3f(0.8f, 0.6f, 0.5f),
         true);
    box("Sluice gate",
        { HALF_X - 0.1f, 0.4f, -1.5f },
        { 0.3f, 1.8f, 2.6f },
        m_metal,
        grey,
        true);
    box("Gate frame",
        { HALF_X - 0.1f, 1.6f, -1.5f },
        { 0.4f, 0.3f, 3.2f },
        m_metal,
        dark,
        false);
    // A drain pipe comes up through the north deck and over the edge of the
    // pool: the jet comes out of it (makeJet). It does not reach the slab of
    // the footprint.
    const float pipe_y = m_jet.mouth.y + 0.04f;
    const Vector3f rusty(0.85f, 0.6f, 0.45f);
    auto alongZ = [&](char const* p_name,
                      float p_z,
                      float p_across,
                      float p_length,
                      Vector3f p_tint)
    {
        solid(m_scene.cylinder(p_name)
                  .position(m_jet.mouth.x, pipe_y, p_z)
                  .rotation(0.5f * PI, Vector3f(1.0f, 0.0f, 0.0f))
                  .scale(p_across, p_length, p_across),
              m_metal,
              p_tint,
              false);
    };
    alongZ("Drain pipe", 8.05f, 0.7f, 3.5f, rusty);
    alongZ("Drain flange", 6.36f, 0.9f, 0.1f, dark);
    alongZ("Drain flange", 8.0f, 0.86f, 0.08f, dark);
    solid(m_scene.cylinder("Drain riser")
              .position(m_jet.mouth.x, 0.5f * (1.0f + pipe_y), 9.8f)
              .scale(0.7f, pipe_y - 1.0f, 0.7f),
          m_metal,
          rusty,
          false);
    solid(m_scene.sphere("Drain elbow")
              .position(m_jet.mouth.x, pipe_y, 9.8f)
              .scale(0.7f),
          m_metal,
          rusty,
          false);
    box("Pipe support",
        { m_jet.mouth.x, 0.5f * (1.0f + pipe_y - 0.35f), 7.4f },
        { 0.12f, pipe_y - 0.35f - 1.0f, 0.12f },
        m_metal,
        dark,
        false);
    m_scene
        .cylinder("Drain hole", compages::renderer::unlit(0.01f, 0.01f, 0.01f))
        .position(m_jet.mouth.x, pipe_y, 6.3f)
        .rotation(0.5f * PI, Vector3f(1.0f, 0.0f, 0.0f))
        .scale(0.56f, 0.02f, 0.56f);

    // A catwalk across the pool: its legs only touch the water.
    constexpr float CATWALK = -8.0f;
    box("Catwalk",
        { CATWALK, 2.6f, 0.0f },
        { 1.4f, 0.12f, 16.0f },
        m_metal,
        grey,
        false);
    for (float side : { -0.68f, 0.68f })
    {
        box("Rail",
            { CATWALK + side, 3.6f, 0.0f },
            { 0.06f, 0.06f, 16.0f },
            m_metal,
            grey,
            false);
        for (float z = -7.5f; z <= 7.6f; z += 2.5f)
        {
            box("Post",
                { CATWALK + side, 3.1f, z },
                { 0.06f, 1.0f, 0.06f },
                m_metal,
                grey,
                false);
        }
    }
    for (float z : { -3.5f, 3.5f })
    {
        box("Catwalk leg",
            { CATWALK, 0.3f, z },
            { 0.25f, 4.6f, 0.25f },
            m_metal,
            grey,
            true);
    }

    // Light: three sodium lamps, a cold tube under the catwalk.
    auto lamp = [&](Vector3f p_at,
                    Vector3f p_colour,
                    float p_intensity,
                    float p_range,
                    Vector3f p_size)
    {
        m_scene.lamp("Lamp", p_colour, p_intensity, p_range).position(p_at);
        m_scene
            .box("Lamp glass",
                 compages::renderer::unlit(
                     p_colour.x, p_colour.y, 0.6f * p_colour.z))
            .position(p_at)
            .scale(p_size);
    };
    lamp({ -4.0f, 4.3f, 14.4f }, SODIUM, 3.0f, 17.0f, { 0.7f, 0.25f, 0.4f });
    lamp({ 7.0f, 4.3f, 14.4f }, SODIUM, 3.0f, 17.0f, { 0.7f, 0.25f, 0.4f });
    lamp({ 3.0f, 4.2f, -8.6f }, SODIUM, 2.6f, 15.0f, { 0.6f, 0.2f, 0.6f });
    box("Lamp post",
        { 3.0f, 2.6f, -9.0f },
        { 0.15f, 3.4f, 0.15f },
        m_metal,
        grey,
        false);
    lamp({ CATWALK, 2.45f, -2.0f },
         Vector3f(0.70f, 1.0f, 0.85f),
         1.6f,
         7.0f,
         { 0.12f, 0.06f, 1.4f });
}

// ----------------------------------------------------------------------------
void FlowingWater::buildFloaters()
{
    const std::filesystem::path wood = dataPath("wooden-crate.jpg");
    auto crate = [&]()
    {
        return wood.empty()
                   ? m_scene.box("Crate",
                                 compages::renderer::color(0.55f, 0.4f, 0.25f))
                   : m_scene.box("Crate", compages::renderer::texture(wood));
    };
    m_floaters[0] = { .body = crate().scale(0.7f),
                      .at = Vector2f(-9.6f, 1.2f),
                      .radius = 0.45f,
                      .height = 0.12f };
    m_floaters[1] = { .body = crate().scale(0.55f),
                      .at = Vector2f(-3.0f, -4.6f),
                      .radius = 0.36f,
                      .height = 0.08f };
    compages::world::Entity barrel =
        m_scene.cylinder("Barrel").scale(0.6f, 0.9f, 0.6f);
    m_scene.look(barrel, m_metal);
    if (compages::renderer::MaterialInstance* instance =
            m_scene.assets().materialInstance(
                barrel.get<compages::renderer::MeshRenderer>()
                    .material_instance))
    {
        instance->base_color_factor =
            compages::renderer::srgbToLinear(Vector3f(1.0f, 0.35f, 0.25f));
    }
    m_floaters[2] = { .body = barrel,
                      .at = Vector2f(4.5f, -0.5f),
                      .radius = 0.36f,
                      .height = 0.2f };
}

// ----------------------------------------------------------------------------
std::vector<flowing::Feature> FlowingWater::extras() const
{
    std::vector<flowing::Feature> all = m_clicked;
    if (m_jet_running)
    {
        const Vector3f fall = m_jet.at(m_jet.fall(LEVEL));
        all.push_back({ .centre = Vector2f(fall.x, fall.z),
                        .radius = 2.2f,
                        .strength = 0.55f,
                        .vortex = false,
                        .spin = 1.0f });
    }
    return all;
}

// ----------------------------------------------------------------------------
void FlowingWater::rebake()
{
    m_bake.bake(m_recipe, extras());
    if (compages::renderer::TextureAsset* flow =
            m_scene.assets().texture(m_flow_map))
    {
        if (compages::Status written =
                flow->texture.write(std::as_bytes(m_bake.packed().values()));
            !written)
        {
            compages::gpu::reportError(written.error());
        }
    }
    if (compages::Status shown =
            m_scene.show(m_arrows, m_bake.flow(), m_arrow_look);
        !shown)
    {
        compages::gpu::reportError(shown.error());
    }
}

// ----------------------------------------------------------------------------
//! The floaters read the flow map where they are, as the GPU does (same
//! Field, bilinear); they turn with the curl of the flow (half of it is the
//! spin of a small body), and the walls push them back. Stuck or gone out,
//! they come back at the inlet.
// ----------------------------------------------------------------------------
void FlowingWater::drift(float p_dt)
{
    compages::Field<Vector2f> const& flow = m_bake.flow();
    compages::Grid2D const& grid = flow.grid();
    const float h = 2.0f * grid.spacing;
    for (std::size_t n = 0u; n < m_floaters.size(); ++n)
    {
        Floater& floater = m_floaters[n];
        const Vector2f v = flow.sample(floater.at) * m_flow_scale;
        const float curl = ((flow.sample(floater.at + Vector2f(h, 0.0f)).y -
                             flow.sample(floater.at - Vector2f(h, 0.0f)).y) -
                            (flow.sample(floater.at + Vector2f(0.0f, h)).x -
                             flow.sample(floater.at - Vector2f(0.0f, h)).x)) /
                           (2.0f * h);
        floater.at += v * p_dt;
        // Turning from +X towards +Z is turning round -Y.
        floater.yaw -= 0.5f * curl * m_flow_scale * p_dt;
        const float wall = m_bake.distance().sample(floater.at);
        if (wall < floater.radius)
        {
            floater.at +=
                m_bake.away().sample(floater.at) * (floater.radius - wall);
        }
        floater.stuck =
            (std::hypot(v.x, v.y) < 0.04f) ? (floater.stuck + p_dt) : 0.0f;
        const bool out = (std::abs(floater.at.x) > HALF_X - 0.3f) ||
                         (std::abs(floater.at.y) > HALF_Z - 0.3f);
        if (out || (floater.stuck > 6.0f))
        {
            floater.at = INLET + Vector2f(0.3f, (float(n) - 1.0f) * 0.6f);
            floater.stuck = 0.0f;
        }
        const float bob = 0.04f * std::sin((1.7f * m_time) + (2.1f * float(n)));
        floater.body
            .position(floater.at.x, LEVEL + floater.height + bob, floater.at.y)
            .rotation(floater.yaw, Vector3f(0.0f, 1.0f, 0.0f));
    }
}

// ----------------------------------------------------------------------------
//! The ray under the mouse from the last camera (two points of the pixel
//! unprojected), to the plane of the water.
// ----------------------------------------------------------------------------
void FlowingWater::click(compages::world::ViewFrame const& p_frame)
{
    compages::renderer::CameraFrame const& camera = m_scene.lastCamera();
    if (!p_frame.input.mouse_left_pressed || !p_frame.input.mouse_over ||
        (camera.viewport_width == 0u) || (camera.viewport_height == 0u))
    {
        return;
    }
    const float x = (2.0f * (p_frame.input.mouse.x - camera.viewport_x) /
                     float(camera.viewport_width)) -
                    1.0f;
    const float y = (2.0f * (p_frame.input.mouse.y - camera.viewport_y) /
                     float(camera.viewport_height)) -
                    1.0f;
    auto unproject = [&](float p_depth)
    {
        const Vector4f eye =
            camera.inverse_projection * Vector4f(x, y, p_depth, 1.0f);
        const Vector4f world =
            camera.inverse_view *
            Vector4f(eye.x / eye.w, eye.y / eye.w, eye.z / eye.w, 1.0f);
        return Vector3f(world.x, world.y, world.z);
    };
    const Vector3f a = unproject(0.2f);
    const Vector3f b = unproject(0.8f);
    if (std::abs(b.y - a.y) < 1.0e-6f)
    {
        return;
    }
    const float t = (LEVEL - a.y) / (b.y - a.y);
    const Vector3f hit = a + ((b - a) * t);
    if ((std::abs(hit.x) > HALF_X) || (std::abs(hit.z) > HALF_Z))
    {
        return;
    }
    if (m_clicked.size() >= 16u)
    {
        m_clicked.erase(m_clicked.begin());
    }
    m_clicked.push_back(
        { .centre = Vector2f(hit.x, hit.z),
          .radius = m_click_vortex ? 1.4f : 2.6f,
          .strength = m_click_vortex ? 0.8f : 0.6f,
          .vortex = m_click_vortex,
          .spin = (m_clicked.size() % 2u == 0u) ? 1.0f : -1.0f });
    rebake();
}

// ----------------------------------------------------------------------------
void FlowingWater::drawFeatures()
{
    auto circle = [&](flowing::Feature const& p_feature, Vector3f p_colour)
    {
        constexpr int SEGMENTS = 32;
        for (int k = 0; k < SEGMENTS; ++k)
        {
            const float a0 = 2.0f * PI * float(k) / float(SEGMENTS);
            const float a1 = 2.0f * PI * float(k + 1) / float(SEGMENTS);
            const Vector2f c = p_feature.centre;
            const float r = p_feature.radius;
            m_scene.debug().line(Vector3f(c.x + (r * std::cos(a0)),
                                          LEVEL + 0.06f,
                                          c.y + (r * std::sin(a0))),
                                 Vector3f(c.x + (r * std::cos(a1)),
                                          LEVEL + 0.06f,
                                          c.y + (r * std::sin(a1))),
                                 p_colour);
        }
    };
    for (flowing::Feature const& feature : m_bake.features())
    {
        circle(feature,
               feature.vortex ? Vector3f(1.0f, 0.55f, 0.1f)
                              : Vector3f(0.2f, 0.85f, 1.0f));
    }
    for (flowing::Feature const& feature : m_clicked)
    {
        circle(feature, Vector3f(1.0f, 1.0f, 1.0f));
    }
}

// ----------------------------------------------------------------------------
void FlowingWater::setUniforms()
{
    for (compages::renderer::MaterialId id : { m_concrete, m_metal, m_hazard })
    {
        if (compages::renderer::Material* material =
                m_scene.assets().material(id))
        {
            material->program.set("tileMetres", (id == m_hazard) ? 1.2f : 4.0f);
            material->program.set("waterLevel", LEVEL);
        }
    }
    compages::renderer::Material* water =
        m_scene.assets().material(m_water_material);
    if (water == nullptr)
    {
        return;
    }
    compages::Grid2D const& grid = m_bake.flow().grid();
    compages::gpu::Program& program = water->program;
    program.set("flowOrigin", grid.origin);
    program.set("flowSpacing", grid.spacing);
    program.set("flowCells", Vector2f(float(grid.columns), float(grid.rows)));
    program.set("time", m_time);
    program.set("cycle", m_cycle);
    program.set("flowScale", m_flow_scale);
    program.set("maxSpeed", std::max(2.0f * m_recipe.speed, 0.05f));
    program.set("waterView", m_view);
    program.set("floater0", m_floaters[0].at);
    program.set("floater1", m_floaters[1].at);
    program.set("floater2", m_floaters[2].at);
    program.set("floaterPull", m_floater_pull);
    const Vector3f fall = m_jet.at(m_jet.fall(LEVEL));
    program.set("splashAt", Vector2f(fall.x, fall.z));
    program.set("splashPower", m_jet_running ? 1.0f : 0.0f);
    for (compages::renderer::MaterialId id :
         { m_jet_material, m_crown_material })
    {
        if (compages::renderer::Material* material =
                m_scene.assets().material(id))
        {
            material->program.set("time", m_time);
        }
    }
}

// ----------------------------------------------------------------------------
void FlowingWater::draw(compages::world::ViewFrame const& p_frame)
{
    const float dt = m_paused ? 0.0f : std::min(p_frame.elapsed, 0.1f);
    m_time += dt;
    click(p_frame);
    drift(dt);
    m_arrows.enable(m_show_arrows);
    m_jet_body.enable(m_jet_running);
    m_crown.enable(m_jet_running);
    // The crown breathes with the jet.
    m_crown.scale(1.0f + (0.06f * std::sin(7.0f * m_time)),
                  1.0f + (0.15f * std::sin(5.3f * m_time)),
                  1.0f + (0.06f * std::cos(6.1f * m_time)));
    if (m_show_features)
    {
        drawFeatures();
    }
    setUniforms();
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
std::string FlowingWater::hud() const
{
    return m_click_vortex ? "Left click on the water: a vortex"
                          : "Left click on the water: an upwelling";
}

// ----------------------------------------------------------------------------
void FlowingWater::controls()
{
    ImGui::Combo("Show", &m_view, VIEWS);
    ImGui::Checkbox("Arrows", &m_show_arrows);
    ImGui::SameLine();
    ImGui::Checkbox("Features", &m_show_features);
    ImGui::SameLine();
    ImGui::Checkbox("Pause", &m_paused);
    ImGui::SliderFloat("Cycle (s)", &m_cycle, 0.5f, 8.0f, "%.1f");
    ImGui::SliderFloat("Flow speed x", &m_flow_scale, 0.0f, 3.0f, "%.2f");
    ImGui::SliderFloat("Floaters swirl", &m_floater_pull, 0.0f, 1.5f, "%.2f");

    ImGui::SeparatorText("Bake (CPU)");
    bool changed = ImGui::SliderFloat(
        "Direction (deg)", &m_recipe.direction, -180.0f, 180.0f, "%.0f");
    changed = ImGui::SliderFloat(
                  "Speed (m/s)", &m_recipe.speed, 0.05f, 0.8f, "%.2f") ||
              changed;
    changed = ImGui::SliderFloat(
                  "Circulation", &m_recipe.circulation, 0.0f, 1.0f, "%.2f") ||
              changed;
    changed = ImGui::SliderFloat(
                  "Deflection", &m_recipe.deflection, 0.0f, 1.5f, "%.2f") ||
              changed;
    changed = ImGui::SliderFloat(
                  "Edge width (m)", &m_recipe.edge_width, 0.2f, 3.0f, "%.1f") ||
              changed;
    changed =
        ImGui::SliderInt("Upwellings", &m_recipe.upwellings, 0, 20) || changed;
    changed = ImGui::SliderFloat("Upwelling radius",
                                 &m_recipe.upwelling_radius,
                                 0.5f,
                                 5.0f,
                                 "%.1f") ||
              changed;
    changed =
        ImGui::SliderInt("Vortices", &m_recipe.vortices, 0, 40) || changed;
    changed =
        ImGui::SliderFloat(
            "Vortex radius", &m_recipe.vortex_radius, 0.3f, 2.5f, "%.1f") ||
        changed;
    changed = ImGui::SliderFloat(
                  "Turbulence", &m_recipe.turbulence, 0.0f, 2.0f, "%.2f") ||
              changed;
    int seed = int(m_recipe.seed);
    if (ImGui::InputInt("Seed", &seed))
    {
        m_recipe.seed = std::uint32_t(std::max(seed, 0));
        changed = true;
    }
    changed = ImGui::Checkbox("Pipe running", &m_jet_running) || changed;
    ImGui::Checkbox("A click adds a vortex", &m_click_vortex);
    if (ImGui::Button("Forget the clicks"))
    {
        m_clicked.clear();
        changed = true;
    }
    if (changed)
    {
        m_arrow_look.high = 2.0f * m_recipe.speed;
        rebake();
    }
}

} // namespace examples
