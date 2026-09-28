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
