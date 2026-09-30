// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

/*
 * Quaternion.hpp
 *
 *  Copyright (c) 2014 G. Cross and C. Qu. MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *        Created on: 12/13/2013
 *  Contributors: G. Cross gcross [dot] code [at] icloud.com
 *                C. Qu quchao [at] seas.upenn.edu
 */

// *****************************************************************************
// Modified code by Quentin Quadrat <lecrapouille@gmail.com>.
// See original code
// https://github.com/gareth-cross/quat/blob/master/include/quaternion.hpp */
// *****************************************************************************

#pragma once

#include "Matrix.hpp"
#include <cmath>
#include <limits>
#include <type_traits>

namespace compages::core
{

// *****************************************************************************
//! \class Quat
//! \brief Template representation of a quaternion q = (a, (b,c,d)) in the axis
//! (i,j,k) where i² = j² = k² = ijk = -1. We write q = a + bi + cj + dk.
//! In some code the quaternion is also defined as q = (w, (x,y,z)).
//!
//! where:
//!   - \c a (aka w) is the T component.
//!   - \c b (aka x) is the first complex dimension (i-axis).
//!   - \c c (aka y) is the second complex dimension (j-axis).
//!   - \c d (aka z) is the third complex dimension (k-axis).
//!
//! Supports multiplication, addition, scaling and integration.
// *****************************************************************************
template <typename T>
class Quat
{
    static_assert(std::is_fundamental<T>::value &&
                      !std::numeric_limits<T>::is_integer,
                  "T must be a floating point type");

public:

    //--------------------------------------------------------------------------
    //! \brief Create a quaternion with null rotation (1, (0,0,0))
    //--------------------------------------------------------------------------
    Quat() noexcept
    {
        data[0] = T(1);
        data[1] = data[2] = data[3] = T(0);
    }

    //--------------------------------------------------------------------------
    //! \brief Construct a quaternion a + bi + cj + dk (aka w + xi + yj + zk)
    //! \param[in] a (aka w) T parameter
    //! \param[in] b (aka x) Complex parameters complex i-axis
    //! \param[in] c (aka y) Complex parameters complex j-axis
    //! \param[in] d (aka z) Complex parameters complex k-axis
    //--------------------------------------------------------------------------
    Quat(T a_, T b_, T c_, T d_)
    {
        data[0] = a_;
        data[1] = b_;
        data[2] = c_;
        data[3] = d_;
    }

    //--------------------------------------------------------------------------
    //! \brief Construct a quaternion a + bi + cj + dk (aka w + xi + yj + zk)
    //! \param[in] a (aka w) T parameter
    //! \param[in] bcd (aka xyz) Complex parameters (i, j, k)
    //! \note do not confuse with angleAxis() which builds a quaternion from a
    //!   given angle and a given rotation axis.
    //--------------------------------------------------------------------------
    Quat(T a_, Vector<T, 3u> const& bcd_)
    {
        data[0] = a_;
        data[1] = bcd_[0];
        data[2] = bcd_[1];
        data[3] = bcd_[2];
    }

    //--------------------------------------------------------------------------
    //! \brief Construct an unit quaternion a + bi + cj + dk where the T
    //! component \c a is not given but rebuild. Used for reading md5mesh files.
    //! \param[in] b (aka x)  Complex parameters complex i-axis
    //! \param[in] c (aka y)  Complex parameters complex j-axis
    //! \param[in] d (aka z)  Complex parameters complex k-axis
    //! \note the length of (b, c, d) shall be <= 1.
    //--------------------------------------------------------------------------
    Quat(T b_, T c_, T d_)
    {
        T n = (b_ * b_) + (c_ * c_) + (d_ * d_);
        assert(n <= T(1));

        data[0] = std::sqrt(T(1) - n);
        data[1] = b_;
        data[2] = c_;
        data[3] = d_;
    }

    //--------------------------------------------------------------------------
    //! \brief Copy constructor
    //--------------------------------------------------------------------------
    Quat(Quat const& other) noexcept
    {
        *this = other;
    }

    //--------------------------------------------------------------------------
    //! \brief Construct a quaternion from a rotation matrix
    //--------------------------------------------------------------------------
    Quat(Matrix<T, 4u, 4u> const& m)
    {
        *this = Quat::fromMatrix(m);
    }

    //--------------------------------------------------------------------------
    //! \brief Copy operator
    //--------------------------------------------------------------------------
    Quat& operator=(Quat const& other) noexcept
    {
        data[0] = other.data[0];
        data[1] = other.data[1];
        data[2] = other.data[2];
        data[3] = other.data[3];

        return *this;
    }

    //--------------------------------------------------------------------------
    //! \brief Creates a rotation which rotates angle degrees around axis.
    //--------------------------------------------------------------------------
    static Quat<T> fromAngleAxis(units::angle::radian_t const angle,
                                 Vector<T, 3u> const& v)
    {
        T const s = std::sin(angle.to<T>() * static_cast<T>(0.5));
        Quat<T> q(std::cos(angle.to<T>() * static_cast<T>(0.5)),
                  v.x * s,
                  v.y * s,
                  v.z * s);
        q.normalize();
        return q;
    }

    //--------------------------------------------------------------------------
    //! \brief Construct a quaternion from an angle/axis
    //--------------------------------------------------------------------------
    Quat(units::angle::radian_t const angle, Vector<T, 3u> const& axis)
    {
        *this = fromAngleAxis(angle, axis);
    }

    //--------------------------------------------------------------------------
    //! \brief Print quaternion
    //--------------------------------------------------------------------------
    inline friend std::ostream& operator<<(std::ostream& os, Quat const& q)
    {
        return os << "Quat(" << q.a << ", (" << q.b << "i, " << q.c << "j, "
                  << q.d << "k))";
    }

    //--------------------------------------------------------------------------
    //! \brief Return the angle of rotation in radian.
    //--------------------------------------------------------------------------
    units::angle::radian_t angle()
    {
        return units::angle::radian_t(std::acos(a) * T(2));
    }

    //--------------------------------------------------------------------------
    //! \brief Return the rotation axis (x,y,z).
    //--------------------------------------------------------------------------
    Vector<T, 3u> axis()
    {
        T tmp1 = static_cast<T>(1) - a * a;
        if (tmp1 <= static_cast<T>(0))
            return Vector<T, 3u>(0, 0, 1);
        T tmp2 = static_cast<T>(1) / std::sqrt(tmp1);
        return Vector<T, 3u>(b * tmp2, c * tmp2, d * tmp2);
    }

    //--------------------------------------------------------------------------
    //! \brief Return the L2 norm of the quaternion.
    //--------------------------------------------------------------------------
    T norm() const
    {
        return std::sqrt(a * a + b * b + c * c + d * d);
    }

    //--------------------------------------------------------------------------
    //! \brief Normalize quaternion.
    //--------------------------------------------------------------------------
    void normalize()
    {
        T const n = norm();
        operator*=(*this, 1 / n);
    }

    //--------------------------------------------------------------------------
    //! \brief Complex conjugate quaternion.
    //--------------------------------------------------------------------------
    Quat conjugate() const
    {
        return Quat(a, -b, -c, -d);
    }

    //--------------------------------------------------------------------------
    //! \brief negation.
    //--------------------------------------------------------------------------
    Quat operator-() const
    {
        return { -data[0], -data[1], -data[2], -data[3] };
    }

    //--------------------------------------------------------------------------
    //! \brief Rotate a vector using this quaternion.
    //! \param[in] v Vector stored in the three complex terms
    //--------------------------------------------------------------------------
    Quat transform(const Quat& v) const
    {
        const Quat& q = *this;
        return q * v * q.conjugate();
    }

    //--------------------------------------------------------------------------
    //! \brief Convert a rotation quaternion to its matrix form.
    //! \note The result is not correct if this quaternion is not a member of
    //!   S(4).
    //! \return 4x4 Rotation matrix.
    //--------------------------------------------------------------------------
    Matrix<T, 4u, 4u> toMatrix() const
    {
        Matrix<T, 4u, 4u> Result(compages::core::matrix::Identity);
        T qxx(b * b);
        T qyy(c * c);
        T qzz(d * d);
        T qxz(b * d);
        T qxy(b * c);
        T qyz(c * d);
        T qwx(a * b);
        T qwy(a * c);
        T qwz(a * d);

        Result(0, 0) = T(1) - T(2) * (qyy + qzz);
        Result(0, 1) = T(2) * (qxy - qwz);
        Result(0, 2) = T(2) * (qxz + qwy);

        Result(1, 0) = T(2) * (qxy + qwz);
        Result(1, 1) = T(1) - T(2) * (qxx + qzz);
        Result(1, 2) = T(2) * (qyz - qwx);

        Result(2, 0) = T(2) * (qxz - qwy);
        Result(2, 1) = T(2) * (qyz + qwx);
        Result(2, 2) = T(1) - T(2) * (qxx + qyy);

        return Result;
    }

    //--------------------------------------------------------------------------
    //! \brief Integrate a rotation quaternion using 4th order Runge Kutta.
    //! \param[in] q0 Angular velocity (body frame), stored in 3 complex terms.
    //! \param[in] dt Time interval in seconds.
    //! \param[in] normalize If true, quaternion is normalized after
    //! integration.
    //--------------------------------------------------------------------------
    void integrateRungeKutta4(const Quat& q0, T dt, bool normalize = true)
    {
        const static T half = static_cast<T>(0.5);
        const static T two = static_cast<T>(2);

        Quat& q = *this;
        Quat qw = q * q0 * half;

        Quat k2 = (q + qw * dt * half) * q0 * half;
        Quat k3 = (q + k2 * dt * half) * q0 * half;
        Quat k4 = (q + k3 * dt) * q0 * half;

        q += (qw + k2 * two + k3 * two + k4) * (dt / 6);

        if (normalize)
        {
            q.normalize();
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Integrate a rotation quaternion using Euler integration.
    //! \param[in] q0 Angular velocity (body frame), stored in 3 complex terms.
    //! \param[in] dt Time interval in seconds.
    //! \param[in] normalize If True, quaternion is normalized after
    //! integration.
    //--------------------------------------------------------------------------
    void integrateEuler(const Quat& q0, T dt, bool normalize = true)
    {
        Quat& q = *this;
        q += (q * q0 * static_cast<T>(0.5)) * dt;

        if (normalize)
        {
            q.normalize();
        }
    }

    //--------------------------------------------------------------------------
    //! \brief Create a rotation quaternion
    //! \param[in] theta Angle of rotation (unit: radian).
    //! \param[in] x X component of rotation vector.
    //! \param[in] y Y component of rotation vector.
    //! \param[in] z Z component of rotation vector.
    //--------------------------------------------------------------------------
    static Quat rotation(units::angle::radian_t theta, T x, T y, T z)
    {
        T const haversine = std::sin(theta.to<T>() / 2);
        T const havercosine = std::cos(theta.to<T>() / 2);

        return Quat(havercosine, haversine * x, haversine * y, haversine * z);
    }

    //--------------------------------------------------------------------------
    //! \brief Create a rotation quaternion from rotation vector [x,y,z].
    //! \note If x/y/z have 0 norm, this function returns the identity
    //! transform.
    //--------------------------------------------------------------------------
    static Quat rotation(T x, T y, T z)
    {
        T const theta = std::sqrt(x * x + y * y + z * z);

        if (theta < std::numeric_limits<T>::epsilon() * 10)
        {
            return Quat();
        }

        return rotation(theta, x / theta, y / theta, z / theta);
    }

    //--------------------------------------------------------------------------
    //! \brief Create quaternion from matrix.
    //! \param[in] m Rotation matrix, should be a member of SO[3].
    //! \param[in] Matrix Any matrix-like type which supports the (i,j)
    //! operator.
    //! \note All singularities are handled, provided \c m belongs to SO[3].
    //! \see
    //! http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm
    //--------------------------------------------------------------------------
    static Quat fromMatrix(Matrix<T, 4u, 4u> const& m)
    {
        Quat Q;
        T const trace = m(0, 0) + m(1, 1) + m(2, 2);
        T s = 0;

        if (trace > 0)
        {
            s = 2 * std::sqrt(1 + trace);

            Q.a = s / 4;
            Q.b = (m(2, 1) - m(1, 2)) / s;
            Q.c = (m(0, 2) - m(2, 0)) / s;
            Q.d = (m(1, 0) - m(0, 1)) / s;
        }
        else if (m(0, 0) > m(1, 1) && m(0, 0) > m(2, 2))
        {
            s = 2 * std::sqrt(1 + m(0, 0) - m(1, 1) - m(2, 2));

            Q.a = (m(2, 1) - m(1, 2)) / s;
            Q.b = s / 4;
            Q.c = (m(0, 1) + m(1, 0)) / s;
            Q.d = (m(0, 2) + m(2, 0)) / s;
        }
        else if (m(1, 1) > m(2, 2))
        {
            s = 2 * std::sqrt(1 + m(1, 1) - m(0, 0) - m(2, 2));

            Q.a = (m(0, 2) - m(2, 0)) / s;
            Q.b = (m(0, 1) + m(1, 0)) / s;
            Q.c = s / 4;
            Q.d = (m(1, 2) + m(2, 1)) / s;
        }
        else
        {
            s = 2 * std::sqrt(1 + m(2, 2) - m(0, 0) - m(1, 1));

            Q.a = (m(1, 0) - m(0, 1)) / s;
            Q.b = (m(0, 2) + m(2, 0)) / s;
            Q.c = (m(1, 2) + m(2, 1)) / s;
            Q.d = s / 4;
        }

        return Q;
    }

    //--------------------------------------------------------------------------
    //! \brief Pointer accessor for direct copying
    //-------------------------------------------------------------------------
    inline T* ptr()
    {
        return data;
    }
    inline T const* ptr() const
    {
        return data;
    }

    //--------------------------------------------------------------------------
    //! \brief operator () Element-wise accessor.
    //! \param[in] i Index into quaternion, must be less than 4 (not checked).
    //! \return Element i of the quaternion.
    //--------------------------------------------------------------------------
    T& operator[](size_t const i)
    {
        assert(i < 4u);
        return data[i];
    }
    T const& operator[](size_t const i) const
    {
        assert(i < 4u);
        return data[i];
    }

public:

    //--------------------------------------------------------------------------
    //! \brief static array holdind values.
    //--------------------------------------------------------------------------
    union
    {
        T data[4u];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
        struct
        {
            T a;
            union
            {
                struct
                {
                    T b;
                    T c;
                    T d;
                };
                Vector<T, 3u> bcd;
            };
        };
        struct
        {
            T w;
            union
            {
                struct
                {
                    T x;
                    T y;
                    T z;
                };
                Vector<T, 3u> xyz;
            };
        };
#pragma GCC diagnostic pop
    };
};

// \brief Typedefs for common float64 quaternion.
typedef Quat<double> Quatd;

// \brief Typedefs for common float32 quaternion.
typedef Quat<float> Quatf;

//! \brief Multiply two quaternions.
//! \param[in] q0 Left quaternion
//! \param[in] q1 Right quaternion
//! \return Product of both quaternions
template <typename T>
Quat<T> operator*(Quat<T> const& q0, Quat<T> const& q1)
{
    Quat<T> lhs;

    lhs[0] = q0[0] * q1[0] - q0[1] * q1[1] - q0[2] * q1[2] - q0[3] * q1[3];
    lhs[1] = q0[0] * q1[1] + q0[1] * q1[0] + q0[2] * q1[3] - q0[3] * q1[2];
    lhs[2] = q0[0] * q1[2] - q0[1] * q1[3] + q0[2] * q1[0] + q0[3] * q1[1];
    lhs[3] = q0[0] * q1[3] + q0[1] * q1[2] - q0[2] * q1[1] + q0[3] * q1[0];

    return lhs;
}

//! \brief Rotation of a vector by a quaternion
template <typename T>
Vector3f operator*(Quat<T> const& q, const Vector3f& v)
{
    Vector3f uv, uuv;
    Vector3f qvec(q.b, q.c, q.d);

    uv = qvec % v; // Cross product
    uuv = qvec % uv;
    uv *= (T(2) * q.a);
    uuv *= T(2);

    return v + uv + uuv;
}

//! \brief Divide a quaternion by a T.
//! \param[in] a Quaternion.
//! \param[in] s T.
//! \return Scaled quaternion.
template <typename T>
Quat<T> operator/(Quat<T> const& q, T const s)
{
    return operator*(q, T(1) / s);
}

//! \brief Right-multiply a quaternion by a T
//! \param[in] a Quaternion
//! \param[in] s T
//! \return Scaled quaternion
template <typename T>
Quat<T> operator*(Quat<T> const& q, T const s)
{
    return { q.a * s, q.b * s, q.c * s, q.d * s };
}

//! \brief Left-multiply a quaternion by a T
//! \param[in] q Quaternion
//! \param[in] s T
//! \return Scaled quaternion
template <typename T>
Quat<T> operator*(T const s, Quat<T> const& q)
{
    return operator*(q, s);
}

//! \brief Multiply a quaternion by a T, in place
//! \param[in] q Quaternion to scale
//! \param[in] s T
//! \return a
template <typename T>
Quat<T>& operator*=(Quat<T>& q, T const s)
{
    q.a *= s;
    q.b *= s;
    q.c *= s;
    q.d *= s;
    return q;
}

//! \brief Divide a quaternion by a T, in place
//! \param[in] q Quaternion to scale
//! \param[in] s T
//! \return a
template <typename T>
Quat<T>& operator/=(Quat<T>& q, T const s)
{
    return operator*=(q, T(1) / s);
}

//! \brief Add two quaternions (element-wise summation)
//! \param[in] q1 First quaternion
//! \param[in] q2 Second quaternion
//! \return The sum
template <typename T>
Quat<T> operator+(Quat<T> const& q1, Quat<T> const& q2)
{
    return { q1.a + q2.a, q1.b + q2.b, q1.c + q2.c, q1.d + q2.d };
}

//! \brief Add-in place quaternion
//! \param[in] q1 First quaternion, is modified
//! \param[in] q2 Second quaternion
//! \return q1
template <typename T>
Quat<T>& operator+=(Quat<T>& q1, Quat<T> const& q2)
{
    q1.a += q2.a;
    q1.b += q2.b;
    q1.c += q2.c;
    q1.d += q2.d;
    return q1;
}

//! \brief Substract two quaternions (element-wise difference)
//! \param[in] q1 First quaternion
//! \param[in] q2 Second quaternion
//! \return The difference
template <typename T>
Quat<T> operator-(Quat<T> const& q1, Quat<T> const& q2)
{
    return { q1.a - q2.a, q1.b - q2.b, q1.c - q2.c, q1.d - q2.d };
}

//! \brief Substract a quaternion by a T, in place
//! \param[in] q1 First quaternion, is modified
//! \param[in] q2 Second quaternion
//! \return q1
template <typename T>
Quat<T>& operator-=(Quat<T>& q1, Quat<T> const& q2)
{
    q1.a -= q2.a;
    q1.b -= q2.b;
    q1.c -= q2.c;
    q1.d -= q2.d;
    return q1;
}

//! \brief Dot product
template <typename T>
float dot(Quat<T> const& q1, Quat<T> const& q2)
{
    return q1.a * q2.a + q1.b * q2.b + q1.c * q2.c + q1.d * q2.d;
}

} // namespace compages::core

