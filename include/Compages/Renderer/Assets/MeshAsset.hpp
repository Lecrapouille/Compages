// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Enums.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/Renderer/Assets/Skin.hpp"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{

// ****************************************************************************
//! \brief What one corner of a mesh looks like for the built-in lit shader.
//!
//! This is the vertex the AssetManager fills for the primitives it can build
//! by itself, and the one the glTF loader makes: every material of the
//! Renderer draws this layout, so one mesh can wear any material. Fill it
//! yourself for a mesh made by the program.
//!
//! \code
//! compages::renderer::MeshVertex v;
//! v.position = centre + out * radius;
//! v.normal = out;
//! v.uv = { float(k) / float(AROUND), length };
//! mesh.source_vertices.push_back(v);
//! \endcode
// ****************************************************************************
struct MeshVertex
{
    compages::Vector3f position;
    compages::Vector3f normal;
    //! \brief Texture coordinates. Unused by the lit shader; required by PBR
    //! and by meshes imported from glTF.
    compages::Vector2f uv{ 0.0f, 0.0f };
    //! \brief Four joint indices. Ignored when the mesh is rigid
    //! (\c uJointCount == 0 in the shader).
    compages::Vector4i joints{ 0, 0, 0, 0 };
    //! \brief Four weights, defaulting to a rigid bind on joint 0.
    compages::Vector4f weights{ 1.0f, 0.0f, 0.0f, 0.0f };
};

// ****************************************************************************
//! \brief The GPU-side realization of a mesh, plus its local bounds.
//!
//! Owned by the AssetManager. compages::world::World components refer to it by
//! MeshAssetId, not by pointer or handle: the AssetManager is what keeps the
//! buffers alive.
//!
//! The corners and indices stay on the CPU (source_vertices,
//! source_indices): a mesh can be built without a GPU, read back for picking
//! or for baking (10f_FlowingWater), and sent again. local_bounds must hold
//! every corner: the Renderer skips a mesh whose box is outside the view.
//!
//! \code
//! compages::renderer::MeshAsset mesh;
//! mesh.source_vertices = corners;
//! mesh.source_indices = { 0, 1, 2, 2, 1, 3 };
//! mesh.index_count = mesh.source_indices.size();
//! mesh.local_bounds = compages::AABB::fromCorners(low, high);
//! compages::world::Entity e = scene.mesh(std::move(mesh), "Quad");
//! \endcode
// ****************************************************************************
struct MeshAsset
{
    //! \brief CPU source retained so loading does not require a GPU context.
    std::vector<MeshVertex> source_vertices;
    //! \brief Always 32 bits on the CPU; upload() narrows them to 16 bits on
    //! the GPU when the mesh has fewer than 65536 vertices.
    std::vector<std::uint32_t> source_indices;
    //! \brief Vertex buffer holding \c MeshVertex records.
    compages::gpu::VertexBuffer<MeshVertex> vertices;
    //! \brief Index buffer when \c index_type is UInt16.
    compages::gpu::IndexBuffer<std::uint16_t> short_indices;
    //! \brief Index buffer when \c index_type is UInt32.
    compages::gpu::IndexBuffer<std::uint32_t> long_indices;
    //! \brief Number of indices, cached to avoid a per-draw query.
    std::size_t index_count = 0u;
    //! \brief Kind of indices in the GPU buffer, chosen by upload().
    compages::gpu::IndexType index_type = compages::gpu::IndexType::UInt16;
    //! \brief Bounding box of the mesh in its own local space. What the
    //! renderer transforms per instance before culling.
    compages::AABB local_bounds;
    //! \brief Inverse-bind skeleton, empty when the mesh is rigid.
    SkinAssetId skin{};
    //! \brief Rest-pose vertices used to rebuild \c vertices each frame when
    //! the mesh is skinned. Empty for a rigid mesh.
    std::vector<MeshVertex> rest_pose;
    //! \brief Four joint indices per rest-pose vertex, matching \c rest_pose.
    std::vector<std::uint16_t> joint_indices;
    //! \brief Four weights per rest-pose vertex, matching \c rest_pose.
    std::vector<compages::Vector4f> joint_weights;

    //! \brief The GPU index buffer matching \c index_type.
    [[nodiscard]] compages::gpu::BufferHandle indexBuffer() const
    {
        return (index_type == compages::gpu::IndexType::UInt16)
                   ? short_indices.handle()
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
        vertices = compages::gpu::VertexBuffer<MeshVertex>();
        short_indices = compages::gpu::IndexBuffer<std::uint16_t>();
        long_indices = compages::gpu::IndexBuffer<std::uint32_t>();
    }

    [[nodiscard]] Status upload()
    {
        if (uploaded())
        {
            return success();
        }
        if (source_vertices.empty() || source_indices.empty())
        {
            return failure("mesh has no CPU data to upload");
        }
        COMPAGES_TRY_ASSIGN(vertices,
                            compages::gpu::VertexBuffer<MeshVertex>::from(
                                std::span<const MeshVertex>(source_vertices),
                                compages::gpu::BufferUsage::Static));
        index_count = source_indices.size();
        if (source_vertices.size() <= 65536u)
        {
            std::vector<std::uint16_t> narrow(source_indices.size());
            for (std::size_t i = 0u; i < source_indices.size(); ++i)
            {
                narrow[i] = static_cast<std::uint16_t>(source_indices[i]);
            }
            index_type = compages::gpu::IndexType::UInt16;
            COMPAGES_TRY_ASSIGN(short_indices,
                                compages::gpu::IndexBuffer<std::uint16_t>::from(
                                    std::span<const std::uint16_t>(narrow),
                                    compages::gpu::BufferUsage::Static));
        }
        else
        {
            index_type = compages::gpu::IndexType::UInt32;
            COMPAGES_TRY_ASSIGN(
                long_indices,
                compages::gpu::IndexBuffer<std::uint32_t>::from(
                    std::span<const std::uint32_t>(source_indices),
                    compages::gpu::BufferUsage::Static));
        }
        return success();
    }
};

} // namespace compages::renderer
