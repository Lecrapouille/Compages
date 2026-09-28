// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Units.hpp"
#include "Compages/Core/Vector.hpp"

namespace compages::world
{

namespace camera_defaults
{
using namespace units::literals;
inline constexpr units::angle::degree_t kFov = 60.0_deg;
} // namespace camera_defaults

// ****************************************************************************
//! \brief How a Camera looks. Not where it sits.
//!
//! A Camera is a component attached to an EntityId of the World. The place from
//! which it looks is the EntityId's own transform, exactly like any other
//! spatial object: parent it to a rig, and it rides that rig. That is why the
//! Camera has no eye/target/up: those belong to a controller (an orbit
//! controller, a follow-target behaviour) which writes the transform.
//!
//! Perspective is what a game needs; orthographic is what a UI overlay or an
//! isometric view needs. The three parameters below cover both:
//! - \c fov is the vertical field of view for perspective, ignored otherwise;
//! - \c ortho_half_height is the half-height, in world units, of the
//!   orthographic view volume, ignored otherwise;
//! - \c near_plane and \c far_plane are the visible depth range, in both
//!   modes.
//! - \c viewport is the normalised rectangle of the target this camera
//!   draws into. The Extractor turns it into pixels on the CameraFrame.
//!
//! \code
//! cam.set(compages::world::Camera{ .projection = compages::world::Camera::Projection::Orthographic,
//!                        .fov = 45.0_deg,
//!                        .near_plane = 0.5f });
//! \endcode
// ****************************************************************************
struct Camera
{
    // -------------------------------------------------------------------------
    //! \brief Which projection this camera uses.
    // -------------------------------------------------------------------------
    enum class Projection
    {
        //! \brief Perspective projection with a vertical field of view.
        Perspective,
        //! \brief Orthographic projection with a symmetric half-height.
        Orthographic,
    };

    // -------------------------------------------------------------------------
    //! \brief Where this camera draws, in normalised framebuffer coordinates.
    //!
    //! (0, 0, 1, 1) is the whole target. Split views set two cameras to
    //! (0, 0, 0.5, 1) and (0.5, 0, 0.5, 1). The Extractor turns this into a
    //! pixel rectangle on the \c CameraFrame.
    //!
    //! \code
    //! compages::world::Camera::Viewport leftHalf{ .width = 0.5f };
    //! \endcode
    // -------------------------------------------------------------------------
    struct Viewport
    {
        //! \brief Left edge in normalised coordinates (0 = left of the target).
        float x = 0.0f;
        //! \brief Bottom edge in normalised coordinates (0 = bottom of the target).
        float y = 0.0f;
        //! \brief Horizontal extent; \c 1.0f spans the full target width.
        float width = 1.0f;
        //! \brief Vertical extent; \c 1.0f spans the full target height.
        float height = 1.0f;
    };

    //! \brief Perspective or orthographic lens mode.
    Projection projection = Projection::Perspective;
    //! \brief Vertical field of view (perspective only).
    units::angle::degree_t fov = camera_defaults::kFov;
    //! \brief Half-height of the orthographic volume in world units (orthographic only).
    float ortho_half_height = 1.0f;
    //! \brief Near clipping plane distance along the view axis.
    float near_plane = 0.1f;
    //! \brief Far clipping plane distance along the view axis.
    float far_plane = 1000.0f;
    //! \brief Normalised sub-rectangle of the render target for this camera.
    Viewport viewport{};
};

} // namespace compages::world
