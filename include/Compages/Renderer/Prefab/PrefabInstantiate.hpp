// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"
#include "Compages/World/EntityId.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

namespace compages::world
{
class World;
}

namespace compages::renderer
{

class AssetManager;

// ****************************************************************************
//! \brief Spawn a prefab hierarchy into a World.
//!
//! \param[in,out] p_world where entities are created.
//! \param[in] p_assets used to resolve mesh and material names.
//! \param[in] p_prefab which template to spawn.
//! \param[in] p_parent optional parent entity. Empty means a new root.
//! \param[in] p_root_offset applied to the prefab root transform after copy.
//! \return the root entity of the instance, tagged with PrefabInstance.
//!
//! \code
//! auto root = COMPAGES_TRY(compages::renderer::instantiate(world, assets, robotPrefab));
//! world.entity(root).position(0, 0, 3);
//! \endcode
// ****************************************************************************
[[nodiscard]] compages::Result<compages::world::EntityId>
instantiate(compages::world::World& p_world,
            AssetManager& p_assets,
            PrefabId p_prefab,
            compages::world::EntityId p_parent = {},
            compages::world::LocalTransform p_root_offset = {});

} // namespace compages::renderer
