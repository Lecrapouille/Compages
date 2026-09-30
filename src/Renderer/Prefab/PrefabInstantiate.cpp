// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Prefab/PrefabInstantiate.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Prefab.hpp"
#include "Compages/Renderer/Components/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Components/PrefabInstance.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/World.hpp"

#include <utility>
#include <vector>

namespace compages::renderer
{

namespace
{

Status
spawnNode(compages::world::World& p_world,
          compages::renderer::AssetManager& p_assets,
          compages::renderer::PrefabId p_prefab,
          compages::renderer::PrefabNode const& p_node,
          compages::world::EntityId p_parent,
          compages::world::EntityId& p_root_out,
          bool p_is_root,
          compages::world::LocalTransform const& p_root_offset,
          std::vector<compages::world::EntityId>& p_nodes,
          std::vector<std::pair<compages::world::EntityId,
                                compages::renderer::PrefabSkinInstance const*>>&
              p_skins)
{
    compages::world::EntityId entity = p_world.create(p_node.name);
    if (p_node.source_index >= p_nodes.size())
    {
        p_nodes.resize(static_cast<std::size_t>(p_node.source_index) + 1u);
    }
    p_nodes[p_node.source_index] = entity;
    if (p_is_root)
    {
        p_root_out = entity;
        p_world.add(entity, PrefabInstance{ p_prefab });
    }

    compages::world::LocalTransform local = p_node.transform;
    if (p_is_root)
    {
        local.position = local.position + p_root_offset.position;
        local.rotation = p_root_offset.rotation * local.rotation;
        local.scale =
            compages::core::Vector3f(local.scale.x * p_root_offset.scale.x,
                                     local.scale.y * p_root_offset.scale.y,
                                     local.scale.z * p_root_offset.scale.z);
    }
    p_world.transform(entity) = local;
    if (!p_node.enabled)
    {
        p_world.setEnabled(entity, false);
    }

    if (p_node.mesh_renderer.has_value())
    {
        compages::renderer::PrefabMeshRenderer const& src =
            *p_node.mesh_renderer;
        const compages::renderer::MeshAssetId mesh =
            p_assets.findMesh(src.mesh);
        const compages::renderer::MaterialInstanceId material =
            p_assets.findMaterialInstance(src.material_instance);
        if (!mesh.valid())
        {
            return failure("prefab references unknown mesh '" + src.mesh + "'");
        }
        if (!material.valid())
        {
            return failure("prefab references unknown material instance '" +
                           src.material_instance + "'");
        }
        compages::renderer::MeshRenderer renderer;
        renderer.mesh = mesh;
        renderer.material_instance = material;
        renderer.flags = src.flags;
        p_world.add(entity, renderer);
    }

    if (p_node.camera.has_value())
    {
        p_world.add(entity, *p_node.camera);
    }
    if (p_node.directional_light.has_value())
    {
        p_world.add(entity, *p_node.directional_light);
    }
    if (p_node.skin_instance.has_value())
    {
        p_skins.emplace_back(entity, &*p_node.skin_instance);
    }

    if (p_parent.valid())
    {
        COMPAGES_TRY(p_world.setParent(entity, p_parent));
    }

    for (compages::renderer::PrefabNode const& child : p_node.children)
    {
        COMPAGES_TRY(spawnNode(p_world,
                               p_assets,
                               p_prefab,
                               child,
                               entity,
                               p_root_out,
                               false,
                               p_root_offset,
                               p_nodes,
                               p_skins));
    }
    return success();
}

} // namespace

Result<compages::world::EntityId>
instantiate(compages::world::World& p_world,
            compages::renderer::AssetManager& p_assets,
            compages::renderer::PrefabId p_prefab,
            compages::world::EntityId p_parent,
            compages::world::LocalTransform p_root_offset)
{
    compages::renderer::Prefab const* prefab = p_assets.prefab(p_prefab);
    if (prefab == nullptr)
    {
        return failure("instantiate called with a stale prefab id");
    }

    compages::world::EntityId root;
    std::vector<compages::world::EntityId> nodes;
    std::vector<std::pair<compages::world::EntityId,
                          compages::renderer::PrefabSkinInstance const*>>
        skins;
    COMPAGES_TRY(spawnNode(p_world,
                           p_assets,
                           p_prefab,
                           prefab->root,
                           p_parent,
                           root,
                           true,
                           p_root_offset,
                           nodes,
                           skins));
    for (auto const& [entity, source] : skins)
    {
        compages::world::SkinInstance instance;
        instance.joints.reserve(source->joints.size());
        for (std::uint32_t const joint : source->joints)
        {
            instance.joints.emplace_back((joint < nodes.size())
                                             ? nodes[joint]
                                             : compages::world::EntityId{});
        }
        p_world.add(entity, std::move(instance));
    }
    if (!prefab->animations.empty())
    {
        compages::renderer::Animator animator;
        animator.clip = prefab->animations.front();
        animator.clips = prefab->animations;
        animator.targets = std::move(nodes);
        p_world.add(root, std::move(animator));
    }
    p_world.update();
    return root;
}

} // namespace compages::renderer
