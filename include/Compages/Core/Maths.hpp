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

#include "Units.hpp"
#include <bit>
#include <cassert>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <numbers>
#include <type_traits>
#include <vector>

//------------------------------------------------------------------------------
//! \file Maths.hpp Wrap some mathematic functions.
//------------------------------------------------------------------------------

namespace compages::core
{

//------------------------------------------------------------------------------
//! \brief Allow to redefine neutral and/or absorbing element in algebra.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T one()
{
    return T(1);
}

//------------------------------------------------------------------------------
//! \brief Allow to redefine neutral and/or absorbing element in algebra.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T zero()
{
    return T(0);
}

//------------------------------------------------------------------------------
//! \brief Return the biggest finite number of the type.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T max()
{
    return std::numeric_limits<T>::max();
}

//------------------------------------------------------------------------------
//! \brief Return std::numeric_limits<T>::min(): the smallest positive
//! normalized value for floating points, the most negative value for integers.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T min()
{
    return std::numeric_limits<T>::min();
}

//------------------------------------------------------------------------------
//! \brief Return Not A Number for float and double
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T nan()
{
    return std::numeric_limits<T>::quiet_NaN();
}

//------------------------------------------------------------------------------
//! \brief Check if x is a Not A Number for float and double
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] bool isNan(T const& x)
{
    return std::isnan(x);
}

//------------------------------------------------------------------------------
// Constant numbers
//------------------------------------------------------------------------------

//! \brief PI number
template <std::floating_point T>
inline constexpr T PI = std::numbers::pi_v<T>;
//! \brief PI number / 2
template <std::floating_point T>
inline constexpr T HALF_PI = std::numbers::pi_v<T> / T(2);
//! \brief 2 * PI number
template <std::floating_point T>
inline constexpr T TWO_PI = T(2) * std::numbers::pi_v<T>;
//! \brief ln(2)
template <std::floating_point T>
inline constexpr T LN2 = std::numbers::ln2_v<T>;

//------------------------------------------------------------------------------
//! \brief Return true if \c value is a strictly positive power of two.
//------------------------------------------------------------------------------
template <std::integral T>
[[nodiscard]] constexpr bool isPowerOfTwo(T const value)
{
    return (value > T(0)) &&
           std::has_single_bit(static_cast<std::make_unsigned_t<T>>(value));
}

//------------------------------------------------------------------------------
//! \brief Smallest power of two greater than or equal to \c value. Return 1
//! for \c value < 1.
//! \pre The result shall be representable by \c T.
//------------------------------------------------------------------------------
template <std::integral T>
[[nodiscard]] constexpr T upperPowerOfTwo(T const value)
{
    using U = std::make_unsigned_t<T>;
    if (value <= T(1))
    {
        return T(1);
    }
    assert(static_cast<U>(value) <= (U(1) << (std::numeric_limits<T>::digits - 1)) &&
           "upperPowerOfTwo: result not representable");
    return static_cast<T>(std::bit_ceil(static_cast<U>(value)));
}

//------------------------------------------------------------------------------
//! \brief Greatest power of two less than or equal to \c value. Return 0
//! for \c value < 1.
//------------------------------------------------------------------------------
template <std::integral T>
[[nodiscard]] constexpr T lowerPowerOfTwo(T const value)
{
    if (value < T(1))
    {
        return T(0);
    }
    return static_cast<T>(
        std::bit_floor(static_cast<std::make_unsigned_t<T>>(value)));
}

//------------------------------------------------------------------------------
//! \brief Power of two whose exponent is the rounded base-2 logarithm of
//! \c value, i.e. 2^round(log2(value)). The rounding happens in log space: the
//! switch from 2^k to 2^(k+1) is at 2^k * sqrt(2), not at 1.5 * 2^k. Return 1
//! for \c value < 1.
//! \pre The result shall be representable by \c T.
//------------------------------------------------------------------------------
template <std::integral T>
    requires(sizeof(T) <= sizeof(std::uint32_t))
[[nodiscard]] constexpr T nearestPowerOfTwo(T const value)
{
    if (value <= T(1))
    {
        return T(1);
    }
    std::uint64_t const v = static_cast<std::uint64_t>(value);
    std::uint64_t const lower = std::bit_floor(v);
    // value > lower * sqrt(2)  <=>  value^2 > 2 * lower^2 (never equal).
    if (v * v > 2u * lower * lower)
    {
        return upperPowerOfTwo(value);
    }
    return static_cast<T>(lower);
}

//------------------------------------------------------------------------------
//! \brief Return absolute number
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T abs(T const x)
{
    return (x >= zero<T>()) ? x : -x;
}

// TODO https://gist.github.com/MikimotoH/282dca62e08b90b9b673
[[nodiscard]] inline float sqrt(int const x)
{
    return std::sqrt(static_cast<float>(x));
}

[[nodiscard]] inline float sqrt(size_t const x)
{
    return std::sqrt(static_cast<float>(x));
}

[[nodiscard]] inline float sqrt(float const x)
{
    return std::sqrt(x);
}

[[nodiscard]] inline long double sqrt(long double const x)
{
    return std::sqrt(x);
}

[[nodiscard]] inline double sqrt(double const x)
{
    return std::sqrt(x);
}

//------------------------------------------------------------------------------
//! \brief Default tolerance, in ULPs, used by almostEqual().
//------------------------------------------------------------------------------
inline constexpr std::uint32_t DEFAULT_MAX_ULPS = 6u;

namespace detail
{
//! \brief Signed integer type having the same size than the float type T.
template <std::floating_point T>
struct UlpsInteger;

template <>
struct UlpsInteger<float>
{
    using type = std::int32_t;
};

template <>
struct UlpsInteger<double>
{
    using type = std::int64_t;
};
} // namespace detail

//------------------------------------------------------------------------------
//! \brief Distance in ULPs between two floating points of the same sign.
//! Return the max value of the integer type for NaN, infinity or different
//! signs.
//! \see https://bitbashing.io/comparing-floats.html
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] constexpr typename detail::UlpsInteger<T>::type
ulpsDistance(T const a, T const b)
{
    using R = typename detail::UlpsInteger<T>::type;
    static_assert(sizeof(T) == sizeof(R));

    // Save work if the floats are equal. Also handles +0 == -0.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
    if (a == b)
        return 0;
#pragma GCC diagnostic pop

    constexpr R max = std::numeric_limits<R>::max();

    // Max distance for NaN and for infinities which are not equal.
    if (std::isnan(a) || std::isnan(b) || std::isinf(a) || std::isinf(b))
        return max;

    R const ia = std::bit_cast<R>(a);
    R const ib = std::bit_cast<R>(b);

    // Don't compare differently-signed floats.
    if ((ia < 0) != (ib < 0))
        return max;

    // Same sign so the subtraction cannot overflow.
    R const distance = ia - ib;
    return (distance < 0) ? -distance : distance;
}

//------------------------------------------------------------------------------
//! \brief Compare two floating points with a tolerance expressed in ULPs.
//! \note ULPs are meaningless around zero (0 and 1e-30 are billions of ULPs
//!   apart): use almostZero() for comparisons against zero.
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] constexpr bool
almostEqual(T const a, T const b, std::uint32_t const maxUlps = DEFAULT_MAX_ULPS)
{
    using R = typename detail::UlpsInteger<T>::type;
    return ulpsDistance(a, b) <= static_cast<R>(maxUlps);
}

//------------------------------------------------------------------------------
//! \brief Exact comparison for integers (so that generic Vector/Matrix code
//! can call almostEqual() whatever their type).
//------------------------------------------------------------------------------
template <std::integral T>
[[nodiscard]] constexpr bool almostEqual(T const a, T const b)
{
    return a == b;
}

//------------------------------------------------------------------------------
//! \brief Check if |x| <= epsilon (absolute tolerance, see almostEqual()).
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] constexpr bool
almostZero(T const x, T const epsilon = std::numeric_limits<T>::epsilon())
{
    return abs(x) <= epsilon;
}

//------------------------------------------------------------------------------
//! \brief Exact comparison against zero for integers.
//------------------------------------------------------------------------------
template <std::integral T>
[[nodiscard]] constexpr bool almostZero(T const x)
{
    return x == T(0);
}

//------------------------------------------------------------------------------
//! \brief Constrain value: std::min(std::max(x, lower), upper)
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T clamp(T const x, T const lower, T const upper)
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
[[nodiscard]] constexpr int sign(T const val)
{
    return int(zero<T>() < val) - int(val < zero<T>());
}

//------------------------------------------------------------------------------
//! \brief Converts degrees to radians and returns the result.
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] constexpr T toRadian(T const degrees)
{
    return degrees * (std::numbers::pi_v<T> / T(180));
}

//------------------------------------------------------------------------------
//! \brief Converts radians to degrees and returns the result.
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] constexpr T toDegree(T const radians)
{
    return radians * (T(180) / std::numbers::pi_v<T>);
}

namespace detail
{
//------------------------------------------------------------------------------
//! \brief Remainder of x / period with the sign of x (like std::fmod), in
//! constant time whatever the distance of x to the interval.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] T truncatedModulo(T const x, T const period)
{
    if constexpr (std::floating_point<T>)
        return std::fmod(x, period);
    else
        return x % period;
}

//------------------------------------------------------------------------------
//! \brief Wrap x into [low, low + period).
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] T wrapHalfOpen(T const x, T const low, T const period)
{
    T r = truncatedModulo(T(x - low), period);
    if (r < zero<T>())
    {
        r += period;
        // A tiny negative r rounds r + period up to period.
        if (r >= period)
            r = zero<T>();
    }
    return low + r;
}

//------------------------------------------------------------------------------
//! \brief Wrap x into (low, low + period].
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] T wrapHalfClosed(T const x, T const low, T const period)
{
    T r = truncatedModulo(T(x - low), period);
    if (r <= zero<T>())
        r += period;
    return low + r;
}
} // namespace detail

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in degrees to ]-180 +180] degrees.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] T wrapTo180(T const degrees)
{
    return detail::wrapHalfClosed(degrees, T(-180), T(360));
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in degrees to [0 +360[ degrees.
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] T wrapTo360(T const degrees)
{
    return detail::wrapHalfOpen(degrees, zero<T>(), T(360));
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in radians to ]-PI +PI] radians.
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] T wrapToPI(T const radians)
{
    return detail::wrapHalfClosed(radians, -PI<T>, TWO_PI<T>);
}

//------------------------------------------------------------------------------
//! \brief Normalize the angle given in radians to [0 2*PI[ radians.
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] T wrapTo2PI(T const radians)
{
    return detail::wrapHalfOpen(radians, zero<T>(), TWO_PI<T>);
}

//------------------------------------------------------------------------------
//! \brief Linear mapping of x from range1 [start1 stop1] to range2 [start2
//! stop2].
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T
lmap(T const x, T const start1, T const stop1, T const start2, T const stop2)
{
    return start2 + (stop2 - start2) * ((x - start1) / (stop1 - start1));
}

//------------------------------------------------------------------------------
//! \brief Linear interpolation between a and b for the parameter t with t
//! inside the range [0,1].
//------------------------------------------------------------------------------
template <typename T>
[[nodiscard]] constexpr T lerp(T const a, T const b, std::type_identity_t<T> const t)
{
    assert((t >= zero<T>()) && (t <= one<T>()) && "param t shall be [0 1]");
    return (one<T>() - t) * a + t * b;
}

//------------------------------------------------------------------------------
//! \brief Return evenly spaced numbers over a specified interval.
//! \param[in] start The starting scalar value of the sequence.
//! \param[in] end The end value of the sequence, unless endpoint is set to
//!    false.  In that case, the sequence consists of all but the last of N +
//!    1 evenly spaced samples, so that end is excluded.
//! \param[in] N Number of samples to generate.
//! \param[out] result The vector of N equally spaced samples in the closed
//!   interval [start, end] or the half-open interval [start, end) (depending
//!   on whether endpoint is true or false).
//! \param[in] endpoint If true, end is the last sample.
//! \return Size of spacing between samples. Return NaN if this value cannot be
//! computed (N == 0, or N == 1 with endpoint).
//! \note: This code has been inspired by the Numpy.linspace function.
//------------------------------------------------------------------------------
template <std::floating_point T>
T linspace(T const start,
           T const end,
           size_t const N,
           std::vector<T>& result,
           bool const endpoint = true)
{
    result.clear();
    if (0u == N)
    {
        return nan<T>();
    }

    result.resize(N);
    size_t const divisions = endpoint ? N - 1u : N;
    if (0u == divisions)
    {
        result[0] = start;
        return nan<T>();
    }

    T const delta = (end - start) / static_cast<T>(divisions);
    for (size_t i = 0u; i < N; ++i)
    {
        result[i] = start + delta * static_cast<T>(i);
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
template <std::floating_point T>
[[nodiscard]] constexpr T smoothstep(T const x, T const min, T const max)
{
    if (x <= min)
        return zero<T>();
    if (x >= max)
        return one<T>();

    T const t = (x - min) / (max - min);
    return t * t * (T(3) - T(2) * t);
}

//------------------------------------------------------------------------------
//! https://en.wikipedia.org/wiki/Smoothstep
//------------------------------------------------------------------------------
template <std::floating_point T>
[[nodiscard]] constexpr T smootherstep(T const x, T const min, T const max)
{
    if (x <= min)
        return zero<T>();
    if (x >= max)
        return one<T>();

    T const t = (x - min) / (max - min);
    return t * t * t * (t * (t * T(6) - T(15)) + T(10));
}

} // namespace compages::core
