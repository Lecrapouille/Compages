// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
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
    //! \brief Whether this camera draws the helps for the person at the
    //! screen: grids, axes, bounds, debug lines, and the meshes the renderer
    //! marks as helpers. False for a camera that stands for a sensor (the
    //! eye of a robot): it sees the world, not the tools around it.
    bool sees_helpers = true;
    //! \brief Where the axis of the lens crosses the picture, in clip units
    //! (-1 to 1, y up): (0, 0) is the centre. A calibrated camera has it off
    //! the centre; calibrate() sets it from \c cx and \c cy. Perspective
    //! only.
    compages::Vector2f lens_shift{ 0.0f, 0.0f };
};

// ****************************************************************************
//! \brief The intrinsic parameters of a real camera, in pixels, as a
//! calibration gives them (OpenCV, ROS \c camera_info): focal lengths and
//! principal point, the origin at the top left corner of the picture.
// ****************************************************************************
struct Intrinsics
{
    //! \brief Focal length along x, in pixels.
    float fx = 0.0f;
    //! \brief Focal length along y, in pixels.
    float fy = 0.0f;
    //! \brief Principal point: column, from the left edge.
    float cx = 0.0f;
    //! \brief Principal point: row, from the top edge.
    float cy = 0.0f;
    //! \brief Size of the picture the parameters were measured for.
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
};

// ****************************************************************************
//! \brief Give a perspective lens the intrinsic parameters of a calibrated
//! camera, so that a simulated picture matches the real one pixel for pixel.
//!
//! Sets \c fov from \c fy and the height, and \c lens_shift from \c cx and
//! \c cy. Draw into a target of \c width x \c height pixels: the parameters
//! hold for that size only. Lens distortion is not modelled: it belongs to the
//! sensor model of the host.
//!
//! \code
//! auto& lens = eye.get<compages::world::Camera>();
//! COMPAGES_TRY(compages::world::calibrate(lens, { .fx = 615.0f, .fy = 615.0f,
//!                                                 .cx = 322.0f, .cy = 238.0f,
//!                                                 .width = 640u, .height = 480u }));
//! \endcode
//!
//! \return an error, the lens left as it was, for a size or a focal length of
//! zero, or for pixels that are not square (\c fx and \c fy more than 1 %
//! apart): one field of view cannot give two focal lengths.
// ****************************************************************************
[[nodiscard]] inline Status calibrate(Camera& p_lens, Intrinsics const& p_intrinsics)
{
    if ((p_intrinsics.width == 0u) || (p_intrinsics.height == 0u))
    {
        return failure("calibrate(): the picture of the intrinsics has no size");
    }
    if ((p_intrinsics.fx <= 0.0f) || (p_intrinsics.fy <= 0.0f))
    {
        return failure("calibrate(): a focal length must be more than zero pixels");
    }
    const float apart = (p_intrinsics.fx > p_intrinsics.fy) ? (p_intrinsics.fx - p_intrinsics.fy)
                                                            : (p_intrinsics.fy - p_intrinsics.fx);
    if (apart > 0.01f * p_intrinsics.fy)
    {
        return failure("calibrate(): fx and fy differ by more than 1 %: pixels that are not square are not "
                       "drawn");
    }
    const float w = float(p_intrinsics.width);
    const float h = float(p_intrinsics.height);
    p_lens.projection = Camera::Projection::Perspective;
    p_lens.fov = units::math::atan(units::dimensionless::scalar_t(h / (2.0f * p_intrinsics.fy))) * 2.0f;
    p_lens.lens_shift = compages::Vector2f((2.0f * p_intrinsics.cx / w) - 1.0f, 1.0f - (2.0f * p_intrinsics.cy / h));
    return success();
}

} // namespace compages::world
