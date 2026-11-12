// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// Internal to the glTF loader: turns the cgltf structures into assets. The
// loader (GltfLoader.cpp) walks the nodes and builds the prefab with them.

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"

#include "CgltfInclude.hpp"

#include <cstddef>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace compages::renderer::gltf
{

//! \brief The pictures of one file: those read so far (an image used by
//! several materials is read once), and the folder of the file, where the
//! pictures that are files of their own are.
struct TextureMap
{
    std::filesystem::path folder;
    std::unordered_map<cgltf_image*, TextureAssetId> read;
};

//! \brief The mesh of one primitive: positions, normals, UVs, joints.
[[nodiscard]] Result<MeshAsset>
meshFromPrimitive(cgltf_primitive const* p_primitive);

//! \brief The material instance of a glTF material, with its pictures.
[[nodiscard]] Result<MaterialInstanceId>
materialInstanceFromGltf(cgltf_material const* p_material,
                         MaterialId p_shared_material,
                         AssetManager& p_assets,
                         TextureMap& p_textures,
                         std::size_t& p_texture_count);

//! \brief The inverse bind matrices of a skin, as an asset.
[[nodiscard]] Result<SkinAssetId>
importSkinAsset(cgltf_skin* p_skin, AssetManager& p_assets, std::size_t p_index);

//! \brief The animations of the file, as clips keyed by node index.
[[nodiscard]] Status importAnimations(cgltf_data* p_data,
                                      AssetManager& p_assets,
                                      std::string const& p_path,
                                      std::vector<AnimationClipId>& p_result);

[[nodiscard]] inline std::size_t nodeIndex(cgltf_data const* p_data,
                                    cgltf_node const* p_node)
{
    return static_cast<std::size_t>(p_node - p_data->nodes);
}

// --- Node transform ----------------------------------------------------------
// Map glTF matrix or TRS into compages::world::LocalTransform / PrefabNode
// fields.

template <typename Transform>
void applyNodeTransform(cgltf_node const* p_node, Transform&& p_local)
{
    if (p_node->has_matrix)
    {
        // glTF matrices are column-major, column-vector: translation sits at
        // indices 12, 13, 14 and the first three columns are the scaled basis.
        // compages::Quatf::fromMatrix() reads a mathematical
        // (column-vector) rotation.
        p_local.position = compages::Vector3f(
            p_node->matrix[12], p_node->matrix[13], p_node->matrix[14]);

        const compages::Vector3f axis_x(
            p_node->matrix[0], p_node->matrix[1], p_node->matrix[2]);
        const compages::Vector3f axis_y(
            p_node->matrix[4], p_node->matrix[5], p_node->matrix[6]);
        const compages::Vector3f axis_z(
            p_node->matrix[8], p_node->matrix[9], p_node->matrix[10]);
        const float scale_x = compages::vector::norm(axis_x);
        const float scale_y = compages::vector::norm(axis_y);
        const float scale_z = compages::vector::norm(axis_z);
        p_local.scale = compages::Vector3f(scale_x, scale_y, scale_z);

        compages::Matrix44f rotation_matrix(
            compages::matrix::Identity);
        if (scale_x > 1.0e-8f)
        {
            const compages::Vector3f n = axis_x / scale_x;
            rotation_matrix(0, 0) = n.x;
            rotation_matrix(1, 0) = n.y;
            rotation_matrix(2, 0) = n.z;
        }
        if (scale_y > 1.0e-8f)
        {
            const compages::Vector3f n = axis_y / scale_y;
            rotation_matrix(0, 1) = n.x;
            rotation_matrix(1, 1) = n.y;
            rotation_matrix(2, 1) = n.z;
        }
        if (scale_z > 1.0e-8f)
        {
            const compages::Vector3f n = axis_z / scale_z;
            rotation_matrix(0, 2) = n.x;
            rotation_matrix(1, 2) = n.y;
            rotation_matrix(2, 2) = n.z;
        }
        p_local.rotation = compages::Quatf::fromMatrix(rotation_matrix);
        return;
    }

    if (p_node->has_translation)
    {
        p_local.position = compages::Vector3f(p_node->translation[0],
                                                    p_node->translation[1],
                                                    p_node->translation[2]);
    }
    if (p_node->has_rotation)
    {
        p_local.rotation = compages::Quatf(p_node->rotation[3],
                                                 p_node->rotation[0],
                                                 p_node->rotation[1],
                                                 p_node->rotation[2]);
    }
    if (p_node->has_scale)
    {
        p_local.scale = compages::Vector3f(
            p_node->scale[0], p_node->scale[1], p_node->scale[2]);
    }
}

} // namespace compages::renderer::gltf
