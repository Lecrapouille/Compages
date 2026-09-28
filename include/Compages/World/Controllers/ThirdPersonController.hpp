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

#include "Compages/Core/Vector.hpp"
#include "Compages/World/Controllers/CameraInput.hpp"
#include "Compages/World/EntityId.hpp"

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief An orbit camera whose target is a living EntityId.
//!
//! Same yaw/pitch/distance as \c OrbitController, but each \c apply() reads
//! the target's world position (plus \c look_offset) so the camera follows
//! a character, a vehicle, a picked mesh. Not a component: the application
//! owns it and runs it before \c World::update().
//!
//! \code
//! compages::world::ThirdPersonController follow;
//! follow.target = hero.id();
//! follow.apply(world, cam, compages::world::cameraInput(input));
//! \endcode
// ****************************************************************************
class ThirdPersonController
{
public:

    EntityId target{};
    Vector3f look_offset{ 0.0f, 1.6f, 0.0f };
    float yaw = 0.0f;
    float pitch = -0.25f;
    float distance = 8.0f;

    float look_sensitivity = 0.005f;
    float zoom_sensitivity = 1.5f;
    float min_distance = 1.5f;
    float max_distance = 40.0f;
    float min_pitch = -1.2f;
    float max_pitch = 0.6f;

    //! \brief Orbit \c target using mouse look and wheel zoom.
    void apply(World& p_world,
               EntityId p_camera,
               CameraInput const& p_input);

    //! \brief Place the camera from yaw/pitch/distance without reading input.
    void writePose(World& p_world, EntityId p_camera) const;
};

} // namespace compages::world
