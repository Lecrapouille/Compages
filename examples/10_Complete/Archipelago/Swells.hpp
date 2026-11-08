// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <array>
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
//! boats. A wide fan, wavelengths that share no ratio: the sum never
//! repeats into stripes. Each one runs at the speed of deep water waves,
//! omega^2 = g k: the swell of 233 m at 19 m/s, a period of 12 s, 1.7 m
//! high.
inline constexpr std::array SWELLS = { Swell{ 0.80f, 0.60f, 233.0f, 0.045f },
                                       Swell{ 0.20f, 0.98f, 151.0f, 0.040f },
                                       Swell{ 0.98f, -0.20f, 97.0f, 0.040f },
                                       Swell{ 0.55f, 0.83f, 61.0f, 0.035f },
                                       Swell{ -0.30f, 0.95f, 39.0f, 0.030f },
                                       Swell{ 0.90f, 0.43f, 27.0f, 0.030f },
                                       Swell{ 0.10f, -0.99f, 17.0f, 0.025f },
                                       Swell{ 0.71f, 0.71f, 11.0f, 0.020f } };

//! \brief The acceleration of gravity, m/s2.
inline constexpr float GRAVITY = 9.81f;

//! \brief The swells and the constants of the map as GLSL, for the shaders.
[[nodiscard]] std::string swellsGlsl();

//! \brief The height of the swell at a point of the sea, as the vertex
//! shader moves the surface (without its sideways motion).
//! \param[in] p_depth The depth of the sea there (m): the swell dies in
//! the shallows.
//! \param[in] p_time Seconds.
[[nodiscard]] float
swellHeight(float p_x, float p_z, float p_depth, float p_time);

} // namespace examples::archipelago
