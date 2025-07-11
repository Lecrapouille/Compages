//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

#ifndef OPENGLCPPWRAPPER_MATHS_HPP
#define OPENGLCPPWRAPPER_MATHS_HPP

#include "Units.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

//------------------------------------------------------------------------------
//! \file Maths.hpp Wrap some mathematic functions.
//------------------------------------------------------------------------------

namespace maths
{

//------------------------------------------------------------------------------
//! \brief Allow to redefine neutral and/or absorbing element in algebra.
//------------------------------------------------------------------------------
template <typename T>
T one()
{
    return T(1);
}

//------------------------------------------------------------------------------
//! \brief Allow to redefine neutral and/or absorbing element in algebra.
//------------------------------------------------------------------------------
template <typename T>
T zero()
{
    return T(0);
}

//------------------------------------------------------------------------------
//! \brief Return the biggest number for float and double
//------------------------------------------------------------------------------
template <typename T>
T max()
{
    return std::numeric_limits<T>::max();
}

//------------------------------------------------------------------------------
//! \brief Return the smallest number for float and double
//------------------------------------------------------------------------------
template <typename T>
T min()
{
    return std::numeric_limits<T>::min();
}

//------------------------------------------------------------------------------
//! \brief Return Not A Number for float and double
//------------------------------------------------------------------------------
template <typename T>
T nan()
{
    return std::numeric_limits<T>::quiet_NaN();
}

//------------------------------------------------------------------------------
//! \brief Check if x is a Not A Number for float and double
//------------------------------------------------------------------------------
template <typename T>
bool isNan(T const& x)
{
    return std::isnan(x);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-const-variable"

//------------------------------------------------------------------------------
// Constant numbers
//------------------------------------------------------------------------------

//! \brief PI number
template <typename T>
constexpr T PI = T(3.1415926535897932385);
//! \brief PI number / 2
template <typename T>
constexpr T HALF_PI = T(0.5) * PI<T>;
//! \brief 2 * PI number
template <typename T>
constexpr T TWO_PI = T(2.0) * PI<T>;
//! \brief ln(2)
template <typename T>
constexpr T LN2 = T(0.6931471805599453094);

#pragma GCC diagnostic pop

//------------------------------------------------------------------------------
//! \brief
//------------------------------------------------------------------------------
static inline bool isPowerOfTwo(int const value)
{
    return (value != 0) && ((value & (value - 1)) == 0);
}

//------------------------------------------------------------------------------
//! \brief
//------------------------------------------------------------------------------
static inline int upperPowerOfTwo(int const value)
{
    return static_cast<int>(std::pow(
        2.0f, std::ceil(std::log(static_cast<float>(value)) / LN2<float>)));
}

//------------------------------------------------------------------------------
//! \brief
//------------------------------------------------------------------------------
static inline int lowerPowerOfTwo(int const value)
{
    return static_cast<int>(std::pow(
        2.0f, std::floor(std::log(static_cast<float>(value)) / LN2<float>)));
}

//------------------------------------------------------------------------------
//! \brief
//------------------------------------------------------------------------------
static inline int nearestPowerOfTwo(int const value)
{
    return static_cast<int>(std::pow(
        2.0f, std::round(std::log(static_cast<float>(value)) / LN2<float>)));
}

//------------------------------------------------------------------------------
//! \brief Return absolute number
//------------------------------------------------------------------------------
template <typename T>
static inline T abs(T const x)
{
    return (x >= zero<T>()) ? x : -x;
}

// TODO https://gist.github.com/MikimotoH/282dca62e08b90b9b673
static inline float sqrt(int const x)
{
    return ::sqrtf(static_cast<float>(x));
}

static inline float sqrt(size_t const x)
{
    return ::sqrtf(static_cast<float>(x));
}

static inline float sqrt(float const x)
{
    return ::sqrtf(x);
}

static inline long double sqrt(long double const x)
{
    return ::sqrtl(x);
}

static inline double sqrt(double const x)
{
    return ::sqrt(x);
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wsign-conversion"

//! \brief
static uint32_t maxUlps = 6U;

//! \brief https://bitbashing.io/comparing-floats.html
//! T/R float/int32_t
template <typename T, typename R>
static R ulpsDistance(const T a, const T b)
{
    // Save work if the floats are equal.
    // Also handles +0 == -0
    if (a == b)
        return 0;

    const auto max = std::numeric_limits<R>::max();

    // Max distance for NaN
    if (std::isnan(a) || std::isnan(b))
        return max;

    // If one's infinite and they're not equal, max distance.
    if (std::isinf(a) || std::isinf(b))
        return max;

    // Float to int without strict aliasing rules warnings
    R ia, ib;
    memcpy(&ia, &a, sizeof(T));
    memcpy(&ib, &b, sizeof(T));

    // Don't compare differently-signed floats.
    if ((ia < 0) != (ib < 0))
        return max;

    // Return the absolute value of the distance in ULPs.
    R distance = ia - ib;
    if (distance < 0)
        distance = -distance;
    return distance;
}

static inline bool almostEqual(float const a, float const b)
{
    return ulpsDistance<float, int32_t>(a, b) <= int32_t(maths::maxUlps);
}

static inline bool almostEqual(double const a, double const b)
{
    return ulpsDistance<double, int64_t>(a, b) <= int32_t(maths::maxUlps);
}

#pragma GCC diagnostic pop

static inline bool almostZero(float const A)
{
    return almostEqual(A, zero<float>());
}

template <typename T,
          class = typename std::enable_if<std::is_integral<T>::value>::type>
bool inline isPowerOfTwo(T const value)
{
    return ((value & (value - one<T>())) == zero<T>()) && (value != zero<T>());
}

//------------------------------------------------------------------------------
//! \brief Constrain value: std::min(std::max(x, lower), upper)
//------------------------------------------------------------------------------
template <typename T>
static inline T clamp(T const x, T const lower, T const upper)
{
    if (x < lower)
        return lower;

    if (x > upper)
        return upper;

    return x;
}

//------------------------------------------------------------------------------
//! \brief Return the sign of the number: -1 or 0 or +1.
//------------------------------------------------------------------------------
template <typename T>
static inline int sign(T const val)
{
    return (zero<T>() < val) - (val < zero<T>);
}

//! \brief Converts radians to degrees and returns the result.
template <typename T>
static inline T toRadian(T const degrees)
{
    return static_cast<T>(degrees * T(0.01745329251994329576923690768489));
}

//------------------------------------------------------------------------------
//! \brief Converts degrees to radians and returns the result.
//------------------------------------------------------------------------------
template <typename T>
static inline T toDegree(T const radians)
{
    return static_cast<T>(radians * T(57.295779513082320876798154814105));
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in degrees to [-180 +180] degrees.
//------------------------------------------------------------------------------
template <typename T>
static inline T wrapTo180(T const degrees)
{
    T angle = degrees;
    while (angle <= -T(180))
        angle += T(360);
    while (angle > T(180))
        angle -= T(360);
    return angle;
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in degrees to [0 +360] degrees.
//------------------------------------------------------------------------------
template <typename T>
static inline T wrapTo360(T const degrees)
{
    T angle = degrees;
    while (angle < zero<T>())
        angle += T(360);
    while (angle >= T(360))
        angle -= T(360);
    return angle;
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in radians to [-PI +PI] radians.
//------------------------------------------------------------------------------
template <typename T>
static inline T wrapToPI(T const radians)
{
    T angle = radians;
    while (angle <= -PI<T>)
        angle += TWO_PI<T>;
    while (angle > PI<T>)
        angle -= TWO_PI<T>;
    return angle;
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in radians to [0 2*PI] radians.
//------------------------------------------------------------------------------
template <typename T>
static inline T wrapTo2PI(T const radians)
{
    T angle = radians;
    while (angle < zero<T>())
        angle += TWO_PI<T>;
    while (angle >= TWO_PI<T>)
        angle -= TWO_PI<T>;
    return angle;
}

//------------------------------------------------------------------------------
//! \brief Linear mapping of x from range2 [start1 stop1] to range2 [start2
//! stop2].
//------------------------------------------------------------------------------
template <typename T>
static inline T
lmap(T const x, T const start1, T const stop1, T const start2, T const stop2)
{
    T mapped = start2 + (stop2 - start2) * ((x - start1) / (stop1 - start1));
    return mapped;
}

//------------------------------------------------------------------------------
//! \brief Linear interpolation between a and b for the parameter t with t
//! inside the range [0,1].
//------------------------------------------------------------------------------
template <typename T>
static inline T lerp(T const a, T const b, T const t)
{
    assert((t >= zero<T>()) && (t <= one<T>()) && "param t shall be [0 1]");
    return (one<T>() - t) * a + t * b;
}

//------------------------------------------------------------------------------
//! \brief Return evenly spaced numbers over a specified interval.
//! \param[in] start The starting scalar value of the sequence.
//! \param[in] end The end value of the sequence, unless endpoint is set to
//!    false.  In that case, the sequence consists of all but the last of num +
//!    1 evenly spaced samples, so that stop is excluded.
//! \param[in] N Number of samples to generate. Should be > 1.
//! \param[out] The vector of equally spaced samples in the closed interval
//!   [start, stop] or the half-open interval [start, stop) (depending on
//!   whether endpoint is true or false).
//! \return Size of spacing between samples. Return NaN if this value cannot be
//! computed.
//! \note: This code has been inpsired by the Numpy.linspace function.
//------------------------------------------------------------------------------
template <typename T>
static T linspace(T const start,
                  T const end,
                  size_t const N,
                  std::vector<T>& result,
                  const bool endpoint)
{
    const T not_a_number = std::numeric_limits<T>::quiet_NaN();
    result.clear();

    if (0u == N)
    {
        return not_a_number;
    }

    result.resize(N);
    if (1u == N)
    {
        result.push_back(start);
        return not_a_number;
    }

    T delta = (end - start) / static_cast<T>(N - 1u);
    for (size_t i = 0u; i < N - 1u; ++i)
    {
        result[i] = start + delta * T(i);
    }

    if (endpoint)
    {
        result[N - 1u] = end;
    }
    return delta;
}

//------------------------------------------------------------------------------
//! https://en.wikipedia.org/wiki/Smoothstep
//------------------------------------------------------------------------------
template <typename T>
static T smoothstep(T const x, T const min, T const max)
{
    if (x <= min)
        return zero<T>();
    if (x >= max)
        return one<T>();

    x = (x - min) / (max - min);
    return x * x * (T(3) - T(2) * x);
}

//------------------------------------------------------------------------------
//! https://en.wikipedia.org/wiki/Smoothstep
//------------------------------------------------------------------------------
template <typename T>
static T smootherstep(T const x, T const min, T const max)
{
    if (x <= min)
        return zero<T>();
    if (x >= max)
        return one<T>();

    x = (x - min) / (max - min);
    return x * x * x * (x * (x * T(6) - T(15)) + T(10));
}

} // namespace maths

#endif
