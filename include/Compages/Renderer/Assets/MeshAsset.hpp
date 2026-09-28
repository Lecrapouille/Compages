//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#pragma once

#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/Renderer/Assets/Skin.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Core/Enums.hpp"
#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Vector.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief What one corner of a mesh looks like for the built-in lit shader.
//!
//! This is the vertex the AssetManager fills for the primitives it can build
//! by itself. A GLB loader will produce vertices of its own struct that share
//! this same layout, so the same pipeline can draw both.
// ****************************************************************************
struct MeshVertex
{
    Vector3f position;
    Vector3f normal;
    //! \brief Texture coordinates. Unused by the lit shader; required by PBR
    //! and by meshes imported from glTF.
    Vector2f uv{ 0.0f, 0.0f };
    //! \brief Four joint indices. Ignored when the mesh is rigid
    //! (\c uJointCount == 0 in the shader).
    Vector4i joints{ 0, 0, 0, 0 };
    //! \brief Four weights, defaulting to a rigid bind on joint 0.
    Vector4f weights{ 1.0f, 0.0f, 0.0f, 0.0f };
};

// ****************************************************************************
//! \brief The GPU-side realization of a mesh, plus its local bounds.
//!
//! Owned by the AssetManager. World components refer to it by MeshAssetId, not
//! by pointer or handle: the AssetManager is what keeps the buffers alive.
// ****************************************************************************
struct MeshAsset
{
    //! \brief CPU source retained so loading does not require a GPU context.
    std::vector<MeshVertex> source_vertices;
    //! \brief Always 32 bits on the CPU; upload() narrows them to 16 bits on
    //! the GPU when the mesh has fewer than 65536 vertices.
    std::vector<std::uint32_t> source_indices;
    //! \brief Vertex buffer holding \c MeshVertex records.
    compages::gpu::Buffer<MeshVertex> vertices;
    //! \brief Index buffer when \c index_type is UInt16.
    compages::gpu::Buffer<std::uint16_t> short_indices;
    //! \brief Index buffer when \c index_type is UInt32.
    compages::gpu::Buffer<std::uint32_t> long_indices;
    //! \brief Number of indices, cached to avoid a per-draw query.
    std::size_t index_count = 0u;
    //! \brief Kind of indices in the GPU buffer, chosen by upload().
    compages::gpu::IndexType index_type = compages::gpu::IndexType::UInt16;
    //! \brief Bounding box of the mesh in its own local space. What the
    //! renderer transforms per instance before culling.
    AABB local_bounds;
    //! \brief Inverse-bind skeleton, empty when the mesh is rigid.
    SkinAssetId skin{};
    //! \brief Rest-pose vertices used to rebuild \c vertices each frame when
    //! the mesh is skinned. Empty for a rigid mesh.
    std::vector<MeshVertex> rest_pose;
    //! \brief Four joint indices per rest-pose vertex, matching \c rest_pose.
    std::vector<std::uint16_t> joint_indices;
    //! \brief Four weights per rest-pose vertex, matching \c rest_pose.
    std::vector<Vector4f> joint_weights;

    //! \brief The GPU index buffer matching \c index_type.
    [[nodiscard]] compages::gpu::BufferHandle indexBuffer() const
    {
        return (index_type == compages::gpu::IndexType::UInt16) ? short_indices.handle()
                                                      : long_indices.handle();
    }

    //! \brief Are the vertices and the indices on the GPU?
    [[nodiscard]] bool uploaded() const
    {
        return vertices.valid() &&
               (short_indices.valid() || long_indices.valid());
    }

    //! \brief Forget the GPU buffers, so that the next upload() sends the CPU
    //! source again.
    void releaseGpu()
    {
        vertices = compages::gpu::Buffer<MeshVertex>();
        short_indices = compages::gpu::Buffer<std::uint16_t>();
        long_indices = compages::gpu::Buffer<std::uint32_t>();
    }

    [[nodiscard]] compages::Status upload()
    {
        if (uploaded())
        {
            return compages::success();
        }
        if (source_vertices.empty() || source_indices.empty())
        {
            return compages::failure("mesh has no CPU data to upload");
        }
        COMPAGES_TRY_ASSIGN(
            vertices,
            compages::gpu::Buffer<MeshVertex>::from(
                std::span<const MeshVertex>(source_vertices),
                compages::gpu::BufferKind::Vertex,
                compages::gpu::BufferUsage::Immutable));
        index_count = source_indices.size();
        if (source_vertices.size() <= 65536u)
        {
            std::vector<std::uint16_t> narrow(source_indices.size());
            for (std::size_t i = 0u; i < source_indices.size(); ++i)
            {
                narrow[i] = static_cast<std::uint16_t>(source_indices[i]);
            }
            index_type = compages::gpu::IndexType::UInt16;
            COMPAGES_TRY_ASSIGN(
                short_indices,
                compages::gpu::Buffer<std::uint16_t>::from(
                    std::span<const std::uint16_t>(narrow),
                    compages::gpu::BufferKind::Index,
                    compages::gpu::BufferUsage::Immutable));
        }
        else
        {
            index_type = compages::gpu::IndexType::UInt32;
            COMPAGES_TRY_ASSIGN(
                long_indices,
                compages::gpu::Buffer<std::uint32_t>::from(
                    std::span<const std::uint32_t>(source_indices),
                    compages::gpu::BufferKind::Index,
                    compages::gpu::BufferUsage::Immutable));
        }
        return compages::success();
    }
};

} // namespace compages::renderer
