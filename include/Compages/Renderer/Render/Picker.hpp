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
