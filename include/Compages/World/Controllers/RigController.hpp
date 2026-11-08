// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Frame.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/World/Controllers/Input.hpp"
#include "Compages/World/EntityId.hpp"

#include <optional>

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief What a camera rig reads in one frame.
//!
//! Filled by \c rigInput() from an \c Input, or by hand in a test.
// ****************************************************************************
struct RigInput
{
    //! \brief Mouse in clip space (−1…1, y up): the point the pan grabs.
    compages::Vector2f mouse{ 0.0f, 0.0f };
    //! \brief Mouse motion this frame, in pixels, y up (turns the rig).
    compages::Vector2f look_delta{ 0.0f, 0.0f };
    //! \brief Scroll wheel this frame; positive zooms in.
    float zoom_delta = 0.0f;
    //! \brief Grab the ground under the mouse and drag it (left button).
    bool pan = false;
    //! \brief Turn around the target with the mouse (right or middle button).
    bool rotate = false;
    //! \brief Move the target along the ground, in the direction the camera
    //! faces.
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    //! \brief Turn around the target.
    bool turn_left = false;
    bool turn_right = false;
    //! \brief Come closer, go away.
    bool zoom_in = false;
    bool zoom_out = false;
    //! \brief Hold to move, turn and zoom faster.
    bool boost = false;
    //! \brief Width / height of the viewport.
    float aspect = 1.0f;
    //! \brief Vertical field of view, in radians.
    float fov = 0.785398f;
};

// ----------------------------------------------------------------------------
//! \brief The usual mapping: the left button drags the ground, the right or
//! middle button turns, the wheel zooms; W A S D or the arrows move, Q and E
//! turn, R and F zoom, Shift runs.
//!
//! The mouse counts only while it is over the picture.
//! \param[in] p_input One frame of input; its \c mouse in the pixels of
//!   \p p_frame, origin bottom-left.
//! \param[in] p_frame The viewport size.
//! \param[in] p_fov Vertical field of view of the camera, in radians.
// ----------------------------------------------------------------------------
[[nodiscard]] RigInput rigInput(Input const& p_input, compages::Frame const& p_frame, float p_fov);

// ****************************************************************************
//! \brief A strategy-game camera: it looks at a target on the ground, from a
//! distance, turned by a yaw and a pitch.
//!
//! The input changes the wanted pose; the camera glides to it. The glide
//! does not depend on the frame rate: each frame it covers the part
//! \c 1 - exp(-sharpness * dt) of the way left, so it never overshoots,
//! whatever \c dt. The pan keeps the grabbed point of the ground under the
//! mouse: it is computed with the wanted pose, not with the gliding one, so
//! the ground does not slide away while the camera catches up. The wheel
//! zooms toward the point of the ground under the mouse, like a map: that
//! point stays under the mouse.
//!
//! A jump to another place (a click on a name, a menu) is a flight, not a
//! glide: flyTo() goes up, across and down along the path of van Wijk and
//! Nuij, the one a map viewer takes. The path starts and ends with no speed
//! and no acceleration, so the camera neither jerks off nor stops dead; the
//! farther the place, the higher the camera goes, so that both ends are seen
//! in the middle of the trip. Any input of the user stops the flight where
//! it is.
//!
//! The world is Y-up; the ground is the plane \c y = \c ground.
//!
//! \code
//! compages::world::RigController rig;
//! rig.target = { 0, 0, 0 };
//! rig.apply(world, cam, compages::world::rigInput(frame.input, frame, fov), frame.elapsed);
//! \endcode
// ****************************************************************************
class RigController
{
public:

    //! \brief Wanted point looked at, on the ground.
    compages::Vector3f target{ 0.0f, 0.0f, 0.0f };
    //! \brief Wanted angle around the vertical axis (rad).
    float yaw = 0.0f;
    //! \brief Wanted angle above the horizon (rad), negative looks down.
    float pitch = -0.6f;
    //! \brief Wanted distance from the target.
    float distance = 5.0f;

    //! \brief Height of the ground plane the pan slides on.
    float ground = 0.0f;
    //! \brief How fast the camera catches up with the wanted pose (1/s).
    float sharpness = 12.0f;
    //! \brief Key move speed, in distances per second (far: faster).
    float move_speed = 0.8f;
    //! \brief Key turn speed (rad/s).
    float turn_speed = 1.5f;
    //! \brief Key zoom speed: log of the distance change per second.
    float zoom_speed = 1.2f;
    //! \brief Factor of the speeds while \c boost is held.
    float boost = 3.0f;
    //! \brief Radians per pixel of mouse motion while rotating.
    float look_sensitivity = 0.005f;
    //! \brief Log of the distance change per notch of the wheel.
    float zoom_step = 0.12f;
    float min_distance = 0.2f;
    float max_distance = 100.0f;
    float min_pitch = -1.55f;
    float max_pitch = 0.3f;
    //! \brief Seconds of a flight per unit of its path: a path unit is a
    //! zoom by e (2.7 times) or a pan of one view width.
    float flight_pace = 0.8f;

    // ------------------------------------------------------------------------
    //! \brief Consume one frame of input, glide and write the camera pose.
    //! \param[in] p_dt Seconds since the previous frame.
    // ------------------------------------------------------------------------
    void apply(World& p_world, EntityId p_camera, RigInput const& p_input, float p_dt);

    // ------------------------------------------------------------------------
    //! \brief Jump to the wanted pose now (no glide), and write it.
    // ------------------------------------------------------------------------
    void snap(World& p_world, EntityId p_camera);

    // ------------------------------------------------------------------------
    //! \brief Fly to another place: look at \p p_target from \p p_distance,
    //! by the smooth path described above. The yaw and the pitch glide to
    //! \c yaw and \c pitch on the way.
    //! \param[in] p_target The point of the ground to look at.
    //! \param[in] p_distance The distance to see it from (clamped to the
    //!   limits; the camera may go beyond \c max_distance on the way).
    // ------------------------------------------------------------------------
    void flyTo(compages::Vector3f const& p_target, float p_distance);

    //! \brief Is a flight under way?
    [[nodiscard]] bool flying() const
    {
        return m_flight.has_value();
    }

    //! \brief The point looked at now (gliding toward \c target).
    [[nodiscard]] compages::Vector3f currentTarget() const
    {
        return m_target;
    }

    //! \brief The distance now (gliding toward \c distance).
    [[nodiscard]] float currentDistance() const
    {
        return m_distance;
    }

private:

    void writePose(World& p_world, EntityId p_camera) const;

    //! \brief A flight under way: the path of van Wijk and Nuij from one
    //! view to another, as a pan \c u and a view width \c w, both functions
    //! of the length \c s along the path.
    struct Flight
    {
        compages::Vector3f from;
        compages::Vector3f to;
        //! \brief Distance to the target at both ends.
        float w0 = 1.0f;
        float w1 = 1.0f;
        //! \brief How far the targets are apart.
        float u1 = 0.0f;
        //! \brief The first of the two constants of the path.
        float r0 = 0.0f;
        //! \brief Length of the path, its duration, the time flown.
        float length = 0.0f;
        float duration = 0.0f;
        float time = 0.0f;
    };

    //! \brief Advance the flight by \p p_dt and write where it is in the
    //! pose now; forget it at its end.
    void fly(float p_dt);

    //! Pose now: it glides toward the wanted one.
    compages::Vector3f m_target{ 0.0f, 0.0f, 0.0f };
    float m_yaw = 0.0f;
    float m_pitch = -0.6f;
    float m_distance = 5.0f;
    //! Point of the ground grabbed by the pan.
    compages::Vector3f m_grab{ 0.0f, 0.0f, 0.0f };
    bool m_grabbing = false;
    std::optional<Flight> m_flight;
};

} // namespace compages::world
