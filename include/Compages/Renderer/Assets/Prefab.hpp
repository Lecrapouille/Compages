// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Animation/AnimationClip.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief A compages::renderer::MeshRenderer inside a prefab, referencing assets by registered
//! name rather than by runtime id.
//!
//! Names survive a save and a load, and a prefab may be copied into another
//! World; instantiate() turns them back into ids.
// ****************************************************************************
struct PrefabMeshRenderer
{
    std::string mesh;
    std::string material_instance;
    compages::renderer::RenderFlags flags = compages::renderer::RenderFlags::None;
};

// ****************************************************************************
//! \brief The skin of a node inside a prefab: the source indices of its joint
//! nodes, turned into entity ids when the prefab is instantiated.
// ****************************************************************************
struct PrefabSkinInstance
{
    std::vector<std::uint32_t> joints;
};

// ****************************************************************************
//! \brief One node of a prefab tree: transform, optional components, children.
//!
//! It becomes one entity per copy. The source index keeps the order of the
//! file, so that skins and animation channels find their node whatever the
//! names.
// ****************************************************************************
struct PrefabNode
{
    //! \brief Stable index used by skins and animation channels.
    std::uint32_t source_index = 0u;
    std::string name;
    compages::world::LocalTransform transform{};
    bool enabled = true;
    std::optional<PrefabMeshRenderer> mesh_renderer;
    std::optional<PrefabSkinInstance> skin_instance;
    std::optional<compages::world::Camera> camera;
    std::optional<compages::world::DirectionalLight> directional_light;
    std::vector<PrefabNode> children;
};

// ****************************************************************************
//! \brief A reusable entity hierarchy template owned by the AssetManager.
//!
//! Asset references inside the tree are strings (``"cube"``, ``"wood"``) so a
//! prefab survives save/load and can be shared across Worlds. Runtime ids are
//! resolved only at instantiation time.
//!
//! Load a file once as a prefab, then make as many copies as needed: each
//! copy is a new tree of entities that can move alone, and they all share the
//! meshes and materials of the catalog.
//!
//! \code
//! COMPAGES_TRY_ASSIGN(barrel, scene.assets().loadFile("barrel.glb"));
//! for (int k = 0; k < 10; ++k)
//! {
//!     COMPAGES_TRY_ASSIGN(copy, scene.instantiate(barrel));
//!     copy.position(float(k), 0.0f, 0.0f);
//! }
//! \endcode
// ****************************************************************************
struct Prefab
{
    std::string name;
    PrefabNode root;
    std::vector<AnimationClipId> animations;
};

} // namespace compages::renderer
