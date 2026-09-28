// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Controllers/FPSController.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cmath>

namespace compages::world
{

void FPSController::apply(World& p_world,
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

    const Quatf q_yaw = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(yaw)),
        Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf q_pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(pitch)),
        Vector3f(1.0f, 0.0f, 0.0f));

    // Walk on XZ using yaw only. At yaw = 0 the camera looks along -Z, so
    // forward on the ground is (0, 0, -1).
    Vector3f walk(0.0f);
    const Vector3f forward_xz = q_yaw * Vector3f(0.0f, 0.0f, -1.0f);
    const Vector3f right_xz = q_yaw * Vector3f(1.0f, 0.0f, 0.0f);
    if (p_input.forward)
    {
        walk += forward_xz;
    }
    if (p_input.back)
    {
        walk -= forward_xz;
    }
    if (p_input.left)
    {
        walk -= right_xz;
    }
    if (p_input.right)
    {
        walk += right_xz;
    }
    walk.y = 0.0f;

    LocalTransformView local = p_world.transform(p_camera);
    local.rotation = q_yaw * q_pitch;
    if (p_input.up)
    {
        eye_height += move_speed * p_dt;
    }
    if (p_input.down)
    {
        eye_height = std::max(0.2f, eye_height - (move_speed * p_dt));
    }
    const float length = compages::vector::norm(walk);
    if (length > 1.0e-6f)
    {
        const float speed =
            move_speed * (p_input.boost ? boost_multiplier : 1.0f);
        local.position += (walk / length) * speed * p_dt;
    }
    local.position.y = eye_height;
}

} // namespace compages::world
