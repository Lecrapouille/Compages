// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Sea.hpp"
#include "10_Complete/Archipelago/Meshes.hpp"
#include "10_Complete/Archipelago/Shaders.hpp"
#include "10_Complete/Archipelago/Swells.hpp"

#include "Common/DataPath.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Assets/Material.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_km;

//! \brief The surface of the sea: 160 x 160 quads, as wide as twice the
//! distance of the camera (1.2 to 7 km), shown within 6 km.
static constexpr int SURFACE_CELLS = 160;
static constexpr float SURFACE_WITHIN = compages::toMetres(6.0_km);

// ----------------------------------------------------------------------------
compages::Status Sea::create(compages::renderer::Scene& p_scene)
{
    // The pictures of the ground (ambientCG, CC0): sRGB, repeated, with
    // mipmaps; the map material reads them by the names of its samplers.
    const std::array<std::pair<char const*, char const*>, 3> grounds = {
        std::pair{ "grassMap", "terrain/grass.jpg" },
        std::pair{ "sandMap", "terrain/sand.jpg" },
        std::pair{ "rockMap", "terrain/rock.jpg" }
    };
    m_grounds.clear();
    for (auto const& [sampler, name] : grounds)
    {
        const std::filesystem::path file = dataPath(name);
        if (file.empty())
        {
            return compages::failure(std::string(name) +
                                     " is missing from external/Compages-data/");
        }
        compages::gpu::LoadOptions options;
        options.srgb = true;
        options.wrap = compages::gpu::Wrap::Repeat;
        compages::renderer::TextureAsset picture;
        picture.name = name;
        COMPAGES_TRY_ASSIGN(picture.texture,
                            compages::gpu::Texture::fromFile(file, options));
        compages::renderer::TextureAssetId id;
        COMPAGES_TRY_ASSIGN(
            id, p_scene.assets().addTexture(name, std::move(picture)));
        m_grounds.push_back({ .sampler = sampler, .texture = id });
    }
    return makeMaterials(p_scene);
}

// ----------------------------------------------------------------------------
//! The PBR shader of the Renderer with our main(), and for the surface the
//! waves in its hook "deform": the renderer still sets every uniform it knows
//! (lights, shadow, fog).
// ----------------------------------------------------------------------------
compages::Status Sea::makeMaterials(compages::renderer::Scene& p_scene)
{
    const compages::gpu::RenderState opaque{
        .depth_test = true, .cull = compages::gpu::CullMode::Back
    };
    const std::string swells = swellsGlsl();

    compages::renderer::shaders::ShaderPair ground =
        compages::renderer::shaders::pbr();
    ground.fragment()
        .include("archipelago/swells.glsl", swells)
        .hook("main", MAP_MAIN);
    compages::renderer::Material map;
    COMPAGES_TRY_ASSIGN(
        map,
        compages::renderer::Material::create(
            "ArchipelagoMap",
            { .family = compages::renderer::ShaderFamily::PbrMinimal,
              .shaders = std::move(ground),
              .state = opaque,
              .see_through = std::nullopt }));
    map.textures = m_grounds;
    COMPAGES_TRY_ASSIGN(m_map,
                        p_scene.assets().addMaterial(map.name, std::move(map)));

    compages::renderer::shaders::ShaderPair water =
        compages::renderer::shaders::pbr();
    water.vertex()
        .include("archipelago/swells.glsl", swells)
        .hook("deform", SURFACE_WAVES);
    water.fragment()
        .include("archipelago/swells.glsl", swells)
        .hook("main", SURFACE_MAIN);
    compages::renderer::Material surface;
    COMPAGES_TRY_ASSIGN(
        surface,
        compages::renderer::Material::create(
            "ArchipelagoSurface",
            { .family = compages::renderer::ShaderFamily::PbrMinimal,
              .shaders = std::move(water),
              .state = opaque,
              // Over the sea bed, blended, without writing the depth.
              .see_through = compages::gpu::RenderState{
                  .depth_test = true,
                  .depth_write = false,
                  .blend = compages::gpu::Blend::alpha(),
                  .cull = compages::gpu::CullMode::Back } }));
    COMPAGES_TRY_ASSIGN(
        m_surface_material,
        p_scene.assets().addMaterial(surface.name, std::move(surface)));
    return compages::success();
}

// ----------------------------------------------------------------------------
compages::Status Sea::show(compages::renderer::Scene& p_scene,
                           Patch const& p_base)
{
    constexpr int CELLS_PER_TILE = BASE_CELLS / TILES;
    const bool first = m_tiles.empty();
    if (first)
    {
        COMPAGES_TRY_ASSIGN(
            m_picture,
            p_scene.assets().addTexture("Archipelago map",
                                        pictureOf("Archipelago map", p_base)));
    }
    else if (compages::renderer::TextureAsset* picture =
                 p_scene.assets().texture(m_picture))
    {
        COMPAGES_TRY(picture->texture.write(p_base.picture));
        COMPAGES_TRY(picture->texture.generateMipmaps());
        picture->pixels = p_base.picture;
    }
    else
    {
        return compages::failure("The map lost its picture");
    }
    for (int tz = 0; tz < TILES; ++tz)
    {
        for (int tx = 0; tx < TILES; ++tx)
        {
            const Square square{ .i = tx * CELLS_PER_TILE,
                                 .j = tz * CELLS_PER_TILE,
                                 .cells = CELLS_PER_TILE,
                                 .uv = Vector2f(float(tx * CELLS_PER_TILE),
                                                float(tz * CELLS_PER_TILE)) *
                                       (1.0f / float(BASE_CELLS)),
                                 .uv_step = 1.0f / float(BASE_CELLS) };
            compages::renderer::MeshAsset mesh = meshOf(p_base, square, 0.0f);
            if (!first)
            {
                compages::world::Entity tile =
                    m_tiles[std::size_t((tz * TILES) + tx)];
                COMPAGES_TRY(p_scene.reshape(tile, std::move(mesh)));
                tile.enable(true);
                continue;
            }
            compages::world::Entity tile =
                p_scene.mesh(std::move(mesh), "Map tile");
            p_scene.look(tile, m_picture);
            p_scene.look(tile, m_map);
            m_tiles.push_back(tile);
        }
    }
    return first ? makeOceanAndSurface(p_scene) : compages::success();
}

// ----------------------------------------------------------------------------
//! The ocean is outside the shadows: the shadow picture stays fitted to the
//! islands. The surface is see-through (opacity under one), so it is drawn
//! after the ground; update() scales and moves it.
// ----------------------------------------------------------------------------
compages::Status Sea::makeOceanAndSurface(compages::renderer::Scene& p_scene)
{
    m_ocean = p_scene.mesh(oceanMesh(), "Ocean");
    p_scene.look(m_ocean, m_picture);
    p_scene.look(m_ocean, m_map);
    m_ocean.get<compages::renderer::MeshRenderer>().flags =
        compages::renderer::RenderFlags::NoShadowCaster;

    m_surface = p_scene.mesh(gridMesh(SURFACE_CELLS), "Sea surface");
    p_scene.look(m_surface, m_picture);
    m_surface.get<compages::renderer::MeshRenderer>().flags =
        compages::renderer::RenderFlags::NoShadowCaster;
    p_scene.look(m_surface, m_surface_material);
    if (compages::renderer::MaterialInstance* instance =
            p_scene.assets().materialInstance(
                m_surface.get<compages::renderer::MeshRenderer>()
                    .material_instance))
    {
        instance->opacity = 0.99f;
    }
    return compages::success();
}

// ----------------------------------------------------------------------------
void Sea::update(compages::renderer::Scene& p_scene,
                 compages::world::RigController const& p_rig,
                 float p_time)
{
    float step = 1.0f;
    if (const float distance = p_rig.currentDistance();
        m_surface && (distance <= SURFACE_WITHIN))
    {
        // As wide as twice the distance; its corners on a grid of its own
        // step, so that the waves do not swim when it follows the camera.
        const float size = std::clamp(2.0f * distance,
                                      compages::toMetres(1.2_km),
                                      compages::toMetres(7.0_km));
        step = size / float(SURFACE_CELLS);
        const Vector3f target = p_rig.currentTarget();
        m_surface_centre = Vector2f(std::round(target.x / step) * step,
                                    std::round(target.z / step) * step);
        m_surface_radius = 0.5f * size;
        m_surface.enable(true)
            .position(m_surface_centre.x, 0.0f, m_surface_centre.y)
            .scale(size, 1.0f, size);
    }
    else
    {
        if (m_surface)
        {
            m_surface.enable(false);
        }
        m_surface_radius = -1.0f;
    }
    for (const compages::renderer::MaterialId id :
         { m_map, m_surface_material })
    {
        if (compages::renderer::Material* material =
                p_scene.assets().material(id))
        {
            material->program.set("time", p_time);
            material->program.set("surfaceCentre", m_surface_centre);
            material->program.set("surfaceRadius", m_surface_radius);
        }
    }
    if (compages::renderer::Material* surface =
            p_scene.assets().material(m_surface_material))
    {
        surface->program.set("gridStep", step);
    }
}

} // namespace examples::archipelago
