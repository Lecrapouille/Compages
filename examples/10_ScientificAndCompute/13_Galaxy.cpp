//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "10_ScientificAndCompute/13_Galaxy.hpp"

#include <algorithm>
#include <cmath>
#include <random>
#include <vector>

namespace examples
{

constexpr std::uint32_t COUNT = 8192u;

constexpr const char* STEP = R"(#version 450 core
layout(local_size_x = 256) in;

struct Star
{
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(std430, binding = 0) readonly buffer Input
{
    Star incoming[];
};

layout(std430, binding = 1) writeonly buffer Output
{
    Star outgoing[];
};

uniform uint count;
uniform float dt;
uniform float G;
uniform float softening;

shared vec2 tile[256];

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }

    // One star. The tiles below are shared by the whole work group, so each
    // position is loaded from global memory once per tile, not once per star.
    Star me = incoming[i];
    vec2 acc = vec2(0.0);
    uint tiles = (count + 255u) / 256u;

    for (uint t = 0u; t < tiles; ++t)
    {
        uint src = t * 256u + gl_LocalInvocationID.x;
        tile[gl_LocalInvocationID.x] =
            (src < count) ? incoming[src].position : vec2(0.0);
        barrier();

        uint n = min(256u, count - t * 256u);
        for (uint j = 0u; j < n; ++j)
        {
            vec2 r = tile[j] - me.position;
            float d2 = dot(r, r) + softening;
            acc += r * (G / (d2 * sqrt(d2)));
        }
        barrier();
    }

    me.velocity += acc * dt;
    me.position += me.velocity * dt;
    outgoing[i] = me;
}
)";

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
in vec4 color;
out vec4 vColor;

void main()
{
    vColor = color;
    gl_PointSize = 2.2;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec4 vColor;
out vec4 oColor;

void main()
{
    // A soft disc: the corners of the square point are dropped, and the
    // rest fades toward the edge.
    vec2 fromCentre = gl_PointCoord - vec2(0.5);
    float fade = 1.0 - (4.0 * dot(fromCentre, fromCentre));
    if (fade <= 0.0)
    {
        discard;
    }
    oColor = vec4(vColor.rgb * fade, vColor.a * fade);
}
)";

//------------------------------------------------------------------------------
std::string Galaxy::description() const
{
    return "Eight thousand stars pull on each other. Each work group loads a "
           "tile of positions into shared memory and reuses it, so the "
           "neighbour loop is not a thousand trips to global memory. A "
           "PingPong swaps the two storage buffers; the one just written is "
           "what the drawable reads.";
}

//------------------------------------------------------------------------------
compages::Status Galaxy::setUp()
{
    // Two spiral arms, each star on a rough circular orbit so that the galaxy
    // does not collapse in its first frames.
    std::vector<Star> seed(COUNT);
    std::mt19937 rng(14u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (std::uint32_t i = 0u; i < COUNT; ++i)
    {
        const float arm = (i % 2u == 0u) ? 0.0f : 3.14159f;
        const float radius = 0.12f + (0.72f * std::sqrt(unit(rng)));
        const float angle = arm + (radius * 3.4f) + (0.15f * unit(rng));
        const float x = radius * std::cos(angle);
        const float y = radius * std::sin(angle) * 0.62f;
        const float speed = 0.22f / std::sqrt(radius + 0.08f);
        seed[i].position = compages::core::Vector2f(x, y);
        seed[i].velocity = compages::core::Vector2f(-y * speed, x * speed);
        seed[i].color = compages::core::Vector4f(0.55f + (0.45f * unit(rng)),
                                                 0.55f + (0.25f * unit(rng)),
                                                 0.95f,
                                                 0.85f);
    }
    COMPAGES_TRY_ASSIGN(
        m_stars,
        compages::gpu::PingPong<Star>::from(std::span<const Star>(seed)));

    COMPAGES_TRY(m_step.load(STEP));
    // Gravity and softening stay put; only the time step changes per frame.
    m_step.set("count", COUNT);
    m_step.set("G", 0.000012f);
    m_step.set("softening", 0.0008f);

    COMPAGES_TRY(m_points.load(VERTEX, FRAGMENT));
    m_points.vertices(m_stars.input());
    m_points.primitive(compages::gpu::Primitive::Points)
        .blend(compages::gpu::Blend::additive());
    return m_points.prepare();
}

//------------------------------------------------------------------------------
void Galaxy::draw(compages::world::ViewFrame const& p_frame)
{
    // Read one buffer, write the other, then swap: what was written is read
    // by the draw and by the next step.
    m_step.set("dt", std::min(p_frame.elapsed, 1.0f / 30.0f));
    if (!compages::gpu::check(m_step.bind("Input", m_stars.input())) ||
        !compages::gpu::check(m_step.bind("Output", m_stars.output())) ||
        !compages::gpu::check(m_step.dispatchItems(COUNT)))
    {
        return;
    }
    compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib |
                           compages::gpu::Barrier::Storage);
    m_stars.swap();

    compages::gpu::clear({ 0.01f, 0.01f, 0.03f });
    m_points.vertices(m_stars.input());
    m_points.draw();
}

} // namespace examples
