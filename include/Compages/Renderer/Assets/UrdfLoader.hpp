// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/World/Entity.hpp"

#include <string>

namespace compages::renderer
{

using compages::world::Entity;
using compages::world::EntityId;
using compages::world::World;

class Scene;

// ****************************************************************************
//! \brief Build the kinematic chain of a URDF robot in a World: one entity
//! per link, named after it, hanging from the link of its parent joint, with
//! a RevoluteJoint or a PrismaticJoint component. Fixed joints are plain
//! local transforms. Visuals are ignored: what a headless simulation needs.
//!
//! URDF is Z-up and Compages is Y-up: the returned root, named after the
//! robot, turns one into the other. The root link hangs under it.
//!
//! \code
//! auto robot = compages::renderer::loadUrdf(world, "irb2400.urdf");
//! robot.value().lookup("base_link/link_1").angle(30.0_deg);
//! \endcode
// ****************************************************************************
[[nodiscard]] compages::Result<Entity>
loadUrdf(World& p_world, std::string const& p_path, EntityId p_parent = {});

// ****************************************************************************
//! \brief Same, with the visuals of the links: STL meshes, glTF models,
//! boxes, cylinders and spheres, in the colour of their URDF material. Mesh
//! paths are relative to the URDF file; a "package://" prefix is dropped.
//! What Scene::load() does for a ".urdf" file.
// ****************************************************************************
[[nodiscard]] compages::Result<Entity>
loadUrdf(Scene& p_scene, std::string const& p_path, EntityId p_parent = {});

} // namespace compages::renderer
