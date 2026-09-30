// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"
#include "Compages/World/Controllers/CameraInput.hpp"
#include "Compages/World/EntityId.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
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

    compages::core::Vector3f target{ 0.0f, 0.0f, 0.0f };
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
    //! input. What a demo uses to drive the camera from \c compages::core::Frame::total.
    // ------------------------------------------------------------------------
    void writePose(World& p_world, EntityId p_camera) const;
};

} // namespace compages::world
