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
#include "Compages/Renderer/Procedural/ShaderLib.hpp"

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
//! \brief A pipeline of the PBR vertex layout for a program.
// ----------------------------------------------------------------------------
static compages::Result<compages::gpu::Pipeline>
pipelineOf(compages::gpu::Program& p_program,
           compages::gpu::RenderState const& p_state)
{
    return compages::gpu::Pipeline::create<compages::renderer::MeshVertex>(
        p_program, p_state);
}

// ----------------------------------------------------------------------------
//! \brief The vertex shader of the PBR material, with the waves moving its
//! corners before they are projected.
// ----------------------------------------------------------------------------
static compages::Result<std::string> surfaceVertex()
{
    std::string vertex = compages::renderer::shaders::pbrVertex();
    const std::size_t main = vertex.find("void main()");
    vertex = vertex.substr(0u, main) + swellsGlsl() + SURFACE_WAVES +
             vertex.substr(main);
    for (auto const& [from, to] :
         { std::pair<std::string_view, std::string_view>{
               "vWorldPos = world.xyz;",
               "world = displace(world);\n    vWorldPos = world.xyz;" },
           std::pair<std::string_view, std::string_view>{
               "vUv = uv;", "vUv = uv;\n    vNormal = gWaveNormal;" } })
    {
        const std::size_t at = vertex.find(from);
        if (at == std::string::npos)
        {
            return compages::failure("The PBR vertex shader has no '" +
                                     std::string(from) + "'");
        }
        vertex.replace(at, from.size(), to);
    }
    return vertex;
}

// ----------------------------------------------------------------------------
compages::Status Sea::create(compages::renderer::Scene& p_scene)
{
    // The pictures of the ground (ambientCG, CC0): sRGB, repeated, with
    // mipmaps; on the units the Renderer leaves free (0: the albedo, 1 and
    // 7: the shadows).
    const std::array<char const*, 3> grounds = { "terrain/grass.jpg",
                                                 "terrain/sand.jpg",
                                                 "terrain/rock.jpg" };
    for (std::size_t k = 0u; k < grounds.size(); ++k)
    {
        const std::filesystem::path file = dataPath(grounds[k]);
        if (file.empty())
        {
            return compages::failure(
                std::string(grounds[k]) +
                " is missing from external/Compages-data/");
        }
        compages::gpu::LoadOptions options;
        options.srgb = true;
        options.wrap = compages::gpu::Wrap::Repeat;
        COMPAGES_TRY_ASSIGN(m_grounds[k],
                            compages::gpu::Texture::fromFile(file, options));
    }
    return makeMaterials(p_scene);
}

// ----------------------------------------------------------------------------
//! The PBR shader of the Renderer up to its main(), then ours: the renderer
//! still sets every uniform it knows (lights, shadow, fog).
// ----------------------------------------------------------------------------
compages::Status Sea::makeMaterials(compages::renderer::Scene& p_scene)
{
    const std::string pbr = compages::renderer::shaders::pbrFragment();
    const std::string head =
        pbr.substr(0u, pbr.find("out vec4 oColor;")) + swellsGlsl();
    const compages::gpu::RenderState opaque{
        .depth_test = true, .cull = compages::gpu::CullMode::Back
    };

    compages::renderer::Material map;
    map.name = "ArchipelagoMap";
    map.family = compages::renderer::ShaderFamily::PbrMinimal;
    COMPAGES_TRY(map.program.loadSources(
        compages::renderer::shaders::pbrVertex(), head + MAP_MAIN));
    COMPAGES_TRY_ASSIGN(map.pipeline, pipelineOf(map.program, opaque));
    map.program.set("grassMap", 2);
    map.program.set("sandMap", 3);
    map.program.set("rockMap", 4);
    COMPAGES_TRY_ASSIGN(m_map,
                        p_scene.assets().addMaterial(map.name, std::move(map)));

    compages::renderer::Material surface;
    surface.name = "ArchipelagoSurface";
    surface.family = compages::renderer::ShaderFamily::PbrMinimal;
    std::string vertex;
    COMPAGES_TRY_ASSIGN(vertex, surfaceVertex());
    COMPAGES_TRY(surface.program.loadSources(vertex, head + SURFACE_MAIN));
    COMPAGES_TRY_ASSIGN(surface.pipeline, pipelineOf(surface.program, opaque));
    // Over the sea bed, blended, without writing the depth.
    COMPAGES_TRY_ASSIGN(
        surface.see_through,
        pipelineOf(surface.program,
                   compages::gpu::RenderState{
                       .depth_test = true,
                       .depth_write = false,
                       .blend = compages::gpu::Blend::alpha(),
                       .cull = compages::gpu::CullMode::Back }));
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
                COMPAGES_TRY(replaceMesh(p_scene, tile, std::move(mesh)));
                tile.enable(true);
                continue;
            }
            compages::world::Entity tile =
                p_scene.mesh(std::move(mesh), "Map tile");
            p_scene.look(tile, m_picture);
            COMPAGES_TRY(wear(p_scene, tile, m_map));
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
    COMPAGES_TRY(wear(p_scene, m_ocean, m_map));
    m_ocean.get<compages::renderer::MeshRenderer>().flags =
        compages::renderer::RenderFlags::NoShadowCaster;

    m_surface = p_scene.mesh(gridMesh(SURFACE_CELLS), "Sea surface");
    p_scene.look(m_surface, m_picture);
    m_surface.get<compages::renderer::MeshRenderer>().flags =
        compages::renderer::RenderFlags::NoShadowCaster;
    COMPAGES_TRY(wear(p_scene, m_surface, m_surface_material));
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
    for (std::size_t k = 0u; k < m_grounds.size(); ++k)
    {
        m_grounds[k].bind(std::uint32_t(2u + k));
    }
}

} // namespace examples::archipelago
