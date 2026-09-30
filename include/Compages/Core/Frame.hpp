// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdint>

namespace compages::core
{

// ****************************************************************************
//! \brief Timing and viewport size for one simulation step (no input).
//!
//! Headless tests and server-side simulation pass a \c Frame to
//! \c World::update(Frame). Interactive apps use \c compages::world::ViewFrame,
//! which publicly inherits \c Frame; pass it to \c World::update(ViewFrame) or
//! slice with \c static_cast<Frame const&>(viewFrame) when only timing/size matter.
//!
//! \code
//! compages::core::Frame step;
//! step.elapsed = 1.0f / 60.0f;
//! step.width = 800;
//! step.height = 600;
//! world.update(step);
//! \endcode
// ****************************************************************************
struct Frame
{
    Frame() = default;

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
//! compages::core::perspective(60.0_deg, compages::world::aspect(frame),
//! 0.1f, 100.0f);
//! \endcode
// ****************************************************************************
[[nodiscard]] inline float aspect(Frame const& p_frame)
{
    return (p_frame.height == 0u)
               ? 1.0f
               : float(p_frame.width) / float(p_frame.height);
}

} // namespace compages::core

