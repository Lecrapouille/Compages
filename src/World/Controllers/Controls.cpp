// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/World/Entity.hpp"

#include <algorithm>
#include <cmath>

namespace compages::world
{

//! \brief Recover yaw/pitch from a unit forward vector.
//!
//! Matches \c OrbitController and \c FlyController: rotation is
//! \c yaw(Y) * pitch(X), and forward is \c rotation * (0, 0, -1).
//! \param[in] p_forward Direction in world or parent space (need not be unit).
//! \param[out] p_yaw Horizontal angle; unchanged if \c p_forward is null.
//! \param[out] p_pitch Vertical angle; unchanged if \c p_forward is null.
static void yawPitchOf(Vector3f p_forward, float& p_yaw, float& p_pitch)
{
    const float length =
        std::sqrt(p_forward.x * p_forward.x + p_forward.y * p_forward.y +
                  p_forward.z * p_forward.z);
    if (length < 1.0e-6f)
    {
        return;
    }
    p_pitch = std::asin(std::clamp(p_forward.y / length, -1.0f, 1.0f));
    p_yaw = std::atan2(-p_forward.x, -p_forward.z);
}

void Orbit::start()
{
    const Vector3f offset = entity().position() - controller.target;
    const float distance = std::sqrt(offset.x * offset.x + offset.y * offset.y +
                                     offset.z * offset.z);
    // The zoom limits follow the size of the scene the camera was placed
    // for, whether it is a room or a galaxy.
    controller.min_distance =
        std::min(controller.min_distance, distance * 0.1f);
    controller.max_distance =
        std::max(controller.max_distance, distance * 4.0f);
    controller.zoom_sensitivity =
        std::max(controller.zoom_sensitivity, distance * 0.05f);
    controller.distance =
        std::clamp(distance, controller.min_distance, controller.max_distance);
    yawPitchOf(-offset, controller.yaw, controller.pitch);
    controller.pitch = std::clamp(
        controller.pitch, controller.min_pitch, controller.max_pitch);
}

void Orbit::update(float p_dt)
{
    const CameraInput camera = cameraInput(input());
    if (!camera.look)
    {
        controller.yaw += spin * p_dt;
    }
    controller.apply(world(), entity(), camera);
}

void Fly::start()
{
    yawPitchOf(entity().rotation() * Vector3f(0.0f, 0.0f, -1.0f),
               controller.yaw,
               controller.pitch);
}

void Fly::update(float p_dt)
{
    controller.apply(world(), entity(), cameraInput(input()), p_dt);
}

} // namespace compages::world
