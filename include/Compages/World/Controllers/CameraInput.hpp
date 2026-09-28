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

#pragma once

#include "Compages/Core/Vector.hpp"
#include "Compages/World/Controllers/Input.hpp"

namespace compages::world
{

// ****************************************************************************
//! \brief What a camera controller reads in one frame.
//!
//! Controllers never talk to a windowing library. The application fills this
//! from GLFW, SDL, a test harness or a recorded demo; the controller only
//! writes a LocalTransform. That is why the same OrbitController can be
//! driven by a unit test without opening a window.
//!
//! \code
//! compages::world::CameraInput cam = compages::world::cameraInput(frame.input);
//! if (cam.look) { orbit.applyLook(cam.look_delta); }
//! \endcode
// ****************************************************************************
struct CameraInput
{
    //! \brief Mouse motion this frame, in pixels, y up. Look is typically
    //! applied only when \c look is true.
    Vector2f look_delta{ 0.0f, 0.0f };
    //! \brief Scroll wheel this frame. Positive is "away from the user",
    //! which an orbit controller treats as zoom in.
    float zoom_delta = 0.0f;
    //! \brief The user is dragging to look (right mouse, captured cursor, …).
    bool look = false;
    //! \brief Translation keys. Names are the intended direction in camera
    //! space, not the physical key: the application maps WASD/QE itself.
    bool forward = false;
    bool back = false;
    bool left = false;
    bool right = false;
    bool up = false;
    bool down = false;
    //! \brief Hold to multiply move speed (Left Shift in the examples).
    bool boost = false;
};

// ----------------------------------------------------------------------------
//! \brief The usual mapping: the right button looks around, the wheel zooms,
//! W A S D move, E and Q go up and down, Shift runs.
//!
//! The mouse only counts while it is over the picture, so dragging a panel
//! slider does not also turn the camera.
// ----------------------------------------------------------------------------
[[nodiscard]] inline CameraInput cameraInput(Input const& p_input)
{
    CameraInput camera;
    if (p_input.mouse_over)
    {
        camera.look_delta = p_input.mouse_delta;
        camera.zoom_delta = p_input.scroll;
        camera.look = p_input.mouse_right;
    }
    camera.forward = p_input.down(Key::W);
    camera.back = p_input.down(Key::S);
    camera.left = p_input.down(Key::A);
    camera.right = p_input.down(Key::D);
    camera.up = p_input.down(Key::E);
    camera.down = p_input.down(Key::Q);
    camera.boost = p_input.down(Key::Shift);
    return camera;
}

} // namespace compages::world
