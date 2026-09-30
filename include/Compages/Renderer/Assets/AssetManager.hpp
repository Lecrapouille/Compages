// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/AnimationClip.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Renderer/Assets/Prefab.hpp"
#include "Compages/Renderer/Assets/Skin.hpp"
#include "Compages/Renderer/Assets/TextureAsset.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Owns and hands out mesh assets, materials and material instances.
//!
//! Assets are addressed by a typed id: a MeshAssetId names a mesh, a
//! MaterialId names a material family, a MaterialInstanceId names one
//! parameterisation. The AssetManager owns the actual data: the GPU buffers
//! of a mesh, the program and pipeline of a material, the parameters of an
//! instance. compages::world::World components store only ids.
//!
//! The manager is not a scene: it does not know which entities use a given
//! asset. It is what would eventually cache the results of loading a glTF file
//! and hand out its ids. For now it is a small holder of pools, with just what
//! Étape 3 of the roadmap needs: registering a mesh made in code, registering
//! a material made in code, and dropping any of them by id.
// ****************************************************************************
class AssetManager
{
public:

    AssetManager() = default;
    AssetManager(AssetManager const&) = delete;
    AssetManager& operator=(AssetManager const&) = delete;
    AssetManager(AssetManager&&) = default;
    AssetManager& operator=(AssetManager&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Register a mesh asset built by the caller and return its id.
    //!
    //! \param[in] p_name what to remember it by. May be empty; a name that has
    //! already been registered replaces the previous asset and reuses its
    //! generation slot.
    //! \param[in] p_asset the mesh, moved from.
    //! \return a stable id or a failure if the pool is full.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<MeshAssetId> addMesh(std::string p_name,
                                              MeshAsset p_asset);

    // ------------------------------------------------------------------------
    //! \brief Register a material and return its id.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<MaterialId> addMaterial(std::string p_name,
                                                 Material p_material);

    // ------------------------------------------------------------------------
    //! \brief Register a material instance and return its id.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<MaterialInstanceId>
    addMaterialInstance(std::string p_name, MaterialInstance p_instance);

    // ------------------------------------------------------------------------
    //! \brief Register a texture and return its id.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<TextureAssetId> addTexture(std::string p_name,
                                                    TextureAsset p_texture);

    // ------------------------------------------------------------------------
    //! \brief Register a prefab template and return its id.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<PrefabId> addPrefab(std::string p_name,
                                             Prefab p_prefab);

    //! \brief Load a reusable asset without creating entities.
    [[nodiscard]] Result<PrefabId> load(std::string const& p_path);

    [[nodiscard]] Result<AnimationClipId> addAnimation(std::string p_name,
                                                       AnimationClip p_clip);

    [[nodiscard]] Result<SkinAssetId> addSkin(std::string p_name,
                                              SkinAsset p_skin);

    // ------------------------------------------------------------------------
    //! \brief Look up a mesh, or nullptr when the id is empty or stale.
    // ------------------------------------------------------------------------
    [[nodiscard]] MeshAsset const* mesh(MeshAssetId p_id) const;
    [[nodiscard]] MeshAsset* mesh(MeshAssetId p_id);

    // ------------------------------------------------------------------------
    //! \brief Look up a material.
    // ------------------------------------------------------------------------
    [[nodiscard]] Material const* material(MaterialId p_id) const;
    [[nodiscard]] Material* material(MaterialId p_id);

    // ------------------------------------------------------------------------
    //! \brief Look up a material instance.
    // ------------------------------------------------------------------------
    [[nodiscard]] MaterialInstance const*
    materialInstance(MaterialInstanceId p_id) const;
    [[nodiscard]] MaterialInstance* materialInstance(MaterialInstanceId p_id);

    // ------------------------------------------------------------------------
    //! \brief Look up a texture.
    // ------------------------------------------------------------------------
    [[nodiscard]] TextureAsset const* texture(TextureAssetId p_id) const;

    // ------------------------------------------------------------------------
    //! \brief Look up a prefab.
    // ------------------------------------------------------------------------
    [[nodiscard]] Prefab const* prefab(PrefabId p_id) const;

    [[nodiscard]] AnimationClip const* animation(AnimationClipId p_id) const;
    [[nodiscard]] SkinAsset const* skin(SkinAssetId p_id) const;

    //! \brief Materialize deferred CPU assets on the current GPU device.
    [[nodiscard]] Status prepare(MeshAssetId p_id);
    [[nodiscard]] Status prepare(TextureAssetId p_id);
    [[nodiscard]] Status prepare(MaterialId p_id);

    // ------------------------------------------------------------------------
    //! \brief Look up a mesh by its registered name.
    // ------------------------------------------------------------------------
    [[nodiscard]] MeshAssetId findMesh(std::string_view p_name) const;
    [[nodiscard]] MaterialId findMaterial(std::string_view p_name) const;
    [[nodiscard]] MaterialInstanceId
    findMaterialInstance(std::string_view p_name) const;
    [[nodiscard]] TextureAssetId findTexture(std::string_view p_name) const;
    [[nodiscard]] PrefabId findPrefab(std::string_view p_name) const;
    [[nodiscard]] AnimationClipId findAnimation(std::string_view p_name) const;
    [[nodiscard]] SkinAssetId findSkin(std::string_view p_name) const;

    // ------------------------------------------------------------------------
    //! \brief Reverse lookup: registered name for a live asset id, or empty.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string meshName(MeshAssetId p_id) const;
    [[nodiscard]] std::string
    materialInstanceName(MaterialInstanceId p_id) const;
    [[nodiscard]] std::string textureName(TextureAssetId p_id) const;
    [[nodiscard]] std::string prefabName(PrefabId p_id) const;

    // ------------------------------------------------------------------------
    //! \brief Drop an asset. Idempotent: a stale id is a no-op.
    // ------------------------------------------------------------------------
    void removeMesh(MeshAssetId p_id);
    void removeMaterial(MaterialId p_id);
    void removeMaterialInstance(MaterialInstanceId p_id);
    void removeTexture(TextureAssetId p_id);
    void removePrefab(PrefabId p_id);
    void removeAnimation(AnimationClipId p_id);
    void removeSkin(SkinAssetId p_id);

    [[nodiscard]] std::size_t meshCount() const
    {
        return m_mesh_living;
    }
    [[nodiscard]] std::size_t materialCount() const
    {
        return m_material_living;
    }
    [[nodiscard]] std::size_t materialInstanceCount() const
    {
        return m_instance_living;
    }
    [[nodiscard]] std::size_t textureCount() const
    {
        return m_texture_living;
    }
    [[nodiscard]] std::size_t prefabCount() const
    {
        return m_prefab_living;
    }
    [[nodiscard]] std::size_t animationCount() const
    {
        return m_animation_living;
    }
    [[nodiscard]] std::size_t skinCount() const
    {
        return m_skin_living;
    }

private:

    template <typename T>
    struct Pool
    {
        std::vector<T> data;
        std::vector<std::uint16_t> generation;
        std::vector<std::uint8_t> free_slot;
        std::vector<std::uint16_t> free_list;
        std::unordered_map<std::string, std::uint32_t> by_name;
        std::vector<std::string> names;
    };

    template <typename Id, typename T>
    Result<Id> insert(Pool<T>& p_pool,
                      std::string p_name,
                      T p_value,
                      std::size_t& p_living,
                      const char* p_kind);

    template <typename Id, typename T>
    void erase(Pool<T>& p_pool, Id p_id, std::size_t& p_living);

    template <typename Id, typename T>
    T const* lookup(Pool<T> const& p_pool, Id p_id) const;

    template <typename Id, typename T>
    T* lookupMutable(Pool<T>& p_pool, Id p_id);

    template <typename Id, typename T>
    Id findByName(Pool<T> const& p_pool, std::string_view p_name) const;

    template <typename Id, typename T>
    std::string nameOf(Pool<T> const& p_pool, Id p_id) const;

    Pool<MeshAsset> m_meshes;
    Pool<Material> m_materials;
    Pool<MaterialInstance> m_instances;
    Pool<TextureAsset> m_textures;
    Pool<Prefab> m_prefabs;
    Pool<AnimationClip> m_animations;
    Pool<SkinAsset> m_skins;
    std::size_t m_mesh_living = 0u;
    std::size_t m_material_living = 0u;
    std::size_t m_instance_living = 0u;
    std::size_t m_texture_living = 0u;
    std::size_t m_prefab_living = 0u;
    std::size_t m_animation_living = 0u;
    std::size_t m_skin_living = 0u;
};

} // namespace compages::renderer
