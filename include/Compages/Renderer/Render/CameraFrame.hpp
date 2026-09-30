// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Frustum.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Ray.hpp"
#include "Compages/Core/Vector.hpp"

#include <cstdint>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{



// ****************************************************************************
//! \brief Everything one frame of rendering needs from a compages::world::Camera. Immutable.
//!
//! A CameraFrame is computed once per frame from a compages::world::Camera component and the
//! compages::world::World matrix of its compages::world::EntityId, and handed to the renderer. Objects never ask
//! the compages::world::Camera what it is doing: they receive this.
//!
//! The inverses are stored so a picker can unproject a pixel without walking
//! back to the compages::world::World. \c screenRay() is the only operation that needs them.
// ****************************************************************************
struct CameraFrame
{
    //! \brief compages::world::World-to-camera matrix.
    compages::core::Matrix44f view{ compages::core::matrix::Identity };
    //! \brief compages::world::Camera-to-clip matrix.
    compages::core::Matrix44f projection{ compages::core::matrix::Identity };
    //! \brief \c projection * \c view; clip = \c view_projection * \c p_world.
    compages::core::Matrix44f view_projection{ compages::core::matrix::Identity };
    //! \brief Inverse of \c view, equal to the camera compages::world::EntityId's world matrix.
    compages::core::Matrix44f inverse_view{ compages::core::matrix::Identity };
    //! \brief Inverse of \c projection.
    compages::core::Matrix44f inverse_projection{ compages::core::matrix::Identity };
    //! \brief Where the camera sits, in world space.
    compages::core::Vector3f position{ 0.0f, 0.0f, 0.0f };
    //! \brief The six planes of the visible volume, for culling.
    compages::core::Frustum frustum{};
    //! \brief Pixel rectangle this frame was extracted for, origin bottom-left.
    float viewport_x = 0.0f;
    float viewport_y = 0.0f;
    std::uint32_t viewport_width = 0u;
    std::uint32_t viewport_height = 0u;

    // ------------------------------------------------------------------------
    //! \brief A world-space ray through a pixel, counting from the bottom
    //! left the way the graphics API counts.
    //!
    //! \param[in] p_x,p_y pixel coordinates, in the same space as
    //! \c compages::core::Frame::mouse.
    //! \param[in] p_width,p_height size of the viewport this frame was
    //! extracted for. Zero on either axis yields a ray looking along the
    //! camera forward, so a forgotten size does not produce a NaN.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::core::Ray screenRay(float p_x,
                                float p_y,
                                std::uint32_t p_width,
                                std::uint32_t p_height) const;
};

} // namespace compages::renderer
