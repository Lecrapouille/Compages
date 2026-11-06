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
//! follow.apply(world, cam, compages::world::cameraInput(input), frame.elapsed);
//! \endcode
// ****************************************************************************
class ThirdPersonController
{
public:

    EntityId target{};
    compages::Vector3f look_offset{ 0.0f, 1.6f, 0.0f };
    float yaw = 0.0f;
    float pitch = -0.25f;
    //! \brief The distance the wheel asks for. The camera glides to it.
    float distance = 8.0f;

    float look_sensitivity = 0.005f;
    //! \brief Log of the distance change per notch of the wheel: each notch
    //! brings the camera the same part of the way closer (about 11 %),
    //! whether the scene is a room or a galaxy.
    float zoom_step = 0.12f;
    //! \brief How fast the shown distance follows the asked one. Each frame
    //! covers 1 - exp(-sharpness * dt) of the way: the same glide at any
    //! frame rate, never past the end.
    float sharpness = 12.0f;
    float min_distance = 1.5f;
    float max_distance = 40.0f;
    float min_pitch = -1.2f;
    float max_pitch = 0.6f;

    //! \brief Orbit \c target using mouse look and wheel zoom.
    //! \param[in] p_dt seconds since the last frame, for the zoom glide.
    void apply(World& p_world,
               EntityId p_camera,
               CameraInput const& p_input,
               float p_dt);

    //! \brief Place the camera from yaw/pitch/distance without reading input.
    void writePose(World& p_world, EntityId p_camera) const;

    //! \brief Jump to the asked distance, without gliding.
    void snap() { m_distance = distance; }

private:

    //! \brief The distance shown, gliding to \c distance; zero before the
    //! first apply().
    float m_distance = 0.0f;
};

} // namespace compages::world
