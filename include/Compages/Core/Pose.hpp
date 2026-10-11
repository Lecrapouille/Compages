// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Vector.hpp"

namespace compages::core
{

template <typename T>
Vector<T, 3u> rotate(Quat<T> const& p_rotation, Vector<T, 3u> const& p_point)
{
    Vector<T, 3u> const q(p_rotation.x, p_rotation.y, p_rotation.z);
    Vector<T, 3u> const uv = q % p_point;
    Vector<T, 3u> const uuv = q % uv;
    return p_point + uv * (T(2) * p_rotation.w) + uuv * T(2);
}

// ****************************************************************************
//! \brief Rigid transform: placement of a frame expressed in a parent frame.
//!
//! Unlike LocalTransform it has no scale, so it composes and inverts exactly:
//! \c a * b chains placements (b expressed in a) and \c pose * point maps a
//! point from the child frame to the parent frame.
//!
//! \code
//! compages::core::Posed const camera{ { 0.05, 0.0, 0.0 }, Quatd() };
//! compages::core::Posed const world_camera = world_link * camera;
//! compages::core::Vector3g const p = world_camera * point_in_camera;
//! \endcode
// ****************************************************************************
template <typename T>
struct Pose
{
    //! \brief Origin of the frame in the parent frame.
    Vector<T, 3u> position{ T(0), T(0), T(0) };
    //! \brief Orientation of the frame in the parent frame.
    Quat<T> rotation{};

    [[nodiscard]] Pose operator*(Pose const& p_child) const
    {
        return { position + rotate(rotation, p_child.position),
                 rotation * p_child.rotation };
    }

    [[nodiscard]] Vector<T, 3u> operator*(Vector<T, 3u> const& p_point) const
    {
        return position + rotate(rotation, p_point);
    }

    [[nodiscard]] Pose inverse() const
    {
        Quat<T> const inverse = rotation.conjugate();
        return { rotate(inverse, -position), inverse };
    }
};

using Posef = Pose<float>;
using Posed = Pose<double>;

} // namespace compages::core
