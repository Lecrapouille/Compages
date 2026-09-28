// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
