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
//! \brief A free-flying camera: look with the mouse, slide along its own
//! axes with the move keys.
//!
//! Six degrees of freedom. Q/E (mapped to \c up / \c down) climb in camera
//! space, not world up, which is what a spectator camera wants and what an
//! FPS camera must not do.
//!
//! \code
//! compages::world::FlyController fly;
//! fly.apply(world, cameraId, compages::world::cameraInput(frame.input), frame.elapsed);
//! \endcode
// ****************************************************************************
class FlyController
{
public:

    float yaw = 0.0f;
    float pitch = 0.0f;
    float look_sensitivity = 0.005f;
    float move_speed = 20.0f;
    float boost_multiplier = 3.0f;
    float min_pitch = -1.55f;
    float max_pitch = 1.55f;

    //! \brief Update yaw/pitch from look input and move along camera axes.
    //! \param[in,out] p_world world whose transform store is written.
    //! \param[in] p_camera entity with a \c Camera (usually).
    //! \param[in] p_input mapped mouse and keys.
    //! \param[in] p_dt seconds since the last frame.
    void apply(World& p_world,
               EntityId p_camera,
               CameraInput const& p_input,
               float p_dt);
};

} // namespace compages::world
