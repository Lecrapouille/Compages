// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/FineTiles.hpp"
#include "10_Complete/Archipelago/Meshes.hpp"
#include "10_Complete/Archipelago/Sea.hpp"

#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>

namespace examples::archipelago
{

using compages::Vector3f;
using units::literals::operator""_km;
using units::literals::operator""_m;

//! \brief A fine tile: 256 quads (8 m) and 1024 pixels (2 m). At most
//! DETAILS of them live at once, BUILDING of them are made at once.
static constexpr int DETAIL_CELLS = 256;
static constexpr int DETAIL_PIXELS = 1024;
static constexpr std::size_t DETAILS = 12u;
static constexpr std::size_t BUILDING = 2u;
//! \brief The fine tiles show when the camera is closer than this to the
//! ground, around what it looks at.
static constexpr float DETAILS_WITHIN = compages::toMetres(7.0_km);
//! \brief The skirt of a fine tile: a wall this deep along its edges.
static constexpr float SKIRT = compages::toMetres(25.0_m);

//! \brief The corner (smallest x, z) of a tile of the map.
static compages::Vector2f cornerOf(int p_tile)
{
    return { (-0.5f * MAP) + (float(p_tile % TILES) * TILE),
             (-0.5f * MAP) + (float(p_tile / TILES) * TILE) };
}

// ----------------------------------------------------------------------------
void FineTiles::drop()
{
    for (Detail& detail : m_details)
    {
        detail.job.wait();
        detail.tile = -1;
        detail.ready = false;
        detail.shown = false;
        m_shown_mask = 0u;
        if (detail.entity)
        {
            detail.entity.enable(false);
        }
    }
}

// ----------------------------------------------------------------------------
float FineTiles::step()
{
    return TILE / float(DETAIL_CELLS);
}

// ----------------------------------------------------------------------------
std::uint64_t FineTiles::bitAt(float p_x, float p_z)
{
    const int i = int(std::floor((p_x + (0.5f * MAP)) / TILE));
    const int j = int(std::floor((p_z + (0.5f * MAP)) / TILE));
    if ((i < 0) || (j < 0) || (i >= TILES) || (j >= TILES))
    {
        return 0u;
    }
    return std::uint64_t(1u) << unsigned((j * TILES) + i);
}

// ----------------------------------------------------------------------------
float FineTiles::groundAt(Patch const& p_base, float p_x, float p_z) const
{
    const int i = int(std::floor((p_x + (0.5f * MAP)) / TILE));
    const int j = int(std::floor((p_z + (0.5f * MAP)) / TILE));
    if ((i >= 0) && (j >= 0) && (i < TILES) && (j < TILES) &&
        ((m_shown_mask & bitAt(p_x, p_z)) != 0u))
    {
        const int tile = (j * TILES) + i;
        for (Detail const& detail : m_details)
        {
            if (detail.shown && (detail.tile == tile))
            {
                return detail.ground.heightAt(p_x, p_z);
            }
        }
    }
    return p_base.heightAt(p_x, p_z);
}

// ----------------------------------------------------------------------------
std::size_t FineTiles::shown() const
{
    return std::size_t(std::ranges::count_if(
        m_details,
        [](Detail const& p_d)
        { return p_d.ready && p_d.entity && p_d.entity.enabled(); }));
}

// ----------------------------------------------------------------------------
std::vector<int> FineTiles::wanted(Vector3f p_target, float p_distance)
{
    const float radius = std::clamp(0.9f * p_distance,
                                    compages::toMetres(1.5_km),
                                    compages::toMetres(3.5_km));
    std::vector<std::pair<float, int>> near;
    for (int tile = 0; tile < TILES * TILES; ++tile)
    {
        const compages::Vector2f corner = cornerOf(tile);
        const float dx = std::max(
            { corner.x - p_target.x, 0.0f, p_target.x - (corner.x + TILE) });
        const float dz = std::max(
            { corner.y - p_target.z, 0.0f, p_target.z - (corner.y + TILE) });
        if (const float gap = std::hypot(dx, dz); gap < radius)
        {
            near.emplace_back(gap, tile);
        }
    }
    std::ranges::sort(near);
    std::vector<int> tiles;
    for (std::size_t k = 0u; k < std::min(near.size(), DETAILS - BUILDING - 1u);
         ++k)
    {
        tiles.push_back(near[k].second);
    }
    return tiles;
}

// ----------------------------------------------------------------------------
compages::Status FineTiles::send(compages::renderer::Scene& p_scene,
                                 compages::renderer::MaterialId p_material,
                                 std::size_t p_slot,
                                 Patch const& p_patch)
{
    Detail& detail = m_details[p_slot];
    compages::renderer::MeshAsset mesh =
        meshOf(p_patch,
               Square{ .i = 0,
                       .j = 0,
                       .cells = p_patch.cells,
                       .uv = { 0.0f, 0.0f },
                       .uv_step = 1.0f / float(p_patch.cells) },
               SKIRT);
    if (!detail.entity)
    {
        const std::string name = "Archipelago tile " + std::to_string(p_slot);
        COMPAGES_TRY_ASSIGN(
            detail.texture,
            p_scene.assets().addTexture(name, pictureOf(name, p_patch)));
        detail.entity = p_scene.mesh(std::move(mesh), "Detail tile");
        p_scene.look(detail.entity, detail.texture);
        COMPAGES_TRY(wear(p_scene, detail.entity, p_material));
        return p_scene.prepare();
    }
    compages::renderer::TextureAsset* picture =
        p_scene.assets().texture(detail.texture);
    if (picture == nullptr)
    {
        return compages::failure("A fine tile lost its picture");
    }
    COMPAGES_TRY(replaceMesh(p_scene, detail.entity, std::move(mesh)));
    COMPAGES_TRY(picture->texture.write(p_patch.picture));
    return picture->texture.generateMipmaps();
}

// ----------------------------------------------------------------------------
void FineTiles::receive(compages::renderer::Scene& p_scene,
                        compages::renderer::MaterialId p_material)
{
    for (std::size_t k = 0u; k < m_details.size(); ++k)
    {
        if (!m_details[k].job.ready())
        {
            continue;
        }
        Patch patch = m_details[k].job.take();
        if (const compages::Status sent = send(p_scene, p_material, k, patch);
            !sent)
        {
            compages::gpu::reportError(sent.error());
            m_details[k].tile = -1;
            continue;
        }
        m_details[k].ready = true;
        // The heights stay for groundAt(); the picture is on the GPU now.
        patch.picture.clear();
        patch.picture.shrink_to_fit();
        m_details[k].ground = std::move(patch);
    }
}

// ----------------------------------------------------------------------------
//! A free slot, or the one unused for the longest; at most BUILDING at once.
// ----------------------------------------------------------------------------
void FineTiles::start(Land const& p_land, std::vector<int> const& p_wanted)
{
    auto building = std::size_t(std::ranges::count_if(
        m_details, [](Detail const& p_d) { return p_d.job.busy(); }));
    auto isWanted = [&p_wanted](int p_tile)
    { return std::ranges::find(p_wanted, p_tile) != p_wanted.end(); };
    for (const int tile : p_wanted)
    {
        if ((building >= BUILDING) ||
            std::ranges::any_of(m_details,
                                [tile](Detail const& p_d)
                                { return p_d.tile == tile; }))
        {
            continue;
        }
        Detail* slot = nullptr;
        if (m_details.size() < DETAILS)
        {
            slot = &m_details.emplace_back();
        }
        else
        {
            for (Detail& detail : m_details)
            {
                const bool free = !detail.job.busy() && !isWanted(detail.tile);
                if (free && ((slot == nullptr) || (detail.used < slot->used)))
                {
                    slot = &detail;
                }
            }
        }
        if (slot == nullptr)
        {
            return;
        }
        slot->tile = tile;
        slot->ready = false;
        const compages::Vector2f corner = cornerOf(tile);
        slot->job.start(
            [land = &p_land, corner]()
            {
                return makePatch(*land,
                                 corner.x,
                                 corner.y,
                                 TILE,
                                 DETAIL_CELLS,
                                 DETAIL_PIXELS,
                                 {});
            });
        ++building;
    }
}

// ----------------------------------------------------------------------------
void FineTiles::update(compages::renderer::Scene& p_scene,
                       Land const& p_land,
                       compages::renderer::MaterialId p_material,
                       Vector3f p_target,
                       float p_distance,
                       bool p_on,
                       std::vector<compages::world::Entity>& p_coarse)
{
    ++m_clock;
    const std::vector<int> tiles = (p_on && (p_distance < DETAILS_WITHIN))
                                       ? wanted(p_target, p_distance)
                                       : std::vector<int>{};
    receive(p_scene, p_material);
    start(p_land, tiles);

    // A fine tile ready and wanted takes the place of its coarse one.
    std::vector<char> fine(p_coarse.size(), 0);
    for (Detail& detail : m_details)
    {
        const bool shown =
            detail.ready && (detail.tile >= 0) &&
            (std::ranges::find(tiles, detail.tile) != tiles.end());
        if (detail.entity)
        {
            detail.entity.enable(shown);
        }
        detail.shown = shown;
        if (shown)
        {
            detail.used = m_clock;
            fine[std::size_t(detail.tile)] = 1;
        }
    }
    m_shown_mask = 0u;
    for (std::size_t tile = 0u; tile < p_coarse.size(); ++tile)
    {
        p_coarse[tile].enable(fine[tile] == 0);
        if (fine[tile] != 0)
        {
            m_shown_mask |= std::uint64_t(1u) << unsigned(tile);
        }
    }
}

} // namespace examples::archipelago
