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

#include <cstdint>

namespace compages::world
{

struct ViewFrame;

// ****************************************************************************
//! \brief Timing and viewport size for one simulation step (no input).
//!
//! Headless tests and server-side simulation pass a \c Frame to
//! \c World::update(Frame). Interactive apps use \c ViewFrame, which extends
//! \c Frame with input; slice to \c Frame or use \c Frame(viewFrame).
//!
//! \code
//! compages::world::Frame step;
//! step.elapsed = 1.0f / 60.0f;
//! step.width = 800;
//! step.height = 600;
//! world.update(step);
//! \endcode
// ****************************************************************************
struct Frame
{
    Frame() = default;

    //! \brief Copy timing and size from an interactive frame (drops input).
    explicit Frame(ViewFrame const& p_view);

    //! \brief Viewport width in pixels (projection, aspect ratio).
    std::uint32_t width = 0u;
    //! \brief Viewport height in pixels.
    std::uint32_t height = 0u;
    //! \brief Seconds since the previous step (multiply speeds by this).
    float elapsed = 0.0f;
    //! \brief Seconds since the simulation or example started.
    float total = 0.0f;
};

// ****************************************************************************
//! \brief Width divided by height; \c 1 when height is zero.
//!
//! Accepts \c ViewFrame through the \c Frame base subobject.
//!
//! \code
//! compages::matrix::perspective(60.0_deg, compages::world::aspect(frame), 0.1f, 100.0f);
//! \endcode
// ****************************************************************************
[[nodiscard]] inline float aspect(Frame const& p_frame)
{
    return (p_frame.height == 0u)
               ? 1.0f
               : float(p_frame.width) / float(p_frame.height);
}

} // namespace compages::world
