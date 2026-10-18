// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_ScientificAndCompute/13_Galaxy.hpp"

#include "Common/Gui.hpp"

#include "Compages/GPU/Errors.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <numbers>
#include <random>
#include <vector>

namespace examples
{

constexpr std::uint32_t COUNT = 8192u;
//! \brief G times the mass of one star, of the halo, and the halo radius.
constexpr float G = 0.000004f;
constexpr float SOFTENING = 0.003f;
constexpr float HALO_MASS = 0.3f;
constexpr float HALO_CORE = 0.2f;

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
uniform float haloMass;   // G times the mass of the dark halo
uniform float haloCore;   // its radius: inside, the pull fades to zero

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

    // The dark matter halo: a fixed ball of mass around the centre (Plummer).
    // Its pull is what keeps a thin disk of stars from breaking into clumps.
    float h2 = dot(me.position, me.position) + haloCore * haloCore;
    acc -= me.position * (haloMass / (h2 * sqrt(h2)));

    me.velocity += acc * dt;
    me.position += me.velocity * dt;
    outgoing[i] = me;
}
)";

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
in vec4 color;
uniform float aspect;   // width / height: a circle stays round
out vec4 vColor;

void main()
{
    vColor = color;
    gl_PointSize = 2.2;
    gl_Position = vec4(position.x / aspect, position.y, 0.0, 1.0);
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

std::string Galaxy::description() const
{
    return "Eight thousand stars pull on each other, inside a dark matter "
           "halo. Each work group loads a tile of positions into shared memory "
           "and reuses it. A PingPong swaps the two storage buffers: one is "
           "read, the other written, then they change roles. Save writes the "
           "stars to galaxy.csv.";
}

std::vector<Galaxy::Star> Galaxy::seed()
{
    // A disk, densest at the centre, with two arms. Each star starts on a
    // circular orbit: its speed balances the pull of what is inside its
    // radius (the stars nearer the centre, and the halo). Slower, the disk
    // falls in; faster, it flies apart.
    std::vector<Star> stars(COUNT);
    std::mt19937 rng(14u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    std::normal_distribution<float> jitter(0.0f, 1.0f);
    std::vector<float> radii(COUNT);
    for (float& radius : radii)
    {
        radius = std::min(
            0.04f - (0.22f * std::log(1.0f - (0.98f * unit(rng)))), 0.9f);
    }
    std::vector<float> sorted = radii;
    std::sort(sorted.begin(), sorted.end());

    for (std::uint32_t i = 0u; i < COUNT; ++i)
    {
        const float radius = radii[i];
        const float arm = (i % 2u == 0u) ? 0.0f : std::numbers::pi_v<float>;
        const float angle =
            arm + (std::log(radius) * 2.2f) + (0.5f * (unit(rng) - 0.5f));
        const compages::core::Vector2f at(radius * std::cos(angle),
                                          radius * std::sin(angle));

        // Pull toward the centre, as the shader computes it.
        const float inside =
            float(std::lower_bound(sorted.begin(), sorted.end(), radius) -
                  sorted.begin());
        const float d2 = (radius * radius) + SOFTENING;
        const float h2 = (radius * radius) + (HALO_CORE * HALO_CORE);
        const float pull = (G * inside * radius / (d2 * std::sqrt(d2))) +
                           (HALO_MASS * radius / (h2 * std::sqrt(h2)));
        // Some disorder, as in a real disk: a perfectly cold disk breaks
        // into clumps within one turn (Toomre).
        const float speed = std::sqrt(pull * radius);
        const compages::core::Vector2f around(-at.y / radius, at.x / radius);
        const compages::core::Vector2f out(at.x / radius, at.y / radius);
        const compages::core::Vector2f velocity =
            (around * (speed * (1.0f + (0.12f * jitter(rng))))) +
            (out * (speed * 0.12f * jitter(rng)));

        stars[i].position = at;
        stars[i].velocity = velocity;
        stars[i].color = compages::core::Vector4f(0.55f + (0.45f * unit(rng)),
                                                  0.55f + (0.25f * unit(rng)),
                                                  0.95f,
                                                  0.85f);
    }
    return stars;
}

compages::Status Galaxy::setUp()
{
    const std::vector<Star> stars = seed();
    COMPAGES_TRY_ASSIGN(
        m_stars,
        compages::gpu::PingPong<Star>::from(std::span<const Star>(stars)));

    COMPAGES_TRY(m_step.load(STEP));
    // Gravity and the halo stay put; only the time step changes per frame.
    m_step.set("count", COUNT);
    m_step.set("G", G);
    m_step.set("softening", SOFTENING);
    m_step.set("haloMass", HALO_MASS);
    m_step.set("haloCore", HALO_CORE);

    COMPAGES_TRY(m_points.load(VERTEX, FRAGMENT));
    m_points.vertices(m_stars.input());
    m_points["aspect"] = 1.0f;
    m_points.primitive(compages::gpu::Primitive::Points)
        .blend(compages::gpu::Blend::additive());
    return m_points.prepare();
}

void Galaxy::draw(compages::world::ViewFrame const& p_frame)
{
    if (m_restart)
    {
        m_restart = false;
        const std::vector<Star> stars = seed();
        auto fresh =
            compages::gpu::PingPong<Star>::from(std::span<const Star>(stars));
        if (fresh)
        {
            m_stars = fresh.take();
            m_time = 0.0f;
        }
        else
        {
            compages::gpu::reportError(fresh.error());
        }
    }

    if (m_running)
    {
        // Ping-pong: read input(), write output(), wait until the writes are
        // visible, then swap. What was just written becomes input(): the
        // buffer the draw reads, and the next step reads.
        const float dt = std::min(p_frame.elapsed, 1.0f / 30.0f);
        m_step.set("dt", dt);
        if (!compages::gpu::check(m_step.bind("Input", m_stars.input())) ||
            !compages::gpu::check(m_step.bind("Output", m_stars.output())) ||
            !compages::gpu::check(m_step.dispatchItems(COUNT)))
        {
            return;
        }
        compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib |
                               compages::gpu::Barrier::Storage);
        m_stars.swap();
        m_time += dt;
    }

    compages::gpu::clear({ 0.01f, 0.01f, 0.03f });
    m_points["aspect"] = compages::core::aspect(p_frame);
    m_points.vertices(m_stars.input());
    m_points.draw();
}

void Galaxy::save(std::string const& p_path)
{
    // The stars live on the GPU: read the buffer last written back to the CPU
    // (this waits for the GPU, fine once, not every frame).
    compages::Result<std::vector<Star>> stars = m_stars.input().read();
    if (!stars)
    {
        m_said = "Cannot read the stars: " + stars.error();
        return;
    }
    std::ofstream file(p_path);
    file << "# 13_Galaxy, t = " << m_time << "\n";
    file << "x,y,vx,vy\n";
    for (Star const& star : stars.value())
    {
        file << star.position.x << ',' << star.position.y << ','
             << star.velocity.x << ',' << star.velocity.y << '\n';
    }
    m_said = file ? (std::to_string(stars.value().size()) +
                     " stars written to " + p_path)
                  : ("Cannot write " + p_path);
    std::cout << "[13_Galaxy] " << m_said << std::endl;
}

void Galaxy::controls()
{
    ImGui::Checkbox("Run", &m_running);
    ImGui::SameLine();
    m_restart = ImGui::Button("Restart") || m_restart;
    ImGui::SameLine();
    if (ImGui::Button("Save to galaxy.csv"))
    {
        save("galaxy.csv");
    }
    ImGui::Text("Simulated time: %.1f", double(m_time));
    if (!m_said.empty())
    {
        ImGui::TextWrapped("%s", m_said.c_str());
    }
}

} // namespace examples
