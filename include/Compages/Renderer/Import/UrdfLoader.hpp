// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/World/Import/UrdfLoader.hpp"

#include <filesystem>
#include <string>

namespace compages::renderer
{

class Scene;

// ****************************************************************************
//! \brief A URDF robot with the visuals of its links: STL meshes, glTF
//! models, boxes, cylinders and spheres, in the colour of their URDF material.
//! Mesh paths are relative to the URDF file; a "package://" prefix is dropped.
//! What Scene::loadFile() does for a ".urdf" file.
//!
//! The chain itself is built by compages::world::buildUrdf(); this adds the
//! visuals to each link. Without a picture, use compages::world::loadUrdf().
//!
//! \code
//! auto robot = compages::renderer::loadUrdf(scene, "irb2400.urdf");
//! \endcode
// ****************************************************************************
[[nodiscard]] Result<compages::world::Entity>
loadUrdf(Scene& p_scene,
         std::filesystem::path const& p_path,
         compages::world::EntityId p_parent = {});

} // namespace compages::renderer
