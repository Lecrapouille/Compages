// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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

namespace compages::renderer
{

// ****************************************************************************
//! \brief One thing to draw during the frame, resolved down to an id and a
//! world matrix.
//!
//! What the World's MeshRenderer becomes after extraction: same ids, plus the
//! current world matrix and the current world bounds. Nothing here points
//! back into the World.
// ****************************************************************************
struct RenderItem
{
    compages::world::EntityId entity;
    compages::renderer::MeshAssetId mesh;
    compages::renderer::MaterialInstanceId material_instance;
    Matrix44f world_matrix{ compages::matrix::Identity };
    AABB world_bounds;
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
    Vector3f direction{ 0.0f, -1.0f, 0.0f };
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
};

// ****************************************************************************
//! \brief A point light, in the form the renderer needs.
// ****************************************************************************
struct PointLightFrame
{
    Vector3f position{ 0.0f, 0.0f, 0.0f };
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    float range = 100.0f;
};

// ****************************************************************************
//! \brief A frozen picture of everything the frame needs to draw.
//!
//! The Renderer consumes a snapshot. It does not read the World. That is what
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
    std::vector<Matrix44f> joint_palette;
    compages::renderer::Environment environment{};
};

} // namespace compages::renderer
