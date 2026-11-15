// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/World/Entity.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace compages::world
{

// ****************************************************************************
//! \file
//! \brief URDF robots in a World: read the file, then build the kinematic
//! chain. A URDF describes a robot, not only its look: the chain belongs to
//! the World; the Renderer adds the visuals (compages::renderer::loadUrdf).
// ****************************************************************************

//! \brief The shape of a visual of a link.
enum class UrdfGeometry
{
    Mesh,
    Box,
    Cylinder,
    Sphere,
};

//! \brief What a link looks like, as written in the file. Plain data: the
//! World keeps no mesh; the Renderer turns it into one.
struct UrdfVisual
{
    LocalTransform origin{};
    UrdfGeometry kind = UrdfGeometry::Box;
    //! \brief Mesh file, as written in the URDF (maybe "package://...").
    std::string filename;
    compages::Vector3f size{ 1.0f, 1.0f, 1.0f };
    float radius = 0.5f;
    float length = 1.0f;
    std::optional<compages::Vector4f> rgba;
};

// ****************************************************************************
//! \brief One link of a URDF robot: its name and what it looks like. It
//! becomes one entity of the World, named after it.
// ****************************************************************************
struct UrdfLink
{
    std::string name;
    std::vector<UrdfVisual> visuals;
};

enum class UrdfJointKind
{
    Fixed,
    Revolute,
    Continuous,
    Prismatic,
};

// ****************************************************************************
//! \brief One joint of a URDF robot, as written in the file: which link it
//! joins to which, where, round or along which axis, and within which limits
//! (in radians or metres). buildUrdf() turns a revolute or continuous joint
//! into a RevoluteJoint, a prismatic one into a PrismaticJoint, and a fixed
//! one into a plain local transform.
// ****************************************************************************
struct UrdfJoint
{
    std::string name;
    UrdfJointKind kind = UrdfJointKind::Fixed;
    std::string parent;
    std::string child;
    LocalTransform origin{};
    compages::Vector3f axis{ 1.0f, 0.0f, 0.0f };
    std::optional<double> lower;
    std::optional<double> upper;
    double velocity = 0.0;
};

// ****************************************************************************
//! \brief A URDF file, read but not built.
//!
//! Read it first with parseUrdf() to look at a robot without creating
//! anything (its joints and their limits, to fill a panel), then build it
//! with buildUrdf(), or do both with loadUrdf(). URDF is Z-up and Compages
//! Y-up: the built root turns one into the other.
//!
//! \code
//! COMPAGES_TRY_ASSIGN(robot, compages::world::parseUrdf("irb2400.urdf"));
//! for (compages::world::UrdfJoint const& j : robot.joints) { std::cout << j.name << '\n'; }
//! COMPAGES_TRY_ASSIGN(root, compages::world::buildUrdf(world, robot));
//! \endcode
// ****************************************************************************
struct UrdfRobot
{
    std::string name;
    std::vector<UrdfLink> links;
    std::vector<UrdfJoint> joints;
};

//! \brief Called for each link once its entity exists, to add what the World
//! does not know (the visuals).
using UrdfLinkHook = std::function<Status(UrdfLink const&, Entity)>;

// ----------------------------------------------------------------------------
//! \brief Read a URDF file. Nothing is created.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<UrdfRobot> parseUrdf(std::filesystem::path const& p_path);

// ----------------------------------------------------------------------------
//! \brief Build the kinematic chain of a robot: one entity per link, named
//! after it, hanging from the link of its parent joint, with a RevoluteJoint
//! or a PrismaticJoint component. Fixed joints are plain local transforms.
//!
//! URDF is Z-up and Compages is Y-up: the returned root, named after the
//! robot, turns one into the other. The root link hangs under it. On failure
//! nothing is left in the World.
//!
//! \param[in] p_on_link called for each link, or empty.
// ----------------------------------------------------------------------------
[[nodiscard]] Result<Entity> buildUrdf(World& p_world,
                                       UrdfRobot const& p_robot,
                                       EntityId p_parent = {},
                                       UrdfLinkHook const& p_on_link = {});

// ----------------------------------------------------------------------------
//! \brief Read and build a URDF robot, without its visuals: what a headless
//! simulation needs.
//!
//! \code
//! auto robot = compages::world::loadUrdf(world, "irb2400.urdf");
//! robot.value().lookup("base_link/link_1").joint().angle(30.0_deg);
//! \endcode
// ----------------------------------------------------------------------------
[[nodiscard]] Result<Entity> loadUrdf(World& p_world,
                                      std::filesystem::path const& p_path,
                                      EntityId p_parent = {});

} // namespace compages::world
