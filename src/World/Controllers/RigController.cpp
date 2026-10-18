// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Controllers/RigController.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cmath>
#include <optional>

namespace compages::world
{

namespace
{

//! \brief Rotation of a camera turned by \p p_yaw about Y, then \p p_pitch
//! about X: forward is \c rotation * (0, 0, -1).
compages::core::Quatf yawPitch(float p_yaw, float p_pitch)
{
    const compages::core::Quatf q_yaw = compages::core::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_yaw)), compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Quatf q_pitch = compages::core::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_pitch)), compages::core::Vector3f(1.0f, 0.0f, 0.0f));
    return q_yaw * q_pitch;
}

//! \brief Eye of a camera looking at \p p_target from \p p_distance.
compages::core::Vector3f eyeOf(compages::core::Vector3f const& p_target, float p_yaw, float p_pitch, float p_distance)
{
    const compages::core::Vector3f forward = yawPitch(p_yaw, p_pitch) * compages::core::Vector3f(0.0f, 0.0f, -1.0f);
    return p_target - (forward * p_distance);
}

//! \brief Where the ray of the camera through the clip point \p p_mouse
//! meets the plane \c y = \p p_ground, or nothing (the ray looks up).
std::optional<compages::core::Vector3f> groundUnder(compages::core::Vector3f const& p_target,
                                                    float p_yaw,
                                                    float p_pitch,
                                                    float p_distance,
                                                    RigInput const& p_input,
                                                    float p_ground)
{
    const float half = std::tan(p_input.fov * 0.5f);
    const compages::core::Vector3f local(p_input.mouse.x * half * p_input.aspect, p_input.mouse.y * half, -1.0f);
    const compages::core::Vector3f ray = yawPitch(p_yaw, p_pitch) * local;
    const compages::core::Vector3f eye = eyeOf(p_target, p_yaw, p_pitch, p_distance);
    if (ray.y > -1.0e-4f)
    {
        return std::nullopt;
    }
    const float t = (p_ground - eye.y) / ray.y;
    // Far grazing hits make the pan jump: ignore them.
    if (t < 0.0f || t > 50.0f * p_distance)
    {
        return std::nullopt;
    }
    return eye + (ray * t);
}

//! \brief Part of the way left a glide covers in \p p_dt: never more than
//! all of it, the same path at any frame rate.
float glide(float p_sharpness, float p_dt)
{
    return 1.0f - std::exp(-p_sharpness * std::max(p_dt, 0.0f));
}

} // namespace

RigInput rigInput(Input const& p_input, compages::core::Frame const& p_frame, float p_fov)
{
    RigInput rig;
    rig.aspect = compages::core::aspect(p_frame);
    rig.fov = p_fov;
    if (p_frame.width > 0u && p_frame.height > 0u)
    {
        rig.mouse = compages::core::Vector2f((p_input.mouse.x / float(p_frame.width)) * 2.0f - 1.0f,
                                             (p_input.mouse.y / float(p_frame.height)) * 2.0f - 1.0f);
    }
    if (p_input.mouse_over)
    {
        rig.look_delta = p_input.mouse_delta;
        rig.zoom_delta = p_input.scroll;
        rig.pan = p_input.mouse_left;
        rig.rotate = p_input.mouse_right || p_input.mouse_middle;
    }
    rig.forward = p_input.down(Key::W) || p_input.down(Key::Up);
    rig.back = p_input.down(Key::S) || p_input.down(Key::Down);
    rig.left = p_input.down(Key::A) || p_input.down(Key::Left);
    rig.right = p_input.down(Key::D) || p_input.down(Key::Right);
    rig.turn_left = p_input.down(Key::Q);
    rig.turn_right = p_input.down(Key::E);
    rig.zoom_in = p_input.down(Key::R);
    rig.zoom_out = p_input.down(Key::F);
    rig.boost = p_input.down(Key::Shift);
    return rig;
}

void RigController::apply(World& p_world, EntityId p_camera, RigInput const& p_input, float p_dt)
{
    const float fast = p_input.boost ? boost : 1.0f;

    // --- The mouse --------------------------------------------------------
    if (p_input.rotate)
    {
        yaw -= p_input.look_delta.x * look_sensitivity;
        pitch += p_input.look_delta.y * look_sensitivity;
    }
    distance *= std::exp(-p_input.zoom_delta * zoom_step);
    if (p_input.pan)
    {
        // The grabbed point stays under the mouse in the wanted view.
        const std::optional<compages::core::Vector3f> hit = groundUnder(target, yaw, pitch, distance, p_input, ground);
        if (!m_grabbing)
        {
            m_grabbing = hit.has_value();
            m_grab = hit.value_or(m_grab);
        }
        else if (hit)
        {
            target = target + (m_grab - *hit);
        }
    }
    else
    {
        m_grabbing = false;
    }

    // --- The keys ---------------------------------------------------------
    const compages::core::Vector3f ahead(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const compages::core::Vector3f side(std::cos(yaw), 0.0f, -std::sin(yaw));
    const float step = move_speed * fast * distance * p_dt;
    const float forward = float(p_input.forward) - float(p_input.back);
    const float sideways = float(p_input.right) - float(p_input.left);
    target = target + (ahead * (forward * step)) + (side * (sideways * step));
    yaw += (float(p_input.turn_left) - float(p_input.turn_right)) * turn_speed * fast * p_dt;
    distance *= std::exp((float(p_input.zoom_out) - float(p_input.zoom_in)) * zoom_speed * fast * p_dt);

    distance = std::clamp(distance, min_distance, max_distance);
    pitch = std::clamp(pitch, min_pitch, max_pitch);

    // --- The glide --------------------------------------------------------
    const float k = glide(sharpness, p_dt);
    m_target = m_target + ((target - m_target) * k);
    m_yaw += (yaw - m_yaw) * k;
    m_pitch += (pitch - m_pitch) * k;
    // The log of the distance glides: a zoom looks even, near or far.
    m_distance = std::exp(std::log(m_distance) + (std::log(distance) - std::log(m_distance)) * k);
    writePose(p_world, p_camera);
}

void RigController::snap(World& p_world, EntityId p_camera)
{
    distance = std::clamp(distance, min_distance, max_distance);
    pitch = std::clamp(pitch, min_pitch, max_pitch);
    m_target = target;
    m_yaw = yaw;
    m_pitch = pitch;
    m_distance = distance;
    m_grabbing = false;
    writePose(p_world, p_camera);
}

void RigController::writePose(World& p_world, EntityId p_camera) const
{
    LocalTransformView local = p_world.transform(p_camera);
    local.position = eyeOf(m_target, m_yaw, m_pitch, m_distance);
    local.rotation = yawPitch(m_yaw, m_pitch);
}

} // namespace compages::world
