// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Swells.hpp"
#include "10_Complete/Archipelago/Map.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <iterator>
#include <numbers>

namespace examples::archipelago
{

// ----------------------------------------------------------------------------
std::string swellsGlsl()
{
    std::string text;
    auto out = std::back_inserter(text);
    std::format_to(out,
                   "const int SWELL_COUNT = {};\n"
                   "const vec4 SWELLS[SWELL_COUNT] = vec4[SWELL_COUNT](",
                   SWELLS.size());
    for (std::size_t i = 0u; i < SWELLS.size(); ++i)
    {
        std::format_to(out,
                       "{}vec4({:f}, {:f}, {:f}, {:f})",
                       (i == 0u) ? "" : ", ",
                       SWELLS[i].x,
                       SWELLS[i].z,
                       SWELLS[i].length,
                       SWELLS[i].steepness);
    }
    std::format_to(out,
                   ");\nconst float GRAVITY = {:f};\nconst float PI = {:f};\n"
                   "const float MAP_SIZE = {:f};\n",
                   GRAVITY,
                   std::numbers::pi_v<float>,
                   MAP);
    return text;
}

// ----------------------------------------------------------------------------
float swellHeight(float p_x, float p_z, float p_depth, float p_time)
{
    const float calm = std::clamp(p_depth / (0.08f * SEABED), 0.0f, 1.0f);
    float height = 0.0f;
    for (std::size_t i = 0u; i < SWELLS.size(); ++i)
    {
        Swell const& swell = SWELLS[i];
        const float length = std::hypot(swell.x, swell.z);
        const float k = 2.0f * std::numbers::pi_v<float> / swell.length;
        const float phase =
            (k * (((swell.x * p_x) + (swell.z * p_z)) / length)) -
            (std::sqrt(GRAVITY * k) * p_time) + (float(i) * 1.7f);
        height += calm * (swell.steepness / k) * std::sin(phase);
    }
    return height;
}

} // namespace examples::archipelago
