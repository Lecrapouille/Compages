// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// *****************************************************************************
//! \file 4×4 transform helpers (translation, scale, rotation, projections,
//! look-at). Matrices follow the **Scilab / column-vector** convention:
//! \c y = M * x and \c translate(M,t) returns \c M * T so that \c T is applied
//! to the point before \c M (same as \c (M * T) * x = M * (T * x)).
//!
//! OpenGL column-major layout is **not** baked in here: transpose once at GPU
//! upload (\c glUniformMatrix4fv(..., GL_TRUE, ...)) or call \c transpose().
//! See also \c doc/MathMatrices.md in the Robotik tree.
//!
//! \note Adapted from GLM:
//!   https://github.com/g-truc/glm/blob/master/glm/ext/matrix_transform.inl
//! \see https://learnopengl.com/Getting-started/Transformations
// *****************************************************************************

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <cassert>
#include <cmath>
#include <limits>

namespace compages::core
{

//! \brief Post-multiply by a translation: \c M * T ( \c T applied first on \c x
//! ).
//!
//! \param[in] M matrix already in the chain.
//! \param[in] t translation (Tx, Ty, Tz).
//! \return \c M * T.
//!
//! \code
//! T = | 1  0  0  Tx |
//!     | 0  1  0  Ty |
//!     | 0  0  1  Tz |
//!     | 0  0  0  1  |
//!
//! (M * T)(i,j) = M(i,j)           for j in {0,1,2}
//! (M * T)(i,3) = M(i,0)*Tx + M(i,1)*Ty + M(i,2)*Tz + M(i,3)
//! \endcode
//!
//! \note Implemented by updating column 3 only (same result as
//!   \c M * translationMatrix(t), without a full 4×4 product).
template <typename T>
Matrix<T, 4u, 4u> translate(Matrix<T, 4u, 4u> const& M, Vector<T, 3u> const& t)
{
    Matrix<T, 4u, 4u> O(M);

    O(0, 3) = M(0, 0) * t.x + M(0, 1) * t.y + M(0, 2) * t.z + M(0, 3);
    O(1, 3) = M(1, 0) * t.x + M(1, 1) * t.y + M(1, 2) * t.z + M(1, 3);
    O(2, 3) = M(2, 0) * t.x + M(2, 1) * t.y + M(2, 2) * t.z + M(2, 3);
    O(3, 3) = M(3, 0) * t.x + M(3, 1) * t.y + M(3, 2) * t.z + M(3, 3);

    return O;
}

//! \brief Post-multiply by an axis-aligned scale: \c M * S.
//!
//! \param[in] M matrix already in the chain.
//! \param[in] s scale (Sx, Sy, Sz).
//! \return \c M * S.
//!
//! \code
//! S = | Sx  0   0   0 |
//!     | 0   Sy  0   0 |
//!     | 0   0   Sz  0 |
//!     | 0   0   0   1 |
//!
//! (M * S)(i,0) = M(i,0)*Sx,  (M * S)(i,1) = M(i,1)*Sy,
//! (M * S)(i,2) = M(i,2)*Sz,  (M * S)(i,3) = M(i,3)
//! \endcode
//!
//! \note Column scaling only (specialized, not a full 4×4 multiply).
template <typename T>
Matrix<T, 4u, 4u> scale(Matrix<T, 4u, 4u> const& M, Vector<T, 3u> const& s)
{
    Matrix<T, 4u, 4u> O;

    O(0, 0) = M(0, 0) * s.x;
    O(0, 1) = M(0, 1) * s.y;
    O(0, 2) = M(0, 2) * s.z;
    O(0, 3) = M(0, 3);
    O(1, 0) = M(1, 0) * s.x;
    O(1, 1) = M(1, 1) * s.y;
    O(1, 2) = M(1, 2) * s.z;
    O(1, 3) = M(1, 3);
    O(2, 0) = M(2, 0) * s.x;
    O(2, 1) = M(2, 1) * s.y;
    O(2, 2) = M(2, 2) * s.z;
    O(2, 3) = M(2, 3);
    O(3, 0) = M(3, 0) * s.x;
    O(3, 1) = M(3, 1) * s.y;
    O(3, 2) = M(3, 2) * s.z;
    O(3, 3) = M(3, 3);

    return O;
}

//! \brief Post-multiply by an axis-angle rotation: \c M * R.
//!
//! \param[in] M matrix already in the chain.
//! \param[in] angle rotation in radians.
//! \param[in] r axis (normalized inside the function).
//! \return \c M * R.
//!
//! \code
//! R = | R00  R01  R02  0 |     with R the 3×3 Rodrigues matrix around
//!     | R10  R11  R12  0 |     unit axis (Rx,Ry,Rz), c = cos(angle),
//!     | R20  R21  R22  0 |     s = sin(angle):
//!     |  0    0    0   1 |
//!
//! R00 = c + Rx²(1-c)       R01 = Rx*Ry(1-c) - Rz*s   R02 = Rx*Rz(1-c) + Ry*s
//! R10 = Ry*Rx(1-c) + Rz*s  R11 = c + Ry²(1-c)        R12 = Ry*Rz(1-c) - Rx*s
//! R20 = Rz*Rx(1-c) - Ry*s  R21 = Rz*Ry(1-c) + Rx*s   R22 = c + Rz²(1-c)
//! \endcode
//!
//! A positive angle turns counter-clockwise when looking from the tip of the
//! axis toward the origin (right-hand rule).
template <typename T>
Matrix<T, 4u, 4u> rotate(Matrix<T, 4u, 4u> const& M,
                         units::angle::radian_t const angle,
                         Vector<T, 3u> const& r)
{
    T const c = units::math::cos(angle);
    T const s = units::math::sin(angle);

    Vector<T, 3u> const axis(vector::normalize(r));
    Vector<T, 3u> const temp((one<T>() - c) * axis);

    Matrix<T, 4u, 4u> rot(matrix::Type::Identity);

    rot(0, 0) = c + temp.x * axis.x;
    rot(0, 1) = temp.x * axis.y - s * axis.z;
    rot(0, 2) = temp.x * axis.z + s * axis.y;

    rot(1, 0) = temp.y * axis.x + s * axis.z;
    rot(1, 1) = c + temp.y * axis.y;
    rot(1, 2) = temp.y * axis.z - s * axis.x;

    rot(2, 0) = temp.z * axis.x - s * axis.y;
    rot(2, 1) = temp.z * axis.y + s * axis.x;
    rot(2, 2) = c + temp.z * axis.z;

    return M * rot;
}

//! \brief Orthographic projection (\c p_ndc = ortho * p_view).
//!
//! \code
//! ortho = | 2/(r-l)    0          0              -(r+l)/(r-l) |
//!         | 0          2/(t-b)    0              -(t+b)/(t-b) |
//!         | 0          0          -2/(f-n)       -(f+n)/(f-n) |
//!         | 0          0          0               1           |
//! \endcode
template <typename T>
Matrix<T, 4u, 4u> ortho(T const left,
                        T const right,
                        T const bottom,
                        T const top,
                        T const near,
                        T const far)
{
    // Column-vector layout: p_ndc = ortho * p_view, camera local -Z. For
    // a point at z_view = -near we want z_ndc = -1, at z_view = -far we want
    // z_ndc = +1. That linear map is z_ndc = -2/(f-n) * z_view - (f+n)/(f-n),
    // which is why the z coefficient below is negative even though the x and
    // y coefficients are positive. A positive value here is the OpenGL
    // convention for a *left-handed* view, and would clip everything against
    // the near plane in the right-handed convention used everywhere else.
    return { //
             T(2) / (right - left),
             zero<T>(),
             zero<T>(),
             -(right + left) / (right - left),
             //
             zero<T>(),
             T(2) / (top - bottom),
             zero<T>(),
             -(top + bottom) / (top - bottom),
             //
             zero<T>(),
             zero<T>(),
             -T(2) / (far - near),
             -(far + near) / (far - near),
             //
             zero<T>(),
             zero<T>(),
             zero<T>(),
             one<T>()
    };
}

//! \brief Replace gluPerspective(). Set the frustum to perspective mode.
//! \param[in] fovY Field of vision in radians in the y direction.
//! \param[in] aspect Aspect ratio of the viewport.
//! \param[in] zNear The near clipping distance.
//! \param[in] zFar The far clipping distance.
//! \return perspective matrix (\c p_ndc = perspective * p_view).
//!
//! \code
//! g = 1 / tan(fovY/2), n = zNear, f = zFar
//!
//! perspective = | g/aspect  0    0              0            |
//!               | 0         g    0              0            |
//!               | 0         0   -(f+n)/(f-n)   -2*f*n/(f-n)  |
//!               | 0         0   -1              0            |
//! \endcode
template <typename T>
Matrix<T, 4u, 4u> perspective(units::angle::radian_t const fovY,
                              T const aspect,
                              T const zNear,
                              T const zFar)
{
    assert(abs(aspect) > std::numeric_limits<T>::epsilon());

    T const tanHalfFovY = std::tan(fovY.to<T>() / T(2));

    return { //
             one<T>() / (aspect * tanHalfFovY),
             zero<T>(),
             zero<T>(),
             zero<T>(),

             //
             zero<T>(),
             one<T>() / (tanHalfFovY),
             zero<T>(),
             zero<T>(),

             //
             zero<T>(),
             zero<T>(),
             -(zFar + zNear) / (zFar - zNear),
             -(T(2) * zFar * zNear) / (zFar - zNear),

             //
             zero<T>(),
             zero<T>(),
             -one<T>(),
             zero<T>()
    };
}

//! \brief Build a look at view matrix based on the default handedness.
//!
//! \param[in] position Position of the camera
//! \param[in] target Position where the camera is looking at
//! \param[in] upwards Normalized up vector, how the camera is oriented.
//!   Typically (0, 0, 1)
//! \return view matrix \c V such that \c p_cam = V * p_world.
//!
//! With \c direction = normalize(target - position), \c right = normalize(
//! direction × up), \c up' = right × direction:
//!
//! \code
//! V = |  Rx   Ry   Rz   -(R·position) |
//!     |  Ux   Uy   Uz   -(U·position) |
//!     | -Dx  -Dy  -Dz    D·position   |
//!     |  0    0    0          1       |
//! \endcode
//!
//! (R, U, D) are the right, up and view-direction vectors: they are the rows
//! of the rotation block, so the camera looks down its local -Z axis.
template <typename T>
Matrix<T, 4u, 4u> lookAt(Vector<T, 3u> const& position,
                         Vector<T, 3u> const& target,
                         Vector<T, 3u> const& upwards)
{
    Vector<T, 3u> const direction(vector::normalize(target - position));
    Vector<T, 3u> const right(
        vector::normalize(vector::cross(direction, upwards)));
    Vector<T, 3u> const up(vector::cross(right, direction));

    return { //
             right.x,
             right.y,
             right.z,
             -vector::dot(right, position),

             //
             up.x,
             up.y,
             up.z,
             -vector::dot(up, position),

             //
             -direction.x,
             -direction.y,
             -direction.z,
             vector::dot(direction, position),

             //
             zero<T>(),
             zero<T>(),
             zero<T>(),
             one<T>()
    };
}

//! \brief
template <typename T>
Matrix<T, 3u, 3u> normalMatrix(Matrix<T, 4u, 4u> const& modelViewMatrix)
{
    return Matrix<T, 3u, 3u>(transpose(inverse(modelViewMatrix)));
}

//! \brief
template <typename T>
Matrix<T, 3u, 3u> normalMatrix(Matrix<T, 4u, 4u> const& modelMatrix,
                               Matrix<T, 4u, 4u> const& viewMatrix)
{
    return normalMatrix(viewMatrix * modelMatrix);
}

// --------------------------------------------------------------------------
//! \brief Translation part (column 3) of an affine 4x4 matrix.
// --------------------------------------------------------------------------
[[nodiscard]] inline Vector3f translation(Matrix44f const& p_matrix)
{
    return Vector3f(p_matrix(0, 3), p_matrix(1, 3), p_matrix(2, 3));
}

// --------------------------------------------------------------------------
//! \brief Apply \c p_matrix to a 3D point (\c w = 1), with perspective divide.
//! Uses the canonical product \c M * x (see \c Matrix::operator*).
// --------------------------------------------------------------------------
[[nodiscard]] inline Vector3f transformPoint(Matrix44f const& p_matrix,
                                             Vector3f const& p_point)
{
    Vector4f const h =
        p_matrix * Vector4f(p_point.x, p_point.y, p_point.z, 1.0f);
    const float w = (std::abs(h.w) < 1.0e-8f) ? 1.0f : h.w;
    return Vector3f(h.x / w, h.y / w, h.z / w);
}

} // namespace compages::core
