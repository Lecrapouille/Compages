// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"

#include <cstdint>

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{

enum class PrimitiveShape
{
    Cube,
    Sphere,
    Plane,
};

// ****************************************************************************
//! \file
//! \brief Helpers that build the meshes and materials the examples need,
//! without a loader.
//!
//! These are what a Three.js user would call \c BoxGeometry, \c SphereGeometry
//! and \c MeshLambertMaterial. They live at the assets level, not in the
//! compages::world::World, because a mesh is data on the device and does not
//! know which entity draws it.
// ****************************************************************************

[[nodiscard]] Result<MeshAsset> makeCube();

//! \brief Build one of the common primitive shapes through one generic entry.
[[nodiscard]] Result<MeshAsset> makePrimitive(PrimitiveShape p_shape);

[[nodiscard]] Result<MeshAsset>
makeBox(float p_width, float p_height, float p_depth);

[[nodiscard]] Result<MeshAsset> makeSphere(float p_radius = 0.5f,
                                           std::uint32_t p_stacks = 16u,
                                           std::uint32_t p_slices = 24u);

[[nodiscard]] Result<MeshAsset> makePlane(float p_width = 1.0f,
                                          float p_height = 1.0f,
                                          std::uint32_t p_x_segments = 1u,
                                          std::uint32_t p_y_segments = 1u);

//! \brief Legacy Tube: frustum between two radii, centred on the origin.
[[nodiscard]] Result<MeshAsset> makeTube(float p_top_radius,
                                         float p_bottom_radius,
                                         float p_height,
                                         std::uint32_t p_slices = 16u,
                                         bool p_tip_along_negative_z = false);

//! \brief Cone with tip on \c -Z so \c compages::world::lookAt() aims it at the
//! target.
[[nodiscard]] Result<MeshAsset> makeCone(float p_bottom_radius,
                                         float p_top_radius,
                                         float p_height,
                                         std::uint32_t p_slices = 16u);

[[nodiscard]] Result<MeshAsset>
makeCylinder(float p_radius, float p_height, std::uint32_t p_slices = 16u);

[[nodiscard]] Result<MeshAsset> makePyramid(float p_radius, float p_height);

[[nodiscard]] Result<Material> makeLitMaterial();

[[nodiscard]] Result<Material> makePbrMaterial();

//! \brief Legacy DepthMaterial: eye-space depth as greyscale.
[[nodiscard]] Result<Material> makeDepthMaterial();

//! \brief Legacy NormalsMaterial: encoded surface normal as RGB.
[[nodiscard]] Result<Material> makeNormalsMaterial();

} // namespace compages::renderer
