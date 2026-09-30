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

#include "Compages/Core/Result.hpp"
#include "Compages/World/EntityId.hpp"

#include <string>
#include <vector>

namespace compages::world
{
class World;
}

namespace compages::renderer
{

class AssetManager;
class Scene;

// ****************************************************************************
//! \brief Save and reload a compages::world::World subtree as JSON.
//!
//! Asset references inside components are stored by their registered name in
//! the AssetManager, not by runtime id, so a scene file survives restarts as
//! long as the same assets are registered before loading.
//!
//! \code
//! COMPAGES_TRY(compages::renderer::saveScene(scene, "level.json"));
//! auto roots = COMPAGES_TRY(compages::renderer::loadScene(scene,
//! "level.json"));
//! \endcode
// ****************************************************************************

// ------------------------------------------------------------------------
//! \brief Write every spatial root of \c p_world to a JSON file.
//! \param[in] p_world simulation to serialise.
//! \param[in] p_assets resolves mesh and material names.
//! \param[in] p_path output path.
// ------------------------------------------------------------------------
[[nodiscard]] Status save(compages::world::World const& p_world,
                          AssetManager const& p_assets,
                          std::string const& p_path);

// ------------------------------------------------------------------------
//! \brief Load entities from a JSON file into \c p_world.
//!
//! \return the entities that became new spatial roots (no parent in the file).
//! \param[in,out] p_world cleared entities are not removed; new ones are added.
//! \param[in] p_assets resolves names from the file.
//! \param[in] p_path JSON scene file.
// ------------------------------------------------------------------------
[[nodiscard]] Result<std::vector<compages::world::EntityId>>
load(compages::world::World& p_world,
     AssetManager const& p_assets,
     std::string const& p_path);

// ------------------------------------------------------------------------
//! \brief Save a Scene's compages::world::World plus presentation settings.
//! \param[in] p_scene world, environment and render settings.
//! \param[in] p_path output path.
// ------------------------------------------------------------------------
[[nodiscard]] Status saveScene(Scene const& p_scene, std::string const& p_path);

// ------------------------------------------------------------------------
//! \brief Reload a Scene's compages::world::World and presentation settings.
//! \param[in,out] p_scene receives loaded entities and settings.
//! \param[in] p_path JSON scene file.
//! \return new spatial root entities from the file.
// ------------------------------------------------------------------------
[[nodiscard]] Result<std::vector<compages::world::EntityId>>
loadScene(Scene& p_scene, std::string const& p_path);

} // namespace compages::renderer
