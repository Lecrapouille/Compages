// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Controllers/FlyController.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>

namespace compages::world
{

void FlyController::apply(World& p_world,
                          EntityId p_camera,
                          CameraInput const& p_input,
                          float p_dt)
{
    if (p_input.look)
    {
        yaw -= p_input.look_delta.x * look_sensitivity;
        pitch += p_input.look_delta.y * look_sensitivity;
        pitch = std::clamp(pitch, min_pitch, max_pitch);
    }

    const compages::core::Quatf q_yaw = compages::core::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(yaw)),
        compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Quatf q_pitch = compages::core::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(pitch)),
        compages::core::Vector3f(1.0f, 0.0f, 0.0f));
    const compages::core::Quatf rotation = q_yaw * q_pitch;

    compages::core::Vector3f motion(0.0f);
    if (p_input.forward)
    {
        motion += compages::core::Vector3f(0.0f, 0.0f, -1.0f);
    }
    if (p_input.back)
    {
        motion += compages::core::Vector3f(0.0f, 0.0f, 1.0f);
    }
    if (p_input.left)
    {
        motion += compages::core::Vector3f(-1.0f, 0.0f, 0.0f);
    }
    if (p_input.right)
    {
        motion += compages::core::Vector3f(1.0f, 0.0f, 0.0f);
    }
    if (p_input.up)
    {
        motion += compages::core::Vector3f(0.0f, 1.0f, 0.0f);
    }
    if (p_input.down)
    {
        motion += compages::core::Vector3f(0.0f, -1.0f, 0.0f);
    }

    LocalTransformView local = p_world.transform(p_camera);
    local.rotation = rotation;
    const float length = compages::core::vector::norm(motion);
    if (length > 1.0e-6f)
    {
        const float speed =
            move_speed * (p_input.boost ? boost_multiplier : 1.0f);
        local.position += rotation * ((motion / length) * speed * p_dt);
    }
}

} // namespace compages::world
