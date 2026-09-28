//=====================================================================
// Compages: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#pragma once


// *****************************************************************************
//! \file Collection of Matrix transformation routines adaptated from the OpenGL
//! Mathematic library (GLM).
//!
//! \warning Note that all matrices are transposed to be directly usable by
//!   OpenGL (since OpenGL matrices are column-major. Beware of option passed
//!   to glUniformMatrix4fv that should be set to GL_FALSE). As consequence:
//!   transpose(A * B) = transpose(B) * transpose(A). But to confuse more things
//!   OpenGL convention is still M * x.
//!
//! \note The original code can be found here:
//!   https://github.com/g-truc/glm/blob/master/glm/ext/matrix_transform.inl
//! \see https://learnopengl.com/Getting-started/Transformations
//! \see https://antongerdelan.net/opengl/raycasting.html
// *****************************************************************************

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <cmath>

namespace compages::matrix
{
//--------------------------------------------------------------------------
//! \brief Build a translation 4 * 4 matrix created from a vector of 3
//! components.
//!
//! \param[in] M Input matrix multiplied by this translation matrix.
//! \param[in] t Coordinates of a translation vector.
//! \return A transposed matrix.
//!
//! \code
//! If \c M is the identity matrix, the output matrix O will be:
//!                | 1 0 0 Tx |
//!                | 0 1 0 Ty |
//! O = transpose( | 0 0 1 Tz | )
//!                | 0 0 0 1  |
//! \endcode
//--------------------------------------------------------------------------

template <typename T>
Matrix<T, 4u, 4u> translate(Matrix<T, 4u, 4u> const& M, Vector<T, 3u> const& t)
{
    Matrix<T, 4u, 4u> O(M);

    O[3] = M[0] * t.x + M[1] * t.y + M[2] * t.z + M[3];

    return O;
}

//--------------------------------------------------------------------------
//! \brief Build a scale 4 * 4 matrix created from 3 scalars.
//!
//! \param[in] M Input matrix multiplied by this scale matrix.
//! \param[in] s Ratio of scaling for each axis.
//! \return A transposed matrix.
//!
//! \code
//! If \c M is the identity matrix, the output matrix O will be:
//!                 | Sx 0  0  0 |
//!                 | 0  Sy 0  0 |
//! O =  transpose( | 0  0  Sz 0 | )
//!                 | 0  0  0  1 |
//! \endcode
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 4u, 4u> scale(Matrix<T, 4u, 4u> const& M, Vector<T, 3u> const& s)
{
    Matrix<T, 4u, 4u> O;

    O[0] = M[0] * s.x;
    O[1] = M[1] * s.y;
    O[2] = M[2] * s.z;
    O[3] = M[3];

    return O;
}

//--------------------------------------------------------------------------
//! \brief Build a rotation 4 * 4 matrix created from an axis vector and an
//! angle. With positive angle the rotation direction is clockwise due to the
//! fact that OpenGL matrices are transposed.
//!
//! \param[in] M Input matrix multiplied by this rotation matrix.
//! \param[in] angle Rotation angle expressed in radians.
//! \param[in] r Rotation axis that will be normalized by this function.
//! \return A transposed matrix.
//!
//! \code
//! If \c M is the identity matrix, the output matrix O will be:
//!
//!                | RxRx(1-c)+c      RxRy(1-c)-Rz.s   RxRz(1-c)+Ry.s   0 |
//!                | RyRx(1-c)+Rz.s   RyRy(1-c)+c      RyRz(1-c)-Rx.s   0 |
//! O = transpose( | RzRx(1-c)-Ry.s   RzRy(1-c)+Rx.s   RzRz(1-c)+c      0 | )
//!                | 0                0                0                1 |
//!
//! Where: c = cosinus and s = sinus.
//! \endcode
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 4u, 4u> rotate(Matrix<T, 4u, 4u> const& M,
                         units::angle::radian_t const angle,
                         Vector<T, 3u> const& r)
{
    T const c = units::math::cos(angle);
    T const s = units::math::sin(angle);

    Vector<T, 3u> axis(compages::vector::normalize(r));
    Vector<T, 3u> temp((compages::maths::one<T>() - c) * axis);
    Matrix<T, 4u, 4u> rotate;

    rotate[0][0] = c + temp[0] * axis[0];
    rotate[0][1] = temp[0] * axis[1] + s * axis[2];
    rotate[0][2] = temp[0] * axis[2] - s * axis[1];

    rotate[1][0] = temp[1] * axis[0] - s * axis[2];
    rotate[1][1] = c + temp[1] * axis[1];
    rotate[1][2] = temp[1] * axis[2] + s * axis[0];

    rotate[2][0] = temp[2] * axis[0] + s * axis[1];
    rotate[2][1] = temp[2] * axis[1] - s * axis[0];
    rotate[2][2] = c + temp[2] * axis[2];

    Matrix<T, 4u, 4u> O;
    O[0] = M[0] * rotate[0][0] + M[1] * rotate[0][1] + M[2] * rotate[0][2];
    O[1] = M[0] * rotate[1][0] + M[1] * rotate[1][1] + M[2] * rotate[1][2];
    O[2] = M[0] * rotate[2][0] + M[1] * rotate[2][1] + M[2] * rotate[2][2];
    O[3] = M[3];

    return O;
}

//--------------------------------------------------------------------------
//! \brief Return the orthogonal matrix.
//! \return the transposed orthogonal matrix.
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 4u, 4u> ortho(T const left,
                        T const right,
                        T const bottom,
                        T const top,
                        T const near,
                        T const far)
{
    // Row-vector layout, consistent with compages::matrix::perspective:
    // p_ndc = p_view * ortho, with the camera looking down its local -Z. For
    // a point at z_view = -near we want z_ndc = -1, at z_view = -far we want
    // z_ndc = +1. That linear map is z_ndc = -2/(f-n) * z_view - (f+n)/(f-n),
    // which is why the z coefficient below is negative even though the x and
    // y coefficients are positive. A positive value here is the OpenGL
    // convention for a *left-handed* view, and would clip everything against
    // the near plane in the right-handed convention used everywhere else.
    return { //
             T(2) / (right - left),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             //
             compages::maths::zero<T>(),
             T(2) / (top - bottom),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             //
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             -T(2) / (far - near),
             compages::maths::zero<T>(),
             //
             -(right + left) / (right - left),
             -(top + bottom) / (top - bottom),
             -(far + near) / (far - near),
             compages::maths::one<T>()
    };
}

//--------------------------------------------------------------------------
//! \brief Replace gluPerspective(). Set the frustum to perspective mode.
//! \param[in] fovY Field of vision in radians in the y direction.
//! \param[in] aspect Aspect ratio of the viewport.
//! \param[in] zNear The near clipping distance.
//! \param[in] zFar The far clipping distance.
//! \return the transposed perspective matrix.
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 4u, 4u> perspective(units::angle::radian_t const fovY,
                              T const aspect,
                              T const zNear,
                              T const zFar)
{
    assert(compages::maths::abs(aspect - std::numeric_limits<T>::epsilon()) >
           static_cast<T>(0));

    T const tanHalfFovY = std::tan(fovY.to<T>() / T(2));

    return { //
             compages::maths::one<T>() / (aspect * tanHalfFovY),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),

             //
             compages::maths::zero<T>(),
             compages::maths::one<T>() / (tanHalfFovY),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),

             //
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             -(zFar + zNear) / (zFar - zNear),
             -compages::maths::one<T>(),

             //
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             -(T(2) * zFar * zNear) / (zFar - zNear),
             compages::maths::zero<T>(),

             //
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             compages::maths::zero<T>(),
             compages::maths::one<T>()
    };
}

//--------------------------------------------------------------------------
//! \brief Build a look at view matrix based on the default handedness.
//!
//! \param[in] position Position of the camera
//! \param[in] target Position where the camera is looking at
//! \param[in] upwards Normalized up vector, how the camera is oriented.
//!   Typically (0, 0, 1)
//! \return A transposed matrix.
//!
//! \code
//!                     | Rx Ry Rz 0 |   | 1 0 0 -Px |
//! LookAt = transpose( | Ux Uy Uz 0 | x | 0 1 0 -Py | )
//!                     | Dx Dy Dz 0 |   | 0 0 1 -Pz |
//!                     |  0  0  0 1 |   | 0 0 0 1   |
//!
//! Where:
//!   - U is the up vector,
//!   - D is the direction vector,
//!   - R is the right vector (= cross product between U and D).
//!   - P is the camera's position vector.
//! \endcode
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 4u, 4u> lookAt(Vector<T, 3u> const& position,
                         Vector<T, 3u> const& target,
                         Vector<T, 3u> const& upwards)
{
    Vector<T, 3u> const direction(compages::vector::normalize(target - position));
    Vector<T, 3u> const right(
        compages::vector::normalize(compages::vector::cross(direction, upwards)));
    Vector<T, 3u> const up(compages::vector::cross(right, direction));

    return { //
             right.x,
             up.x,
             -direction.x,
             compages::maths::zero<T>(),

             //
             right.y,
             up.y,
             -direction.y,
             compages::maths::zero<T>(),

             //
             right.z,
             up.z,
             -direction.z,
             compages::maths::zero<T>(),

             //
             -(compages::vector::dot(right, position)),
             -(compages::vector::dot(up, position)),
             compages::vector::dot(direction, position),
             compages::maths::one<T>()
    };
}

//--------------------------------------------------------------------------
//! \brief
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 3u, 3u> normalMatrix(Matrix<T, 4u, 4u> const& modelViewMatrix)
{
    return Matrix<T, 3u, 3u>(transpose(inverse(modelViewMatrix)));
}

//--------------------------------------------------------------------------
//! \brief
//--------------------------------------------------------------------------
template <typename T>
Matrix<T, 3u, 3u> normalMatrix(Matrix<T, 4u, 4u> const& modelMatrix,
                               Matrix<T, 4u, 4u> const& viewMatrix)
{
    return normalMatrix(modelMatrix * viewMatrix);
}

// --------------------------------------------------------------------------
//! \brief Apply a 4x4 the way this library stores it: rows of the CPU array
//! are the columns the shader multiplies with, so a point is a weighted sum
//! of those rows. Same convention as \c AABB::transformed.
// --------------------------------------------------------------------------
[[nodiscard]] inline Vector3f transformPoint(Matrix44f const& p_matrix,
                                             Vector3f const& p_point)
{
    Vector4f const h = (p_matrix[0] * p_point.x) + (p_matrix[1] * p_point.y) +
                       (p_matrix[2] * p_point.z) + p_matrix[3];
    const float w = (std::abs(h.w) < 1.0e-8f) ? 1.0f : h.w;
    return Vector3f(h.x / w, h.y / w, h.z / w);
}

} // namespace compages::matrix

