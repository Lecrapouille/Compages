// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Highlight.hpp"
#include "10_Complete/Archipelago/Cells.hpp"
#include "10_Complete/Archipelago/Meshes.hpp"

#include "Compages/GPU/Errors.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using compages::renderer::MeshAsset;
using compages::renderer::MeshVertex;
using units::literals::operator""_m;

//! \brief Each edge of a cell is cut in this many pieces, so that the veil
//! follows the ground.
static constexpr int PIECES = 4;

//! \brief A point of the overlay: on the ground (never under the sea), a
//! little over it.
static MeshVertex onGround(Land const& p_land, Vector2f p_at, float p_lift)
{
    MeshVertex vertex;
    vertex.position =
        Vector3f(p_at.x,
                 std::max(0.0f, p_land.heightAt(p_at.x, p_at.y)) + p_lift,
                 p_at.y);
    vertex.normal = Vector3f::UNIT_Y;
    return vertex;
}

// ----------------------------------------------------------------------------
//! \brief The veil of one cell: a fan from its site, with a ring half way so
//! that it bends with the ground; seen from above and from below.
// ----------------------------------------------------------------------------
static void
veilCell(MeshAsset& p_mesh, Land const& p_land, int p_cell, float p_lift)
{
    const std::vector<Vector2f> corners = p_land.cellPolygon(p_cell);
    std::vector<Vector2f> rim;
    for (std::size_t k = 0u; k < corners.size(); ++k)
    {
        const Vector2f next = corners[(k + 1u) % corners.size()];
        for (int s = 0; s < PIECES; ++s)
        {
            rim.push_back(corners[k] +
                          ((next - corners[k]) * (float(s) / float(PIECES))));
        }
    }
    const Vector2f middle = p_land.sites[std::size_t(p_cell)];
    const auto centre = std::uint32_t(p_mesh.source_vertices.size());
    p_mesh.source_vertices.push_back(onGround(p_land, middle, p_lift));
    for (Vector2f const& point : rim)
    {
        p_mesh.source_vertices.push_back(
            onGround(p_land, (middle + point) * 0.5f, p_lift));
        p_mesh.source_vertices.push_back(onGround(p_land, point, p_lift));
    }
    const auto count = std::uint32_t(rim.size());
    for (std::uint32_t k = 0u; k < count; ++k)
    {
        const std::uint32_t half = centre + 1u + (2u * k);
        const std::uint32_t edge = half + 1u;
        const std::uint32_t next_half = centre + 1u + (2u * ((k + 1u) % count));
        const std::uint32_t next_edge = next_half + 1u;
        p_mesh.source_indices.insert(p_mesh.source_indices.end(),
                                     { centre,
                                       next_half,
                                       half,
                                       half,
                                       next_half,
                                       edge,
                                       edge,
                                       next_half,
                                       next_edge,
                                       centre,
                                       half,
                                       next_half,
                                       half,
                                       edge,
                                       next_half,
                                       edge,
                                       next_edge,
                                       next_half });
    }
}

// ----------------------------------------------------------------------------
//! \brief The band: a ribbon \p p_width wide along each segment of the
//! outline, seen from both sides.
// ----------------------------------------------------------------------------
static MeshAsset
bandOf(Land const& p_land, Region const& p_region, float p_width, float p_lift)
{
    MeshAsset band;
    std::vector<Vector3f> const& border = p_region.border;
    for (std::size_t k = 0u; (k + 1u) < border.size(); k += 2u)
    {
        const Vector2f a(border[k].x, border[k].z);
        const Vector2f b(border[k + 1u].x, border[k + 1u].z);
        const Vector2f along = b - a;
        const float length = std::max(std::hypot(along.x, along.y), 1.0e-3f);
        const Vector2f across =
            Vector2f(-along.y, along.x) * (0.5f * p_width / length);
        const auto first = std::uint32_t(band.source_vertices.size());
        for (Vector2f const& point :
             { a - across, a + across, b + across, b - across })
        {
            band.source_vertices.push_back(onGround(p_land, point, p_lift));
        }
        band.source_indices.insert(band.source_indices.end(),
                                   { first,
                                     first + 1u,
                                     first + 2u,
                                     first,
                                     first + 2u,
                                     first + 3u,
                                     first,
                                     first + 2u,
                                     first + 1u,
                                     first,
                                     first + 3u,
                                     first + 2u });
    }
    return band;
}

// ----------------------------------------------------------------------------
//! \brief A box around the vertices of a mesh, and its index count.
// ----------------------------------------------------------------------------
static void finish(MeshAsset& p_mesh)
{
    p_mesh.index_count = p_mesh.source_indices.size();
    Vector3f low(1.0e9f, 1.0e9f, 1.0e9f);
    Vector3f high(-1.0e9f, -1.0e9f, -1.0e9f);
    for (MeshVertex const& vertex : p_mesh.source_vertices)
    {
        low = Vector3f(std::min(low.x, vertex.position.x),
                       std::min(low.y, vertex.position.y),
                       std::min(low.z, vertex.position.z));
        high = Vector3f(std::max(high.x, vertex.position.x),
                        std::max(high.y, vertex.position.y),
                        std::max(high.z, vertex.position.z));
    }
    p_mesh.local_bounds = compages::AABB::fromCorners(low, high);
}

// ----------------------------------------------------------------------------
void Highlight::create(compages::renderer::Scene& p_scene)
{
    m_veil =
        p_scene.mesh(hiddenMesh(),
                     "Region veil",
                     compages::renderer::seeThrough(
                         compages::renderer::unlit(1.0f, 0.97f, 0.85f), 0.25f));
    m_band = p_scene.mesh(hiddenMesh(),
                          "Region band",
                          compages::renderer::unlit(1.0f, 0.82f, 0.48f));
    for (compages::world::Entity overlay : { m_veil, m_band })
    {
        overlay.get<compages::renderer::MeshRenderer>().flags =
            compages::renderer::RenderFlags::NoShadowCaster;
        overlay.enable(false);
    }
}

// ----------------------------------------------------------------------------
void Highlight::show(compages::renderer::Scene& p_scene,
                     Land const& p_land,
                     int p_region,
                     float p_distance)
{
    // The band keeps about 3 pixels wide from where the camera is.
    const float width =
        std::max(compages::toMetres(3.0_m), 0.0035f * p_distance);
    if ((p_region == m_region) &&
        ((p_region < 0) || (std::abs(width - m_width) < 0.15f * m_width)))
    {
        return;
    }
    m_region = p_region;
    m_width = width;
    const bool shown =
        (p_region >= 0) && (std::size_t(p_region) < p_land.regions.size());
    m_veil.enable(shown);
    m_band.enable(shown);
    if (!shown)
    {
        return;
    }
    const float lift = compages::toMetres(2.0_m) + (0.002f * p_distance);
    MeshAsset veil;
    for (std::size_t c = 0u; c < CELL_COUNT; ++c)
    {
        if (p_land.regions_of[c] == p_region)
        {
            veilCell(veil, p_land, int(c), lift);
        }
    }
    MeshAsset band = bandOf(
        p_land, p_land.regions[std::size_t(p_region)], width, 1.5f * lift);
    for (auto [entity, mesh] :
         { std::pair{ m_veil, &veil }, std::pair{ m_band, &band } })
    {
        if (mesh->source_vertices.empty())
        {
            entity.enable(false);
            continue;
        }
        finish(*mesh);
        if (const compages::Status sent =
                replaceMesh(p_scene, entity, std::move(*mesh));
            !sent)
        {
            compages::gpu::reportError(sent.error());
        }
    }
}

} // namespace examples::archipelago
