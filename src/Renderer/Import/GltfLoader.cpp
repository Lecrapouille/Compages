// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Assets/GltfLoader.hpp"

#include "Compages/Renderer/Assets/AnimationClip.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"
#include "Compages/Renderer/Assets/Skin.hpp"
#include "Compages/Renderer/Components/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/World.hpp"

#include "Compages/GPU/Device.hpp"

#include "GltfConvert.hpp"


#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace compages::renderer
{

namespace
{

using namespace gltf;

// --- Prefab tree -------------------------------------------------------------
// Headless path: one PrefabNode per glTF node; extra primitives become
// children.

[[nodiscard]] Result<PrefabNode>
loadPrefabNode(cgltf_node* p_node,
               cgltf_data* p_data,
               AssetManager& p_assets,
               MaterialId p_shared_material,
               TextureMap& p_textures,
               std::size_t& p_texture_count,
               std::unordered_map<cgltf_skin*, SkinAssetId>& p_skins,
               std::string const& p_asset_name)
{
    PrefabNode result;
    result.source_index = static_cast<std::uint32_t>(nodeIndex(p_data, p_node));
    result.name = (p_node->name != nullptr) ? p_node->name : "gltf-node";
    applyNodeTransform(p_node, result.transform);

    // One renderer per entity: the first primitive of the mesh is drawn by
    // the node itself, each other one by a child of it, at the same place,
    // with the same skin. A mesh made of several materials is several
    // primitives, and loses parts of itself otherwise.
    std::vector<PrefabNode> extra_primitives;
    const std::size_t primitives =
        (p_node->mesh != nullptr) ? p_node->mesh->primitives_count : 0u;
    for (std::size_t p = 0u; p < primitives; ++p)
    {
        cgltf_primitive* primitive = &p_node->mesh->primitives[p];
        PrefabNode extra;
        PrefabNode& target = (p == 0u) ? result : extra;
        if (p != 0u)
        {
            // Past every index of the file, distinct for each node and
            // primitive: what skins and clips name are never these.
            extra.source_index = static_cast<std::uint32_t>(
                p_data->nodes_count + 1u + (result.source_index * 16u) + p);
            extra.name = result.name + "#" + std::to_string(p);
        }
        auto mesh_result = meshFromPrimitive(primitive);
        if (!mesh_result)
        {
            return failure(mesh_result.error());
        }
        auto mesh = mesh_result.take();
        const std::string mesh_name =
            p_asset_name + "#node-" + std::to_string(result.source_index) +
            "-mesh" + ((p == 0u) ? std::string() : ("-" + std::to_string(p)));
        auto mesh_id_result = p_assets.addMesh(mesh_name, std::move(mesh));
        if (!mesh_id_result)
        {
            return failure(mesh_id_result.error());
        }
        auto mesh_id = mesh_id_result.take();
        auto material_id_result = materialInstanceFromGltf(primitive->material,
                                                           p_shared_material,
                                                           p_assets,
                                                           p_textures,
                                                           p_texture_count);
        if (!material_id_result)
        {
            return failure(material_id_result.error());
        }
        auto material_id = material_id_result.take();

        target.mesh_renderer =
            PrefabMeshRenderer{ p_assets.meshName(mesh_id),
                                p_assets.materialInstanceName(material_id),
                                compages::renderer::RenderFlags::None };

        if (p_node->skin != nullptr)
        {
            SkinAssetId skin_id;
            auto const cached = p_skins.find(p_node->skin);
            if (cached != p_skins.end())
            {
                skin_id = cached->second;
            }
            else
            {
                const std::size_t skin_index =
                    static_cast<std::size_t>(p_node->skin - p_data->skins);
                auto created_result =
                    importSkinAsset(p_node->skin, p_assets, skin_index);
                if (!created_result)
                {
                    return failure(created_result.error());
                }
                auto created = created_result.take();
                skin_id = created;
                p_skins.emplace(p_node->skin, skin_id);
            }
            if (MeshAsset* stored = p_assets.mesh(mesh_id); stored != nullptr)
            {
                stored->skin = skin_id;
            }

            PrefabSkinInstance skin;
            skin.joints.reserve(p_node->skin->joints_count);
            for (std::size_t i = 0u; i < p_node->skin->joints_count; ++i)
            {
                skin.joints.emplace_back(static_cast<std::uint32_t>(
                    nodeIndex(p_data, p_node->skin->joints[i])));
            }
            target.skin_instance = std::move(skin);
        }
        if (p != 0u)
        {
            extra_primitives.emplace_back(std::move(extra));
        }
    }

    result.children.reserve(p_node->children_count);
    for (std::size_t i = 0u; i < p_node->children_count; ++i)
    {
        auto child_result = loadPrefabNode(p_node->children[i],
                                           p_data,
                                           p_assets,
                                           p_shared_material,
                                           p_textures,
                                           p_texture_count,
                                           p_skins,
                                           p_asset_name);
        if (!child_result)
        {
            return failure(child_result.error());
        }
        auto child = child_result.take();
        result.children.emplace_back(std::move(child));
    }
    for (PrefabNode& extra : extra_primitives)
    {
        result.children.emplace_back(std::move(extra));
    }
    return result;
}

} // namespace

// --- Public: reusable prefab -------------------------------------------------
// Parse file, build Prefab + animations; no compages::world::World entities.

Result<PrefabId> loadGltf(std::string const& p_path,
                          AssetManager& p_assets,
                          MaterialId p_shared_material)
{
    cgltf_options options{};
    cgltf_data* raw = nullptr;
    if (cgltf_parse_file(&options, p_path.c_str(), &raw) !=
        cgltf_result_success)
    {
        return failure("cgltf_parse_file failed for '" + p_path + "'");
    }
    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> data(raw, &cgltf_free);
    if (cgltf_load_buffers(&options, data.get(), p_path.c_str()) !=
        cgltf_result_success)
    {
        return failure("cgltf_load_buffers failed for '" + p_path + "'");
    }
    if ((data->scenes_count == 0u) || (data->scene == nullptr))
    {
        return failure("glTF file has no default scene");
    }

    MaterialId shared_material = p_shared_material;
    if (!shared_material.valid())
    {
        Material material;
        if (compages::gpu::initialized())
        {
            COMPAGES_TRY_ASSIGN(material, makePbrMaterial());
        }
        else
        {
            material.name = "gltf-pbr";
            material.family = ShaderFamily::PbrMinimal;
        }
        auto created_result =
            p_assets.addMaterial("gltf-pbr", std::move(material));
        if (!created_result)
        {
            return failure(created_result.error());
        }
        auto created = created_result.take();
        shared_material = created;
    }

    Prefab prefab;
    prefab.name = p_path;
    prefab.root.name = "gltf-root";
    prefab.root.source_index = static_cast<std::uint32_t>(data->nodes_count);

    TextureMap textures;
    std::size_t texture_count = 0u;
    std::unordered_map<cgltf_skin*, SkinAssetId> skins;
    prefab.root.children.reserve(data->scene->nodes_count);
    for (std::size_t i = 0u; i < data->scene->nodes_count; ++i)
    {
        auto child_result = loadPrefabNode(data->scene->nodes[i],
                                           data.get(),
                                           p_assets,
                                           shared_material,
                                           textures,
                                           texture_count,
                                           skins,
                                           p_path);
        if (!child_result)
        {
            return failure(child_result.error());
        }
        auto child = child_result.take();
        prefab.root.children.emplace_back(std::move(child));
    }
    COMPAGES_TRY(
        importAnimations(data.get(), p_assets, p_path, prefab.animations));

    return p_assets.addPrefab(p_path, std::move(prefab));
}

} // namespace compages::renderer
