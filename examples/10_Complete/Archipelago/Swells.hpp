// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace examples::archipelago
{

//! \brief A swell of the sea: its direction (x, z), its wavelength (m), its
//! steepness (amplitude times k).
struct Swell
{
    float x;
    float z;
    float length;
    float steepness;
};

//! \brief The swells of the sea, the same for the shaders and for the
//! boats. One wind blows them, toward (0.8, 0.6): the long swells follow it
//! closely, the short chop spreads wider, at angles with no rule (0, -14,
//! +21, -33, +44, -52, +68, -75, +95, -100 degrees). Wavelengths that share
//! no ratio: the sum never repeats into stripes. Each one runs at the speed
//! of deep water waves, omega^2 = g k: the swell of 233 m at 19 m/s, a
//! period of 12 s, 2.3 m high on average. The last two are a short chop, in
//! the light only (shorter than three quads of the surface).
inline constexpr std::array SWELLS = { Swell{ 0.80f, 0.60f, 233.0f, 0.062f },
                                       Swell{ 0.92f, 0.39f, 151.0f, 0.055f },
                                       Swell{ 0.53f, 0.85f, 97.0f, 0.055f },
                                       Swell{ 1.00f, 0.07f, 61.0f, 0.048f },
                                       Swell{ 0.16f, 0.99f, 39.0f, 0.041f },
                                       Swell{ 0.97f, -0.26f, 27.0f, 0.041f },
                                       Swell{ -0.26f, 0.97f, 17.0f, 0.034f },
                                       Swell{ 0.79f, -0.62f, 11.0f, 0.027f },
                                       Swell{ -0.67f, 0.74f, 7.0f, 0.020f },
                                       Swell{ 0.45f, -0.89f, 4.6f, 0.015f } };

//! \brief Waves come in groups: a series of tall ones, then calmer water.
//! The height of each swell is multiplied by GROUP_LOW + GROUP_SPAN times a
//! smooth noise (0 to 1, so 1 on average), about GROUP_SIZE wavelengths
//! wide, which drifts at the group speed of deep water (half the speed of
//! the waves). Without it, each swell is equally tall everywhere and their
//! sum looks like a regular grid. At most 1.7 times as tall: with the
//! Gerstner factor (0.75), the crests never loop (0.4 x 1.7 x 0.75 < 1).
inline constexpr float GROUP_LOW = 0.3f;
inline constexpr float GROUP_SPAN = 1.4f;
inline constexpr float GROUP_SIZE = 3.5f;
//! \brief The noise of swell i uses the seed GROUP_SEED + i.
inline constexpr unsigned GROUP_SEED = 200u;

//! \brief The acceleration of gravity, m/s2.
inline constexpr float GRAVITY = 9.81f;

//! \brief The height factor of swell \p p_i at a point (see GROUP_LOW).
[[nodiscard]] float
swellGroup(std::size_t p_i, float p_x, float p_z, float p_time);

//! \brief The swells and the constants of the map as GLSL, for the shaders,
//! with the same noise and groups as the C++ side (swellNoise(),
//! swellGroup()), so that the boats ride the water drawn.
[[nodiscard]] std::string swellsGlsl();

//! \brief The height of the swell at a point of the sea, as the vertex
//! shader moves the surface (without its sideways motion).
//! \param[in] p_depth The depth of the sea there (m): the swell dies in
//! the shallows.
//! \param[in] p_time Seconds.
[[nodiscard]] float
swellHeight(float p_x, float p_z, float p_depth, float p_time);

} // namespace examples::archipelago
