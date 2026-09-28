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

#include "Compages/World/Controllers/FlyController.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>

namespace compages::world
{

//------------------------------------------------------------------------------
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

    const Quatf q_yaw = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(yaw)),
        Vector3f(0.0f, 1.0f, 0.0f));
    const Quatf q_pitch = Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(pitch)),
        Vector3f(1.0f, 0.0f, 0.0f));
    const Quatf rotation = q_yaw * q_pitch;

    Vector3f motion(0.0f);
    if (p_input.forward)
    {
        motion += Vector3f(0.0f, 0.0f, -1.0f);
    }
    if (p_input.back)
    {
        motion += Vector3f(0.0f, 0.0f, 1.0f);
    }
    if (p_input.left)
    {
        motion += Vector3f(-1.0f, 0.0f, 0.0f);
    }
    if (p_input.right)
    {
        motion += Vector3f(1.0f, 0.0f, 0.0f);
    }
    if (p_input.up)
    {
        motion += Vector3f(0.0f, 1.0f, 0.0f);
    }
    if (p_input.down)
    {
        motion += Vector3f(0.0f, -1.0f, 0.0f);
    }

    LocalTransformView local = p_world.transform(p_camera);
    local.rotation = rotation;
    const float length = compages::vector::norm(motion);
    if (length > 1.0e-6f)
    {
        const float speed =
            move_speed * (p_input.boost ? boost_multiplier : 1.0f);
        local.position += rotation * ((motion / length) * speed * p_dt);
    }
}

} // namespace compages::world
