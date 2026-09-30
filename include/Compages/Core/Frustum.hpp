// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <algorithm>
#include <array>

namespace compages::core
{
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
            return compages::core::vector::dot(normal, p_point) + offset;
        }
    };

public:

    // ------------------------------------------------------------------------
    //! \brief Read the six planes out of a view-projection matrix.
    //!
    //! \param[in] p_vp combined matrix \c projection * \c view (column-vector
    //! clip = \c p_vp * \c p_world).
    //! \return A frustum whose six planes are normalized.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Frustum fromViewProjection(Matrix44f const& p_vp)
    {
        auto make = [](Vector4f const& p_row)
        {
            Plane plane;
            Vector3f const xyz(p_row.x, p_row.y, p_row.z);
            float const length = compages::core::vector::norm(xyz);
            float const inv = (length < 1.0e-8f) ? 1.0f : (1.0f / length);
            plane.normal = xyz * inv;
            plane.offset = p_row.w * inv;
            return plane;
        };

        Frustum frustum;
        frustum.m_planes[0] = make(p_vp[3] + p_vp[0]); // left
        frustum.m_planes[1] = make(p_vp[3] - p_vp[0]); // right
        frustum.m_planes[2] = make(p_vp[3] + p_vp[1]); // bottom
        frustum.m_planes[3] = make(p_vp[3] - p_vp[1]); // top
        frustum.m_planes[4] = make(p_vp[3] + p_vp[2]); // near
        frustum.m_planes[5] = make(p_vp[3] - p_vp[2]); // far
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

} // namespace compages::core

