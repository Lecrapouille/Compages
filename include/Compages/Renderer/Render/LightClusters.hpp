// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/Renderer/Render/RenderSnapshot.hpp"

#include <cstdint>
#include <vector>

namespace compages::gpu
{
class Program;
} // namespace compages::gpu

namespace compages::renderer
{

// ****************************************************************************
//! \brief Many lamps for a picture: clustered forward lighting (O. Olsson,
//! M. Billeter and U. Assarsson, "Clustered Deferred and Forward Shading",
//! HPG 2012).
//!
//! The four lamps nearest the camera keep their own uniforms and their
//! shadows. The others, up to MAX_LIGHTS, are sorted into clusters: the
//! picture is cut into TILES_X x TILES_Y tiles, and the depth into SLICES
//! slices whose thickness grows with the distance (the slice of a depth z is
//! log(z / near) / log(far / near) times SLICES). A lamp goes into each
//! cluster its sphere of light may touch. A pixel then reads only the lamps
//! of its cluster: a hundred lamps in a level cost a pixel a few of them.
//!
//! Built on the CPU at each render (a few hundred lamps take well under a
//! millisecond) into three storage buffers that the lit shaders read
//! (<compages/lamps.glsl>, bindings 9, 10 and 11):
//! - ClusterLights: per lamp, (position, range) and (colour x intensity, 0);
//! - ClusterCells: per cluster, (first index, count);
//! - ClusterIndices: the lamps of each cluster, one after the other.
// ****************************************************************************
class LightClusters: private compages::NotCopiable
{
public:

    static constexpr int TILES_X = 16;
    static constexpr int TILES_Y = 9;
    static constexpr int SLICES = 24;
    static constexpr std::size_t MAX_LIGHTS = 256u;
    static constexpr std::size_t MAX_INDICES = 65536u;
    static constexpr std::size_t CLUSTERS = std::size_t(TILES_X * TILES_Y * SLICES);

    LightClusters() = default;

    // ------------------------------------------------------------------------
    //! \brief Sort the lamps of a snapshot, from \c p_first on, into the
    //! clusters of its camera.
    //! \param[in] p_first The lamps before it have uniforms of their own.
    // ------------------------------------------------------------------------
    void build(RenderSnapshot const& p_snapshot, std::size_t p_first);

    // ------------------------------------------------------------------------
    //! \brief Send the clusters to the GPU (the buffers are made once).
    // ------------------------------------------------------------------------
    [[nodiscard]] Status upload();

    // ------------------------------------------------------------------------
    //! \brief Give a program the clusters, when it reads them.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bind(compages::gpu::Program& p_program) const;

    //! \brief The cluster of a point of view space (z < 0 in front) seen
    //! at a pixel (from 0 to 1 across the viewport), as the shader finds it.
    [[nodiscard]] std::size_t clusterOf(float p_u, float p_v, float p_depth) const;

    //! \brief The lamps of a cluster, as indices into the lamps sorted.
    [[nodiscard]] std::vector<std::uint32_t> lampsOf(std::size_t p_cluster) const;

    //! \brief How many lamps were sorted into clusters.
    [[nodiscard]] std::size_t lampCount() const
    {
        return m_lamps.size() / 2u;
    }

    //! \brief Near and far distances of the slices.
    [[nodiscard]] float nearDepth() const
    {
        return m_near;
    }

    [[nodiscard]] float farDepth() const
    {
        return m_far;
    }

private:

    [[nodiscard]] int sliceOf(float p_depth) const;

    std::vector<compages::Vector4f> m_lamps;
    std::vector<std::uint32_t> m_cells;
    std::vector<std::uint32_t> m_indices;
    float m_near = 0.1f;
    float m_far = 100.0f;
    int m_tiles_x = TILES_X;
    int m_tiles_y = TILES_Y;
    compages::Vector4f m_viewport{ 0.0f, 0.0f, 1.0f, 1.0f };
    compages::Matrix44f m_view{ compages::matrix::Identity };
    compages::gpu::StorageBuffer<compages::Vector4f> m_lamp_buffer;
    compages::gpu::StorageBuffer<std::uint32_t> m_cell_buffer;
    compages::gpu::StorageBuffer<std::uint32_t> m_index_buffer;
};

} // namespace compages::renderer
