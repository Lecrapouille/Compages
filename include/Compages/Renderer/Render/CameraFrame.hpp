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

#include "Compages/Core/Frustum.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Ray.hpp"
#include "Compages/Core/Vector.hpp"

#include <cstdint>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Everything one frame of rendering needs from a Camera. Immutable.
//!
//! A CameraFrame is computed once per frame from a Camera component and the
//! World matrix of its EntityId, and handed to the renderer. Objects never ask
//! the Camera what it is doing: they receive this.
//!
//! The inverses are stored so a picker can unproject a pixel without walking
//! back to the World. \c screenRay() is the only operation that needs them.
// ****************************************************************************
struct CameraFrame
{
    //! \brief World-to-camera matrix.
    Matrix44f view{ compages::matrix::Identity };
    //! \brief Camera-to-clip matrix.
    Matrix44f projection{ compages::matrix::Identity };
    //! \brief \c view * \c projection, the row-vector composition this
    //! library uses: a point \c p is transformed as \c p * view * projection.
    Matrix44f view_projection{ compages::matrix::Identity };
    //! \brief Inverse of \c view, equal to the camera EntityId's world matrix.
    Matrix44f inverse_view{ compages::matrix::Identity };
    //! \brief Inverse of \c projection.
    Matrix44f inverse_projection{ compages::matrix::Identity };
    //! \brief Where the camera sits, in world space.
    Vector3f position{ 0.0f, 0.0f, 0.0f };
    //! \brief The six planes of the visible volume, for culling.
    Frustum frustum{};
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
    //! \c Frame::mouse.
    //! \param[in] p_width,p_height size of the viewport this frame was
    //! extracted for. Zero on either axis yields a ray looking along the
    //! camera forward, so a forgotten size does not produce a NaN.
    // ------------------------------------------------------------------------
    [[nodiscard]] Ray screenRay(float p_x,
                                float p_y,
                                std::uint32_t p_width,
                                std::uint32_t p_height) const;
};

} // namespace compages::renderer
