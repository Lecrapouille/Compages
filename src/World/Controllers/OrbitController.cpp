// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Controllers/OrbitController.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cmath>

namespace compages::world
{

namespace
{

compages::Quatf yawPitch(float p_yaw, float p_pitch)
{
    const compages::Quatf q_yaw = compages::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_yaw)),
        compages::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::Quatf q_pitch = compages::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_pitch)),
        compages::Vector3f(1.0f, 0.0f, 0.0f));
    return q_yaw * q_pitch;
}

} // namespace

void OrbitController::apply(World& p_world,
                            EntityId p_camera,
                            CameraInput const& p_input,
                            float p_dt)
{
    if (p_input.look)
    {
        yaw -= p_input.look_delta.x * look_sensitivity;
        pitch += p_input.look_delta.y * look_sensitivity;
    }
    // Where the camera is now, read before the wheel changes the distance.
    const float shown = (m_distance > 0.0f) ? m_distance : distance;
    const float notches = p_input.zoom_delta * (p_input.boost ? boost : 1.0f);
    distance = std::clamp(distance * std::exp(-notches * zoom_step),
                          min_distance,
                          max_distance);
    // The log of the distance glides: a zoom looks even, near or far, and a
    // notch of the wheel is a smooth move rather than a jump.
    const float k = 1.0f - std::exp(-sharpness * std::max(p_dt, 0.0f));
    m_distance =
        std::exp(std::log(shown) + ((std::log(distance) - std::log(shown)) * k));
    pitch = std::clamp(pitch, min_pitch, max_pitch);
    writePose(p_world, p_camera);
}

void OrbitController::writePose(World& p_world, EntityId p_camera) const
{
    const compages::Quatf rotation = yawPitch(yaw, pitch);
    const compages::Vector3f forward = rotation * compages::Vector3f(0.0f, 0.0f, -1.0f);
    LocalTransformView local = p_world.transform(p_camera);
    local.position = target - (forward * ((m_distance > 0.0f) ? m_distance : distance));
    local.rotation = rotation;
}

} // namespace compages::world
