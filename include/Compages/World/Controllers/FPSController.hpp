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

#include "Compages/World/Controllers/CameraInput.hpp"
#include "Compages/World/EntityId.hpp"

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief A ground-locked camera: look freely, walk on the XZ plane.
//!
//! Pitch tilts the view but never the walk direction. \c up / \c down change
//! the eye height rather than flying, so the same WASD mapping as the fly
//! controller stays honest.
//!
//! \code
//! compages::world::FPSController fps;
//! fps.apply(world, cam, compages::world::cameraInput(input), dt);
//! \endcode
// ****************************************************************************
class FPSController
{
public:

    float yaw = 0.0f;
    float pitch = 0.0f;
    float eye_height = 1.7f;
    float look_sensitivity = 0.005f;
    float move_speed = 8.0f;
    float boost_multiplier = 2.0f;
    float min_pitch = -1.4f;
    float max_pitch = 1.4f;

    //! \brief Look with the mouse, walk on XZ, adjust height with Q/E.
    void apply(World& p_world,
               EntityId p_camera,
               CameraInput const& p_input,
               float p_dt);
};

} // namespace compages::world
