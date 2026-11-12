// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/LightClusters.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Renderer/Render/Offer.hpp"

#include <algorithm>
#include <cmath>
#include <span>

namespace compages::renderer
{

namespace
{

//! \brief The distance in front of the camera of a point of clip space
//! (x = y = 0) at a depth of normalised device coordinates.
float depthAt(compages::Matrix44f const& p_inverse_projection, float p_ndc)
{
    const compages::Vector4f p = p_inverse_projection * compages::Vector4f(0.0f, 0.0f, p_ndc, 1.0f);
    return -p.z / ((std::abs(p.w) > 1.0e-12f) ? p.w : 1.0e-12f);
}

} // namespace

// ----------------------------------------------------------------------------
int LightClusters::sliceOf(float p_depth) const
{
    const float t = std::log(std::max(p_depth, m_near) / m_near) / std::log(m_far / m_near);
    return std::clamp(int(t * float(SLICES)), 0, SLICES - 1);
}

// ----------------------------------------------------------------------------
std::size_t LightClusters::clusterOf(float p_u, float p_v, float p_depth) const
{
    const int x = std::clamp(int(p_u * float(m_tiles_x)), 0, m_tiles_x - 1);
    const int y = std::clamp(int(p_v * float(m_tiles_y)), 0, m_tiles_y - 1);
    return std::size_t((((sliceOf(p_depth) * m_tiles_y) + y) * m_tiles_x) + x);
}

// ----------------------------------------------------------------------------
std::vector<std::uint32_t> LightClusters::lampsOf(std::size_t p_cluster) const
{
    if (2u * p_cluster + 1u >= m_cells.size())
    {
        return {};
    }
    const std::uint32_t first = m_cells[2u * p_cluster];
    const std::uint32_t count = m_cells[(2u * p_cluster) + 1u];
    return { m_indices.begin() + first, m_indices.begin() + first + count };
}

// ----------------------------------------------------------------------------
//! For each lamp, the slices its sphere spans in depth, and the tiles its
//! box spans on the picture (the eight corners of the box round the sphere,
//! projected); every tile when the sphere reaches the near plane.
// ----------------------------------------------------------------------------
void LightClusters::build(RenderSnapshot const& p_snapshot, std::size_t p_first)
{
    CameraFrame const& camera = p_snapshot.camera;
    m_view = camera.view;
    m_near = std::max(depthAt(camera.inverse_projection, -1.0f), 1.0e-4f);
    m_far = std::max(depthAt(camera.inverse_projection, 1.0f), m_near * 1.01f);
    const bool sized = (camera.viewport_width > 0u) && (camera.viewport_height > 0u);
    m_tiles_x = sized ? TILES_X : 1;
    m_tiles_y = sized ? TILES_Y : 1;
    m_viewport = sized ? compages::Vector4f(camera.viewport_x, camera.viewport_y, float(camera.viewport_width),
                                            float(camera.viewport_height))
                       : compages::Vector4f(0.0f, 0.0f, 1.0f, 1.0f);

    m_lamps.clear();
    std::vector<std::vector<std::uint32_t>> lists(CLUSTERS);
    const std::size_t end = std::min(p_snapshot.point_lights.size(), p_first + MAX_LIGHTS);
    for (std::size_t i = p_first; i < end; ++i)
    {
        PointLightFrame const& lamp = p_snapshot.point_lights[i];
        const compages::Vector4f c = camera.view * compages::Vector4f(lamp.position.x, lamp.position.y,
                                                                      lamp.position.z, 1.0f);
        const float r = lamp.range;
        const float closest = -c.z - r;
        const float farthest = -c.z + r;
        if ((farthest < m_near) || (closest > m_far) || (r <= 0.0f))
        {
            continue;
        }
        const auto index = std::uint32_t(m_lamps.size() / 2u);
        m_lamps.emplace_back(lamp.position.x, lamp.position.y, lamp.position.z, r);
        const compages::Vector3f colour = lamp.color * lamp.intensity;
        m_lamps.emplace_back(colour.x, colour.y, colour.z, 0.0f);

        int x0 = 0;
        int x1 = m_tiles_x - 1;
        int y0 = 0;
        int y1 = m_tiles_y - 1;
        if (closest > m_near)
        {
            float u0 = 1.0f;
            float u1 = 0.0f;
            float v0 = 1.0f;
            float v1 = 0.0f;
            for (int corner = 0; corner < 8; ++corner)
            {
                const compages::Vector4f p((corner & 1) ? c.x + r : c.x - r, (corner & 2) ? c.y + r : c.y - r,
                                           (corner & 4) ? c.z + r : c.z - r, 1.0f);
                const compages::Vector4f clip = camera.projection * p;
                const float u = (clip.x / clip.w) * 0.5f + 0.5f;
                const float v = (clip.y / clip.w) * 0.5f + 0.5f;
                u0 = std::min(u0, u);
                u1 = std::max(u1, u);
                v0 = std::min(v0, v);
                v1 = std::max(v1, v);
            }
            if ((u1 < 0.0f) || (u0 > 1.0f) || (v1 < 0.0f) || (v0 > 1.0f))
            {
                continue;
            }
            x0 = std::clamp(int(u0 * float(m_tiles_x)), 0, m_tiles_x - 1);
            x1 = std::clamp(int(u1 * float(m_tiles_x)), 0, m_tiles_x - 1);
            y0 = std::clamp(int(v0 * float(m_tiles_y)), 0, m_tiles_y - 1);
            y1 = std::clamp(int(v1 * float(m_tiles_y)), 0, m_tiles_y - 1);
        }
        const int s0 = sliceOf(std::max(closest, m_near));
        const int s1 = sliceOf(std::min(farthest, m_far));
        for (int s = s0; s <= s1; ++s)
        {
            for (int y = y0; y <= y1; ++y)
            {
                for (int x = x0; x <= x1; ++x)
                {
                    lists[std::size_t((((s * m_tiles_y) + y) * m_tiles_x) + x)].push_back(index);
                }
            }
        }
    }

    // One list after the other; what does not fit is left out.
    m_cells.assign(2u * CLUSTERS, 0u);
    m_indices.clear();
    for (std::size_t k = 0u; k < CLUSTERS; ++k)
    {
        const std::size_t room = MAX_INDICES - m_indices.size();
        const std::size_t taken = std::min(lists[k].size(), room);
        m_cells[2u * k] = std::uint32_t(m_indices.size());
        m_cells[(2u * k) + 1u] = std::uint32_t(taken);
        m_indices.insert(m_indices.end(), lists[k].begin(), lists[k].begin() + std::ptrdiff_t(taken));
    }
}

// ----------------------------------------------------------------------------
Status LightClusters::upload()
{
    if (!m_lamp_buffer.valid())
    {
        COMPAGES_TRY_ASSIGN(m_lamp_buffer, compages::gpu::StorageBuffer<compages::Vector4f>::create(
                                               2u * MAX_LIGHTS, compages::gpu::BufferUsage::Stream));
        COMPAGES_TRY_ASSIGN(m_cell_buffer, compages::gpu::StorageBuffer<std::uint32_t>::create(
                                               2u * CLUSTERS, compages::gpu::BufferUsage::Stream));
        COMPAGES_TRY_ASSIGN(m_index_buffer, compages::gpu::StorageBuffer<std::uint32_t>::create(
                                                MAX_INDICES, compages::gpu::BufferUsage::Stream));
    }
    if (!m_lamps.empty())
    {
        m_lamp_buffer.write(std::span<const compages::Vector4f>(m_lamps));
        m_cell_buffer.write(std::span<const std::uint32_t>(m_cells));
        if (!m_indices.empty())
        {
            m_index_buffer.write(std::span<const std::uint32_t>(m_indices));
        }
    }
    return success();
}

// ----------------------------------------------------------------------------
Status LightClusters::bind(compages::gpu::Program& p_program) const
{
    if (!p_program.has("clusterLightCount"))
    {
        return success();
    }
    const int count = m_lamp_buffer.valid() ? int(m_lamps.size() / 2u) : 0;
    p_program.set("clusterLightCount", count);
    if (count == 0)
    {
        return success();
    }
    offer(p_program, "clusterTiles", compages::Vector2i(m_tiles_x, m_tiles_y));
    offer(p_program, "clusterSlices", SLICES);
    offer(p_program, "clusterDepth", compages::Vector2f(m_near, m_far));
    offer(p_program, "clusterViewport", m_viewport);
    offer(p_program, "clusterView", m_view);
    COMPAGES_TRY(p_program.bind("ClusterLights", m_lamp_buffer.handle()));
    COMPAGES_TRY(p_program.bind("ClusterCells", m_cell_buffer.handle()));
    COMPAGES_TRY(p_program.bind("ClusterIndices", m_index_buffer.handle()));
    return success();
}

} // namespace compages::renderer
