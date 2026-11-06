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
//! orbit.apply(world, cam, compages::world::cameraInput(input), frame.elapsed);
//! \endcode
// ****************************************************************************
class OrbitController
{
public:

    compages::Vector3f target{ 0.0f, 0.0f, 0.0f };
    float yaw = 0.0f;
    float pitch = -0.35f;
    //! \brief The distance the wheel asks for. The camera glides to it.
    float distance = 40.0f;

    float look_sensitivity = 0.005f;
    //! \brief Log of the distance change per notch of the wheel: each notch
    //! brings the camera the same part of the way closer (about 11 %),
    //! whether the scene is a room or a galaxy.
    float zoom_step = 0.12f;
    //! \brief How fast the shown distance follows the asked one. Each frame
    //! covers 1 - exp(-sharpness * dt) of the way: the same glide at any
    //! frame rate, never past the end.
    float sharpness = 12.0f;
    //! \brief How many notches one notch is while Shift is held: crossing
    //! from a rocket to the Moon takes a few turns of the wheel, not a
    //! hundred.
    float boost = 6.0f;
    float min_distance = 2.0f;
    float max_distance = 200.0f;
    float min_pitch = -1.45f;
    float max_pitch = 1.45f;

    // ------------------------------------------------------------------------
    //! \brief Consume one frame of input and write the camera pose.
    //! \param[in] p_dt seconds since the last frame, for the zoom glide.
    // ------------------------------------------------------------------------
    void apply(World& p_world,
               EntityId p_camera,
               CameraInput const& p_input,
               float p_dt);

    // ------------------------------------------------------------------------
    //! \brief Write the pose from the current yaw/pitch/distance, ignoring
    //! input. What a demo uses to drive the camera from \c compages::Frame::total.
    // ------------------------------------------------------------------------
    void writePose(World& p_world, EntityId p_camera) const;

    //! \brief Jump to the asked distance, without gliding: after placing
    //! the camera by hand.
    void snap() { m_distance = distance; }

private:

    //! \brief The distance shown, gliding to \c distance. Zero until the
    //! first apply(): the asked distance is shown as it is.
    float m_distance = 0.0f;
};

} // namespace compages::world
