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

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/GltfLoader.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"

#include <cstdint>
#include <string>

namespace compages::renderer
{

namespace
{
constexpr std::uint32_t MAX_ASSETS = 0xFFFFu;
} // namespace

//------------------------------------------------------------------------------
template <typename Id, typename T>
Result<Id> AssetManager::insert(Pool<T>& p_pool,
                                std::string p_name,
                                T p_value,
                                std::size_t& p_living,
                                const char* p_kind)
{
    // Replace when a name is already used, so hot-reloading a mesh is one
    // call and does not leak an id.
    if (!p_name.empty())
    {
        auto it = p_pool.by_name.find(p_name);
        if (it != p_pool.by_name.end())
        {
            const std::uint16_t index = static_cast<std::uint16_t>(it->second);
            p_pool.data[index] = std::move(p_value);
            return Id(index, p_pool.generation[index]);
        }
    }

    std::uint16_t index = 0u;
    if (!p_pool.free_list.empty())
    {
        index = p_pool.free_list.back();
        p_pool.free_list.pop_back();
        p_pool.free_slot[index] = 0u;
        p_pool.data[index] = std::move(p_value);
    }
    else
    {
        if (p_pool.data.size() >= MAX_ASSETS)
        {
            return failure(std::string("the ") + p_kind + " pool is full");
        }
        index = static_cast<std::uint16_t>(p_pool.data.size());
        p_pool.data.emplace_back(std::move(p_value));
        p_pool.generation.emplace_back(1u);
        p_pool.free_slot.emplace_back(0u);
        p_pool.names.emplace_back();
    }

    p_pool.names[index] = p_name;
    if (!p_name.empty())
    {
        p_pool.by_name[p_name] = index;
    }
    ++p_living;
    return Id(index, p_pool.generation[index]);
}

//------------------------------------------------------------------------------
template <typename Id, typename T>
void AssetManager::erase(Pool<T>& p_pool, Id p_id, std::size_t& p_living)
{
    if (!p_id.valid())
    {
        return;
    }
    const std::size_t index = p_id.index();
    if (index >= p_pool.data.size())
    {
        return;
    }
    if (p_pool.free_slot[index] != 0u)
    {
        return;
    }
    if (p_pool.generation[index] != p_id.generation())
    {
        return;
    }

    if (!p_pool.names[index].empty())
    {
        p_pool.by_name.erase(p_pool.names[index]);
    }
    p_pool.names[index].clear();
    p_pool.data[index] = T();
    p_pool.free_slot[index] = 1u;
    std::uint16_t next =
        static_cast<std::uint16_t>(p_pool.generation[index] + 1u);
    if (next == 0u)
    {
        next = 1u;
    }
    p_pool.generation[index] = next;
    p_pool.free_list.emplace_back(static_cast<std::uint16_t>(index));
    --p_living;
}

//------------------------------------------------------------------------------
template <typename Id, typename T>
T const* AssetManager::lookup(Pool<T> const& p_pool, Id p_id) const
{
    if (!p_id.valid())
    {
        return nullptr;
    }
    const std::size_t index = p_id.index();
    if (index >= p_pool.data.size())
    {
        return nullptr;
    }
    if (p_pool.free_slot[index] != 0u)
    {
        return nullptr;
    }
    if (p_pool.generation[index] != p_id.generation())
    {
        return nullptr;
    }
    return &p_pool.data[index];
}

//------------------------------------------------------------------------------
template <typename Id, typename T>
T* AssetManager::lookupMutable(Pool<T>& p_pool, Id p_id)
{
    return const_cast<T*>(
        lookup<Id, T>(const_cast<Pool<T> const&>(p_pool), p_id));
}

//------------------------------------------------------------------------------
template <typename Id, typename T>
Id AssetManager::findByName(Pool<T> const& p_pool,
                            std::string_view p_name) const
{
    const auto it = p_pool.by_name.find(std::string(p_name));
    if (it == p_pool.by_name.end())
    {
        return {};
    }
    const std::uint16_t index = static_cast<std::uint16_t>(it->second);
    return Id(index, p_pool.generation[index]);
}

//------------------------------------------------------------------------------
template <typename Id, typename T>
std::string AssetManager::nameOf(Pool<T> const& p_pool, Id p_id) const
{
    if (!p_id.valid())
    {
        return {};
    }
    const std::size_t index = p_id.index();
    if (index >= p_pool.names.size())
    {
        return {};
    }
    if (p_pool.free_slot[index] != 0u)
    {
        return {};
    }
    if (p_pool.generation[index] != p_id.generation())
    {
        return {};
    }
    return p_pool.names[index];
}

//------------------------------------------------------------------------------
Result<MeshAssetId> AssetManager::addMesh(std::string p_name, MeshAsset p_asset)
{
    return insert<MeshAssetId>(
        m_meshes, std::move(p_name), std::move(p_asset), m_mesh_living, "mesh");
}

//------------------------------------------------------------------------------
Result<MaterialId> AssetManager::addMaterial(std::string p_name,
                                             Material p_material)
{
    return insert<MaterialId>(m_materials,
                              std::move(p_name),
                              std::move(p_material),
                              m_material_living,
                              "material");
}

//------------------------------------------------------------------------------
Result<MaterialInstanceId>
AssetManager::addMaterialInstance(std::string p_name,
                                  MaterialInstance p_instance)
{
    return insert<MaterialInstanceId>(m_instances,
                                      std::move(p_name),
                                      std::move(p_instance),
                                      m_instance_living,
                                      "material instance");
}

//------------------------------------------------------------------------------
MeshAsset const* AssetManager::mesh(MeshAssetId p_id) const
{
    return lookup<MeshAssetId, MeshAsset>(m_meshes, p_id);
}

//------------------------------------------------------------------------------
MeshAsset* AssetManager::mesh(MeshAssetId p_id)
{
    return lookupMutable<MeshAssetId, MeshAsset>(m_meshes, p_id);
}

//------------------------------------------------------------------------------
Material const* AssetManager::material(MaterialId p_id) const
{
    return lookup<MaterialId, Material>(m_materials, p_id);
}

//------------------------------------------------------------------------------
Material* AssetManager::material(MaterialId p_id)
{
    return lookupMutable<MaterialId, Material>(m_materials, p_id);
}

//------------------------------------------------------------------------------
MaterialInstance const*
AssetManager::materialInstance(MaterialInstanceId p_id) const
{
    return lookup<MaterialInstanceId, MaterialInstance>(m_instances, p_id);
}

//------------------------------------------------------------------------------
MaterialInstance* AssetManager::materialInstance(MaterialInstanceId p_id)
{
    return lookupMutable<MaterialInstanceId, MaterialInstance>(m_instances,
                                                               p_id);
}

//------------------------------------------------------------------------------
MeshAssetId AssetManager::findMesh(std::string_view p_name) const
{
    return findByName<MeshAssetId>(m_meshes, p_name);
}

//------------------------------------------------------------------------------
MaterialId AssetManager::findMaterial(std::string_view p_name) const
{
    return findByName<MaterialId>(m_materials, p_name);
}

//------------------------------------------------------------------------------
MaterialInstanceId
AssetManager::findMaterialInstance(std::string_view p_name) const
{
    return findByName<MaterialInstanceId>(m_instances, p_name);
}

//------------------------------------------------------------------------------
void AssetManager::removeMesh(MeshAssetId p_id)
{
    erase(m_meshes, p_id, m_mesh_living);
}

//------------------------------------------------------------------------------
void AssetManager::removeMaterial(MaterialId p_id)
{
    erase(m_materials, p_id, m_material_living);
}

//------------------------------------------------------------------------------
void AssetManager::removeMaterialInstance(MaterialInstanceId p_id)
{
    erase(m_instances, p_id, m_instance_living);
}

//------------------------------------------------------------------------------
Result<TextureAssetId> AssetManager::addTexture(std::string p_name,
                                                TextureAsset p_texture)
{
    return insert<TextureAssetId>(m_textures,
                                  std::move(p_name),
                                  std::move(p_texture),
                                  m_texture_living,
                                  "texture");
}

//------------------------------------------------------------------------------
TextureAsset const* AssetManager::texture(TextureAssetId p_id) const
{
    return lookup<TextureAssetId, TextureAsset>(m_textures, p_id);
}

//------------------------------------------------------------------------------
TextureAssetId AssetManager::findTexture(std::string_view p_name) const
{
    return findByName<TextureAssetId>(m_textures, p_name);
}

//------------------------------------------------------------------------------
void AssetManager::removeTexture(TextureAssetId p_id)
{
    erase(m_textures, p_id, m_texture_living);
}

//------------------------------------------------------------------------------
Result<PrefabId> AssetManager::addPrefab(std::string p_name, Prefab p_prefab)
{
    return insert<PrefabId>(m_prefabs,
                            std::move(p_name),
                            std::move(p_prefab),
                            m_prefab_living,
                            "prefab");
}

//------------------------------------------------------------------------------
Result<PrefabId> AssetManager::load(std::string const& p_path)
{
    return loadGltf(p_path, *this);
}

//------------------------------------------------------------------------------
Prefab const* AssetManager::prefab(PrefabId p_id) const
{
    return lookup<PrefabId, Prefab>(m_prefabs, p_id);
}

//------------------------------------------------------------------------------
PrefabId AssetManager::findPrefab(std::string_view p_name) const
{
    return findByName<PrefabId>(m_prefabs, p_name);
}

//------------------------------------------------------------------------------
std::string AssetManager::meshName(MeshAssetId p_id) const
{
    return nameOf<MeshAssetId>(m_meshes, p_id);
}

//------------------------------------------------------------------------------
std::string AssetManager::materialInstanceName(MaterialInstanceId p_id) const
{
    return nameOf<MaterialInstanceId>(m_instances, p_id);
}

//------------------------------------------------------------------------------
std::string AssetManager::textureName(TextureAssetId p_id) const
{
    return nameOf<TextureAssetId>(m_textures, p_id);
}

//------------------------------------------------------------------------------
std::string AssetManager::prefabName(PrefabId p_id) const
{
    return nameOf<PrefabId>(m_prefabs, p_id);
}

//------------------------------------------------------------------------------
void AssetManager::removePrefab(PrefabId p_id)
{
    erase(m_prefabs, p_id, m_prefab_living);
}

//------------------------------------------------------------------------------
Result<AnimationClipId> AssetManager::addAnimation(std::string p_name,
                                                   AnimationClip p_clip)
{
    return insert<AnimationClipId>(m_animations,
                                   std::move(p_name),
                                   std::move(p_clip),
                                   m_animation_living,
                                   "animation");
}

//------------------------------------------------------------------------------
AnimationClip const* AssetManager::animation(AnimationClipId p_id) const
{
    return lookup<AnimationClipId, AnimationClip>(m_animations, p_id);
}

//------------------------------------------------------------------------------
AnimationClipId AssetManager::findAnimation(std::string_view p_name) const
{
    return findByName<AnimationClipId>(m_animations, p_name);
}

//------------------------------------------------------------------------------
void AssetManager::removeAnimation(AnimationClipId p_id)
{
    erase(m_animations, p_id, m_animation_living);
}

//------------------------------------------------------------------------------
Result<SkinAssetId> AssetManager::addSkin(std::string p_name, SkinAsset p_skin)
{
    return insert<SkinAssetId>(
        m_skins, std::move(p_name), std::move(p_skin), m_skin_living, "skin");
}

//------------------------------------------------------------------------------
SkinAsset const* AssetManager::skin(SkinAssetId p_id) const
{
    return lookup<SkinAssetId, SkinAsset>(m_skins, p_id);
}

//------------------------------------------------------------------------------
SkinAssetId AssetManager::findSkin(std::string_view p_name) const
{
    return findByName<SkinAssetId>(m_skins, p_name);
}

//------------------------------------------------------------------------------
void AssetManager::removeSkin(SkinAssetId p_id)
{
    erase(m_skins, p_id, m_skin_living);
}

//------------------------------------------------------------------------------
Status AssetManager::prepare(MeshAssetId p_id)
{
    MeshAsset* value = mesh(p_id);
    if (value == nullptr)
    {
        return failure("cannot prepare a stale mesh asset");
    }
    return value->upload();
}

//------------------------------------------------------------------------------
Status AssetManager::prepare(TextureAssetId p_id)
{
    TextureAsset* value =
        lookupMutable<TextureAssetId, TextureAsset>(m_textures, p_id);
    if (value == nullptr)
    {
        return failure("cannot prepare a stale texture asset");
    }
    return value->upload();
}

//------------------------------------------------------------------------------
Status AssetManager::prepare(MaterialId p_id)
{
    Material* value = material(p_id);
    if (value == nullptr)
    {
        return failure("cannot prepare a stale material asset");
    }
    if (value->program.valid() && value->pipeline.valid())
    {
        return success();
    }

    Result<Material> prepared = failure("unknown material family");
    switch (value->family)
    {
        case ShaderFamily::Lit:
            prepared = makeLitMaterial();
            break;
        case ShaderFamily::PbrMinimal:
            prepared = makePbrMaterial();
            break;
        case ShaderFamily::Depth:
            prepared = makeDepthMaterial();
            break;
        case ShaderFamily::Normals:
            prepared = makeNormalsMaterial();
            break;
    }
    if (!prepared)
    {
        return failure(prepared.error());
    }
    std::string const name = value->name;
    *value = prepared.take();
    if (!name.empty())
    {
        value->name = name;
    }
    return success();
}

} // namespace compages::renderer
