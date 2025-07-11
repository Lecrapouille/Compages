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

#ifndef OPENGLCPPWRAPPER_RANDOM_HPP
#define OPENGLCPPWRAPPER_RANDOM_HPP

#include "Math/Maths.hpp"
#include <random>

using namespace maths;

namespace dist // distribution
{

static std::mt19937& engine()
{
    static std::random_device seed_gen;
    static std::mt19937 engine(seed_gen());
    return engine;
}

//! \brief Produce random T-typed values, uniformly distributed on the interval
//! [low, high[. If no parameters is given then produce uniformly T-typed
//! values distributed on the interval [0, 1[.
template <typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
real(T const low = zero<T>(), T const high = one<T>())
{
    std::uniform_real_distribution<T> dist(std::min(low, high),
                                           std::max(low, high));
    return dist(dist::engine());
}

//! \brief Random integer-typed values, uniformly distributed on the
//! interval [low, high].
template <typename T>
inline typename std::enable_if<std::is_integral<T>::value &&
                                   std::is_unsigned<T>::value &&
                                   !std::is_floating_point<T>::value,
                               T>::type
integer(T const low = zero<T>(), T const high = one<T>())
{
    return low + static_cast<T>(
                     std::floor(dist::real<float>() *
                                static_cast<float>(high - low + one<T>())));
}

//! \brief Return a Random bool value, either 'true' or 'false'
//! \param[in] p: distribution parameter (probability of generating true)
//! by default: fifty fifty chance.
inline bool boolean(float const p = 0.5f)
{
    std::bernoulli_distribution dist(p);
    return dist(dist::engine());
}

} // namespace dist

#endif
