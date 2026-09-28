// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Ray.hpp"
#include "Compages/Renderer/Query/Raycast.hpp"

#include <cstdint>
#include <optional>

namespace compages::renderer
{

class Scene;
struct CameraFrame;

// ****************************************************************************
//! \brief Geometric pick against the MeshRenderers of a Scene.
//!
//! Resolves each MeshRenderer to its MeshAsset bounds, transforms them by
//! the last \c World::update() matrices, and returns the closest hit. This
//! is the visual raycast of Étape 4: it does not talk to a physics world.
//!
//! \param[in] p_scene World + AssetManager + (unused) camera. The ray is
//! already in world space; use \c CameraFrame::screenRay to build it.
// ****************************************************************************
[[nodiscard]] std::optional<RayHit> pick(Scene const& p_scene, Ray const& p_ray);

// ****************************************************************************
//! \brief Unproject a pixel through the camera frame and pick.
//!
//! Convenience for the common "click to select" path.
// ****************************************************************************
[[nodiscard]] std::optional<RayHit>
pickAt(Scene const& p_scene,
       CameraFrame const& p_camera,
       float p_x,
       float p_y,
       std::uint32_t p_width,
       std::uint32_t p_height);

} // namespace compages::renderer
