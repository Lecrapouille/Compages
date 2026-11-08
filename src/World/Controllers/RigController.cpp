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
compages::Quatf yawPitch(float p_yaw, float p_pitch)
{
    const compages::Quatf q_yaw = compages::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_yaw)), compages::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::Quatf q_pitch = compages::Quatf::fromAngleAxis(
        units::angle::radian_t(static_cast<double>(p_pitch)), compages::Vector3f(1.0f, 0.0f, 0.0f));
    return q_yaw * q_pitch;
}

//! \brief Eye of a camera looking at \p p_target from \p p_distance.
compages::Vector3f eyeOf(compages::Vector3f const& p_target, float p_yaw, float p_pitch, float p_distance)
{
    const compages::Vector3f forward = yawPitch(p_yaw, p_pitch) * compages::Vector3f(0.0f, 0.0f, -1.0f);
    return p_target - (forward * p_distance);
}

//! \brief Where the ray of the camera through the clip point \p p_mouse
//! meets the plane \c y = \p p_ground, or nothing (the ray looks up).
std::optional<compages::Vector3f> groundUnder(compages::Vector3f const& p_target,
                                                    float p_yaw,
                                                    float p_pitch,
                                                    float p_distance,
                                                    RigInput const& p_input,
                                                    float p_ground)
{
    const float half = std::tan(p_input.fov * 0.5f);
    const compages::Vector3f local(p_input.mouse.x * half * p_input.aspect, p_input.mouse.y * half, -1.0f);
    const compages::Vector3f ray = yawPitch(p_yaw, p_pitch) * local;
    const compages::Vector3f eye = eyeOf(p_target, p_yaw, p_pitch, p_distance);
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

//! \brief How much a flight zooms out for a pan: the rho of van Wijk and
//! Nuij. Their tests found sqrt(2) the most pleasing.
constexpr double FLIGHT_RHO = 1.41421356;

//! \brief An ease with no speed and no acceleration at 0 and 1: what
//! starts and stops a flight without a jolt (Perlin's smootherstep).
double smootherstep(double p_x)
{
    return p_x * p_x * p_x * ((p_x * ((p_x * 6.0) - 15.0)) + 10.0);
}

//! \brief Part of the way left a glide covers in \p p_dt: never more than
//! all of it, the same path at any frame rate.
float glide(float p_sharpness, float p_dt)
{
    return 1.0f - std::exp(-p_sharpness * std::max(p_dt, 0.0f));
}

} // namespace

RigInput rigInput(Input const& p_input, compages::Frame const& p_frame, float p_fov)
{
    RigInput rig;
    rig.aspect = compages::aspect(p_frame);
    rig.fov = p_fov;
    if (p_frame.width > 0u && p_frame.height > 0u)
    {
        rig.mouse = compages::Vector2f((p_input.mouse.x / float(p_frame.width)) * 2.0f - 1.0f,
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
    // Any input stops a flight where the camera is now.
    const bool touched = p_input.pan || p_input.rotate || (std::abs(p_input.zoom_delta) > 0.0f) ||
                         p_input.forward || p_input.back || p_input.left || p_input.right ||
                         p_input.turn_left || p_input.turn_right || p_input.zoom_in ||
                         p_input.zoom_out;
    if (m_flight && touched)
    {
        target = m_target;
        distance = m_distance;
        m_flight.reset();
    }
    if (std::abs(p_input.zoom_delta) > 0.0f)
    {
        // Toward the point under the mouse: it stays under the mouse.
        const std::optional<compages::Vector3f> before =
            groundUnder(target, yaw, pitch, distance, p_input, ground);
        distance = std::clamp(distance * std::exp(-p_input.zoom_delta * zoom_step),
                              min_distance, max_distance);
        const std::optional<compages::Vector3f> after =
            groundUnder(target, yaw, pitch, distance, p_input, ground);
        if (before && after)
        {
            target = target + (*before - *after);
        }
    }
    if (p_input.pan)
    {
        // The grabbed point stays under the mouse in the wanted view.
        const std::optional<compages::Vector3f> hit = groundUnder(target, yaw, pitch, distance, p_input, ground);
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
    const compages::Vector3f ahead(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const compages::Vector3f side(std::cos(yaw), 0.0f, -std::sin(yaw));
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
    m_yaw += (yaw - m_yaw) * k;
    m_pitch += (pitch - m_pitch) * k;
    if (m_flight)
    {
        fly(p_dt);
    }
    else
    {
        m_target = m_target + ((target - m_target) * k);
        // The log of the distance glides: a zoom looks even, near or far.
        m_distance =
            std::exp(std::log(m_distance) + (std::log(distance) - std::log(m_distance)) * k);
    }
    writePose(p_world, p_camera);
}

void RigController::flyTo(compages::Vector3f const& p_target, float p_distance)
{
    Flight flight;
    flight.from = m_target;
    flight.to = p_target;
    flight.w0 = m_distance;
    flight.w1 = std::clamp(p_distance, min_distance, max_distance);
    const double w0 = double(flight.w0);
    const double w1 = double(flight.w1);
    const double u1 = double(compages::vector::norm(p_target - m_target));
    const double rho2 = FLIGHT_RHO * FLIGHT_RHO;
    if (u1 < (1.0e-6 * std::max(w0, w1)))
    {
        // No pan: a zoom alone, by the same ratio each second.
        flight.length = float(std::abs(std::log(w1 / w0)) / FLIGHT_RHO);
    }
    else
    {
        // van Wijk and Nuij, "Smooth and efficient zooming and panning"
        // (2003): r_i = ln(-b_i + sqrt(b_i^2 + 1)) = -asinh(b_i).
        const double b0 = ((w1 * w1) - (w0 * w0) + (rho2 * rho2 * u1 * u1)) / (2.0 * w0 * rho2 * u1);
        const double b1 = ((w1 * w1) - (w0 * w0) - (rho2 * rho2 * u1 * u1)) / (2.0 * w1 * rho2 * u1);
        const double r0 = -std::asinh(b0);
        const double r1 = -std::asinh(b1);
        flight.u1 = float(u1);
        flight.r0 = float(r0);
        flight.length = float((r1 - r0) / FLIGHT_RHO);
    }
    flight.duration = std::max(0.25f, flight.length * flight_pace);
    target = p_target;
    distance = flight.w1;
    m_grabbing = false;
    m_flight = flight;
}

void RigController::fly(float p_dt)
{
    Flight& flight = *m_flight;
    flight.time += std::max(p_dt, 0.0f);
    const double x = std::min(double(flight.time / flight.duration), 1.0);
    const double eased = smootherstep(x);
    const double w0 = double(flight.w0);
    if (flight.u1 <= 0.0f)
    {
        m_target = flight.to;
        m_distance = float(w0 * std::pow(double(flight.w1) / w0, eased));
    }
    else
    {
        const double s = eased * double(flight.length);
        const double r0 = double(flight.r0);
        const double rho2 = FLIGHT_RHO * FLIGHT_RHO;
        const double u = (w0 / rho2) * ((std::cosh(r0) * std::tanh((FLIGHT_RHO * s) + r0)) -
                                         std::sinh(r0));
        m_target = flight.from + ((flight.to - flight.from) * float(u / double(flight.u1)));
        m_distance = float(w0 * std::cosh(r0) / std::cosh((FLIGHT_RHO * s) + r0));
    }
    if (x >= 1.0)
    {
        m_target = flight.to;
        m_distance = flight.w1;
        m_flight.reset();
    }
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
    m_flight.reset();
    writePose(p_world, p_camera);
}

void RigController::writePose(World& p_world, EntityId p_camera) const
{
    LocalTransformView local = p_world.transform(p_camera);
    local.position = eyeOf(m_target, m_yaw, m_pitch, m_distance);
    local.rotation = yawPitch(m_yaw, m_pitch);
}

} // namespace compages::world
