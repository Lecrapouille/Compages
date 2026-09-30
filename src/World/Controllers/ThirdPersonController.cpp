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

#include "Compages/World/Controllers/ThirdPersonController.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cmath>

namespace compages::world
{

namespace
{

compages::core::Quatf yawPitch(float p_yaw, float p_pitch)
{
    const compages::core::Quatf q_yaw = compages::core::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_yaw)),
        compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Quatf q_pitch = compages::core::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_pitch)),
        compages::core::Vector3f(1.0f, 0.0f, 0.0f));
    return q_yaw * q_pitch;
}

compages::core::Vector3f targetPoint(World const& p_world,
                     EntityId p_target,
                     compages::core::Vector3f const& p_offset)
{
    if (!p_world.alive(p_target))
    {
        return p_offset;
    }
    if (p_world.parent(p_target).valid())
    {
        const compages::core::Matrix44f& world = p_world.worldMatrix(p_target);
        return compages::core::translation(world) + p_offset;
    }
    return p_world.transform(p_target).position + p_offset;
}

} // namespace

//------------------------------------------------------------------------------
void ThirdPersonController::apply(World& p_world,
                                  EntityId p_camera,
                                  CameraInput const& p_input)
{
    if (p_input.look)
    {
        yaw -= p_input.look_delta.x * look_sensitivity;
        pitch += p_input.look_delta.y * look_sensitivity;
    }
    distance = std::clamp(distance - (p_input.zoom_delta * zoom_sensitivity),
                          min_distance,
                          max_distance);
    pitch = std::clamp(pitch, min_pitch, max_pitch);
    writePose(p_world, p_camera);
}

//------------------------------------------------------------------------------
void ThirdPersonController::writePose(World& p_world, EntityId p_camera) const
{
    const compages::core::Vector3f focus = targetPoint(p_world, target, look_offset);
    const compages::core::Quatf rotation = yawPitch(yaw, pitch);
    const compages::core::Vector3f forward = rotation * compages::core::Vector3f(0.0f, 0.0f, -1.0f);
    LocalTransformView local = p_world.transform(p_camera);
    local.position = focus - (forward * distance);
    local.rotation = rotation;
}

} // namespace compages::world
