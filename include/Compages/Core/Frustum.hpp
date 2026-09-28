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

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <algorithm>
#include <array>

// ****************************************************************************
//! \file
//! \brief The six planes of a view, for asking whether a box is visible.
//!
//! Built from a view-projection matrix, so any camera that can write that
//! matrix can be culled against. The renderer does not need to know how the
//! camera was aimed.
// ****************************************************************************

// ****************************************************************************
//! \brief The volume a camera can see, as six planes.
//!
//! \code
//! Matrix44f const vp = camera.view() * camera.projection();
//! Frustum const frustum = Frustum::fromViewProjection(vp);
//! if (frustum.contains(mesh_bounds))
//! {
//!     // draw or keep in the render list
//! }
//! \endcode
// ****************************************************************************
class Frustum
{
    // ------------------------------------------------------------------------
    //! \brief One side plane: \f$ \mathbf{n} \cdot \mathbf{x} + d = 0 \f$,
    //! with \f$ \lVert \mathbf{n} \rVert = 1 \f$.
    //!
    //! The visible half-space is \f$ \mathbf{n} \cdot \mathbf{x} + d \ge 0 \f$.
    // ------------------------------------------------------------------------
    struct Plane
    {
        Vector3f normal{ 0.0f, 0.0f, 1.0f };
        float offset = 0.0f;

        // --------------------------------------------------------------------
        //! \brief Signed distance from a point to the plane.
        //! \param[in] p_point position to test.
        //! \return \f$ \mathbf{n} \cdot \mathbf{p} + d \f$. Non-negative
        //! values lie in the half-space treated as visible for culling.
        // --------------------------------------------------------------------
        [[nodiscard]] float signedDistance(Vector3f const& p_point) const
        {
            return compages::vector::dot(normal, p_point) + offset;
        }
    };

public:

    // ------------------------------------------------------------------------
    //! \brief Read the six planes out of a view-projection matrix.
    //!
    //! This library uses the row-vector convention: a point p is a 1x4 row
    //! and applying view then projection to p is p * view * projection. The
    //! caller therefore passes \c view * \c projection, not the other way
    //! round. The extraction uses the standard Gribb/Hartmann form
    //! (row 3 +/- row i of the combined matrix).
    //! \param[in] p_vp combined view and projection matrix (\c view * \c
    //! projection).
    //! \return A frustum whose six planes are normalized.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Frustum fromViewProjection(Matrix44f const& p_vp)
    {
        auto make = [](Vector4f const& p_row)
        {
            Plane plane;
            Vector3f const xyz(p_row.x, p_row.y, p_row.z);
            float const length = compages::vector::norm(xyz);
            float const inv = (length < 1.0e-8f) ? 1.0f : (1.0f / length);
            plane.normal = xyz * inv;
            plane.offset = p_row.w * inv;
            return plane;
        };

        // Gribb/Hartmann wants the rows of the matrix the shader multiplies
        // with. This library stores those as columns: CPU row i is shader
        // column i, so shader row r is (M[0][r], M[1][r], M[2][r], M[3][r]).
        auto shaderRow = [&p_vp](std::size_t p_row)
        {
            return Vector4f(
                p_vp[0][p_row], p_vp[1][p_row], p_vp[2][p_row], p_vp[3][p_row]);
        };

        Frustum frustum;
        frustum.m_planes[0] = make(shaderRow(3) + shaderRow(0)); // left
        frustum.m_planes[1] = make(shaderRow(3) - shaderRow(0)); // right
        frustum.m_planes[2] = make(shaderRow(3) + shaderRow(1)); // bottom
        frustum.m_planes[3] = make(shaderRow(3) - shaderRow(1)); // top
        frustum.m_planes[4] = make(shaderRow(3) + shaderRow(2)); // near
        frustum.m_planes[5] = make(shaderRow(3) - shaderRow(2)); // far
        return frustum;
    }

    // ------------------------------------------------------------------------
    //! \brief Is any part of this box in front of every plane?
    //!
    //! An empty box is not visible: there is nothing to draw.
    //! \param[in] p_box axis-aligned bounds in the same space as the matrix
    //! passed to \ref fromViewProjection.
    //! \return \c true if \c p_box intersects the frustum; \c false if it is
    //! empty or lies entirely outside.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool contains(AABB const& p_box) const
    {
        if (p_box.empty())
        {
            return false;
        }

        return std::ranges::none_of(
            m_planes,
            [&p_box](Plane const& p_plane)
            {
                // The corner furthest along the plane normal. If that one is
                // behind the plane, the whole box is.
                Vector3f const corner(
                    (p_plane.normal.x >= 0.0f) ? p_box.max.x : p_box.min.x,
                    (p_plane.normal.y >= 0.0f) ? p_box.max.y : p_box.min.y,
                    (p_plane.normal.z >= 0.0f) ? p_box.max.z : p_box.min.z);
                return p_plane.signedDistance(corner) < 0.0f;
            });
    }

private:

    std::array<Plane, 6u> m_planes{};
};
