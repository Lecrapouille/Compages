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

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"
#include "Compages/Renderer/Environment.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/EntityId.hpp"

#include <cstdint>
#include <vector>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{



// ****************************************************************************
//! \brief One thing to draw during the frame, resolved down to an id and a
//! world matrix.
//!
//! What the compages::world::World's compages::renderer::MeshRenderer becomes after extraction: same ids, plus the
//! current world matrix and the current world bounds. Nothing here points
//! back into the compages::world::World.
// ****************************************************************************
struct RenderItem
{
    compages::world::EntityId entity;
    compages::renderer::MeshAssetId mesh;
    compages::renderer::MaterialInstanceId material_instance;
    compages::core::Matrix44f world_matrix{ compages::core::matrix::Identity };
    compages::core::AABB world_bounds;
    compages::renderer::RenderFlags flags = compages::renderer::RenderFlags::None;
    //! \brief Offset into \c RenderSnapshot::joint_palette. Zero and a
    //! \c joint_count of zero means a rigid mesh.
    std::uint32_t joint_offset = 0u;
    std::uint16_t joint_count = 0u;
};

// ****************************************************************************
//! \brief A directional light, in the form the renderer needs.
// ****************************************************************************
struct DirectionalLightFrame
{
    compages::core::Vector3f direction{ 0.0f, -1.0f, 0.0f };
    compages::core::Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
};

// ****************************************************************************
//! \brief A point light, in the form the renderer needs.
// ****************************************************************************
struct PointLightFrame
{
    compages::core::Vector3f position{ 0.0f, 0.0f, 0.0f };
    compages::core::Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float range = 100.0f;
};

// ****************************************************************************
//! \brief A frozen picture of everything the frame needs to draw.
//!
//! The Renderer consumes a snapshot. It does not read the compages::world::World. That is what
//! lets a future version put simulation on one thread and rendering on
//! another, and what makes the renderer testable in isolation.
// ****************************************************************************
struct RenderSnapshot
{
    CameraFrame camera{};
    std::vector<RenderItem> items;
    std::vector<DirectionalLightFrame> directional_lights;
    std::vector<PointLightFrame> point_lights;
    //! \brief Packed joint matrices for every skinned item, in the same
    //! space the vertex shader multiplies rest-pose vertices with.
    std::vector<compages::core::Matrix44f> joint_palette;
    compages::renderer::Environment environment{};
};

} // namespace compages::renderer
