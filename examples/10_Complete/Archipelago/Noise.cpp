// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Noise.hpp"

#include <algorithm>
#include <cmath>

namespace examples::archipelago
{

// ----------------------------------------------------------------------------
float smoothstep(float p_from, float p_to, float p_x)
{
    const float t = std::clamp((p_x - p_from) / (p_to - p_from), 0.0f, 1.0f);
    return t * t * (3.0f - (2.0f * t));
}

// ----------------------------------------------------------------------------
float lattice(int p_x, int p_y, std::uint32_t p_seed)
{
    auto h = (static_cast<std::uint32_t>(p_x) * 0x8da6b343u) ^
             (static_cast<std::uint32_t>(p_y) * 0xd8163841u) ^
             (p_seed * 0xcb1ab31fu);
    h ^= h >> 13u;
    h *= 0x5bd1e995u;
    h ^= h >> 15u;
    return float(h & 0xffffffu) / float(0xffffffu);
}

// ----------------------------------------------------------------------------
float noise(float p_x, float p_y, std::uint32_t p_seed)
{
    const float fx = std::floor(p_x);
    const float fy = std::floor(p_y);
    const auto x = int(fx);
    const auto y = int(fy);
    const float tx = p_x - fx;
    const float ty = p_y - fy;
    // The quintic fade of Perlin: no crease at the corners.
    const float sx = tx * tx * tx * ((tx * ((tx * 6.0f) - 15.0f)) + 10.0f);
    const float sy = ty * ty * ty * ((ty * ((ty * 6.0f) - 15.0f)) + 10.0f);
    const float a = lattice(x, y, p_seed);
    const float b = lattice(x + 1, y, p_seed);
    const float c = lattice(x, y + 1, p_seed);
    const float d = lattice(x + 1, y + 1, p_seed);
    return a + ((b - a) * sx) + ((c - a) * sy) + ((a - b - c + d) * sx * sy);
}

// ----------------------------------------------------------------------------
float fbm(float p_x, float p_y, std::uint32_t p_seed, int p_octaves)
{
    float sum = 0.0f;
    float amplitude = 0.5f;
    float total = 0.0f;
    for (int i = 0; i < p_octaves; ++i)
    {
        sum += amplitude * noise(p_x, p_y, p_seed + std::uint32_t(i));
        total += amplitude;
        p_x *= 2.03f;
        p_y *= 2.03f;
        amplitude *= 0.5f;
    }
    return sum / total;
}

} // namespace examples::archipelago
