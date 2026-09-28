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

#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Core/Result.hpp"

#include <cstdint>

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
//! and \c MeshLambertMaterial. They live at the assets level, not in the World,
//! because a mesh is data on the device and does not know which entity draws
//! it.
// ****************************************************************************

[[nodiscard]] compages::Result<MeshAsset> makeCube();

//! \brief Build one of the common primitive shapes through one generic entry.
[[nodiscard]] compages::Result<MeshAsset>
makePrimitive(PrimitiveShape p_shape);

[[nodiscard]] compages::Result<MeshAsset> makeBox(float p_width,
                                             float p_height,
                                             float p_depth);

[[nodiscard]] compages::Result<MeshAsset> makeSphere(float p_radius = 0.5f,
                                                std::uint32_t p_stacks = 16u,
                                                std::uint32_t p_slices = 24u);

[[nodiscard]] compages::Result<MeshAsset> makePlane(float p_width = 1.0f,
                                               float p_height = 1.0f,
                                               std::uint32_t p_x_segments = 1u,
                                               std::uint32_t p_y_segments = 1u);

//! \brief Legacy Tube: frustum between two radii, centred on the origin.
[[nodiscard]] compages::Result<MeshAsset> makeTube(float p_top_radius,
                                              float p_bottom_radius,
                                              float p_height,
                                              std::uint32_t p_slices = 16u,
                                              bool p_tip_along_negative_z = false);

//! \brief Cone with tip on \c -Z so \c compages::world::lookAt() aims it at the target.
[[nodiscard]] compages::Result<MeshAsset> makeCone(float p_bottom_radius,
                                              float p_top_radius,
                                              float p_height,
                                              std::uint32_t p_slices = 16u);

[[nodiscard]] compages::Result<MeshAsset> makeCylinder(float p_radius,
                                                    float p_height,
                                                    std::uint32_t p_slices = 16u);

[[nodiscard]] compages::Result<MeshAsset> makePyramid(float p_radius,
                                                   float p_height);

[[nodiscard]] compages::Result<Material> makeLitMaterial();

[[nodiscard]] compages::Result<Material> makePbrMaterial();

//! \brief Legacy DepthMaterial: eye-space depth as greyscale.
[[nodiscard]] compages::Result<Material> makeDepthMaterial();

//! \brief Legacy NormalsMaterial: encoded surface normal as RGB.
[[nodiscard]] compages::Result<Material> makeNormalsMaterial();

} // namespace compages::renderer
