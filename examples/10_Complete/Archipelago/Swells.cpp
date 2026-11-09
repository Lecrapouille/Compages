// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Swells.hpp"
#include "10_Complete/Archipelago/Map.hpp"
#include "10_Complete/Archipelago/Noise.hpp"

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
    std::format_to(
        out,
        ");\nconst float GRAVITY = {:f};\nconst float PI = {:f};\n"
        "const float MAP_SIZE = {:f};\n"
        "const float GROUP_LOW = {:f};\nconst float GROUP_SPAN = {:f};\n"
        "const float GROUP_SIZE = {:f};\nconst uint GROUP_SEED = {}u;\n",
        GRAVITY,
        std::numbers::pi_v<float>,
        MAP,
        GROUP_LOW,
        GROUP_SPAN,
        GROUP_SIZE,
        GROUP_SEED);
    // The value noise of Noise.cpp, bit for bit: the same integer hash (a
    // negative int keeps its bits as a uint, as in C++).
    text += R"(
float swellLattice(int x, int y, uint seed)
{
    uint h = (uint(x) * 0x8da6b343u) ^ (uint(y) * 0xd8163841u) ^ (seed * 0xcb1ab31fu);
    h ^= h >> 13u;
    h *= 0x5bd1e995u;
    h ^= h >> 15u;
    return float(h & 0xffffffu) / float(0xffffffu);
}

float swellNoise(vec2 p, uint seed)
{
    vec2 f = floor(p);
    ivec2 i = ivec2(f);
    vec2 t = p - f;
    vec2 s = t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    float a = swellLattice(i.x, i.y, seed);
    float b = swellLattice(i.x + 1, i.y, seed);
    float c = swellLattice(i.x, i.y + 1, seed);
    float d = swellLattice(i.x + 1, i.y + 1, seed);
    return a + (b - a) * s.x + (c - a) * s.y + (a - b - c + d) * s.x * s.y;
}

float swellGroup(int i, vec2 p, float t)
{
    vec2 d = normalize(SWELLS[i].xy);
    float k = 2.0 * PI / SWELLS[i].z;
    float cg = 0.5 * sqrt(GRAVITY / k);
    return GROUP_LOW + GROUP_SPAN * swellNoise((p - d * (cg * t)) / (GROUP_SIZE * SWELLS[i].z),
                                               GROUP_SEED + uint(i));
}
)";
    return text;
}

// ----------------------------------------------------------------------------
float swellGroup(std::size_t p_i, float p_x, float p_z, float p_time)
{
    Swell const& swell = SWELLS[p_i];
    const float length = std::hypot(swell.x, swell.z);
    const float k = 2.0f * std::numbers::pi_v<float> / swell.length;
    const float drift = 0.5f * std::sqrt(GRAVITY / k) * p_time;
    const float size = GROUP_SIZE * swell.length;
    return GROUP_LOW +
           (GROUP_SPAN * noise((p_x - (swell.x / length * drift)) / size,
                               (p_z - (swell.z / length * drift)) / size,
                               GROUP_SEED + unsigned(p_i)));
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
        height += calm * swellGroup(i, p_x, p_z, p_time) *
                  (swell.steepness / k) * std::sin(phase);
    }
    return height;
}

} // namespace examples::archipelago
