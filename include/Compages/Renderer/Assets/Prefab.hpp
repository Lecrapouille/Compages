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

#include "Compages/Renderer/Assets/AnimationClip.hpp"
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
// ****************************************************************************
struct PrefabMeshRenderer
{
    std::string mesh;
    std::string material_instance;
    compages::renderer::RenderFlags flags = compages::renderer::RenderFlags::None;
};

struct PrefabSkinInstance
{
    std::vector<std::uint32_t> joints;
};

// ****************************************************************************
//! \brief One node of a prefab tree: transform, optional components, children.
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
// ****************************************************************************
struct Prefab
{
    std::string name;
    PrefabNode root;
    std::vector<AnimationClipId> animations;
};

} // namespace compages::renderer
