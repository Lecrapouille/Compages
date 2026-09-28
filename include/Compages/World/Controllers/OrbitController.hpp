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
//! \brief A camera that orbits a target point.
//!
//! Yaw and pitch are the primary data. Each \c apply() writes the camera
//! EntityId's local pose so that it looks at \c target from \c distance. The
//! controller is not a component: it is a system the application owns and
//! runs before \c World::update().
//!
//! \code
//! compages::world::OrbitController orbit;
//! orbit.target = { 0, 1, 0 };
//! orbit.apply(world, cam, compages::world::cameraInput(input));
//! \endcode
// ****************************************************************************
class OrbitController
{
public:

    Vector3f target{ 0.0f, 0.0f, 0.0f };
    float yaw = 0.0f;
    float pitch = -0.35f;
    float distance = 40.0f;

    float look_sensitivity = 0.005f;
    float zoom_sensitivity = 2.5f;
    float min_distance = 2.0f;
    float max_distance = 200.0f;
    float min_pitch = -1.45f;
    float max_pitch = 1.45f;

    // ------------------------------------------------------------------------
    //! \brief Consume one frame of input and write the camera pose.
    // ------------------------------------------------------------------------
    void apply(World& p_world,
               EntityId p_camera,
               CameraInput const& p_input);

    // ------------------------------------------------------------------------
    //! \brief Write the pose from the current yaw/pitch/distance, ignoring
    //! input. What a demo uses to drive the camera from \c Frame::total.
    // ------------------------------------------------------------------------
    void writePose(World& p_world, EntityId p_camera) const;
};

} // namespace compages::world
