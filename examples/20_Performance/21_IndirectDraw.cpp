// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "20_Performance/21_IndirectDraw.hpp"

#include <cmath>
#include <random>
#include <vector>

namespace examples
{

constexpr std::uint32_t SIDE = 128u;
constexpr std::uint32_t COUNT = SIDE * SIDE;

constexpr const char* CULL = R"(#version 450 core
layout(local_size_x = 64) in;

struct Dot
{
    vec2 position;
};

layout(std430, binding = 0) readonly buffer All
{
    Dot incoming[];
};

layout(std430, binding = 1) writeonly buffer Kept
{
    Dot outgoing[];
};

layout(std430, binding = 2) buffer Command
{
    uint vertex_count;
    uint instance_count;
    uint first_vertex;
    uint first_instance;
};

uniform vec2 center;
uniform float radius;
uniform uint count;

void main()
{
    // One point. Outside the circle it writes nothing, so the count stays
    // the number of survivors and not the size of the grid.
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }

    vec2 p = incoming[i].position;
    vec2 d = p - center;
    if (dot(d, d) > radius * radius)
    {
        return;
    }

    uint slot = atomicAdd(vertex_count, 1u);
    outgoing[slot].position = p;
}
)";

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;

void main()
{
    gl_PointSize = 4.0;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
out vec4 oColor;

void main()
{
    // Drop the corners of the square point, so a survivor reads as a dot.
    vec2 fromCentre = gl_PointCoord - vec2(0.5);
    if (dot(fromCentre, fromCentre) > 0.25)
    {
        discard;
    }
    oColor = vec4(0.95, 0.85, 0.35, 1.0);
}
)";

std::string IndirectDraw::description() const
{
    return "Sixteen thousand points, and a circle that keeps some of them. A "
           "compute pass writes the survivors into a second buffer and counts "
           "them into the four words a drawIndirect command is. The CPU resets "
           "the count to zero; it never reads how many survived.";
}

compages::Status IndirectDraw::setUp()
{
    // A jittered grid of points covering the screen.
    std::vector<Dot> dots;
    dots.reserve(COUNT);
    std::mt19937 rng(21u);
    std::uniform_real_distribution<float> jitter(-0.35f, 0.35f);
    const float step = 2.0f / static_cast<float>(SIDE);
    for (std::uint32_t y = 0u; y < SIDE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            dots.emplace_back(Dot{ compages::core::Vector2f(
                -1.0f + ((float(x) + 0.5f + jitter(rng)) * step),
                -1.0f + ((float(y) + 0.5f + jitter(rng)) * step)) });
        }
    }

    // Every point, the survivors, and the command: all written by the compute
    // pass or read by it, so all storage.
    COMPAGES_TRY_ASSIGN(
        m_all,
        compages::gpu::Buffer<Dot>::from(dots,
                                         compages::gpu::BufferKind::Storage,
                                         compages::gpu::BufferUsage::Storage));
    COMPAGES_TRY_ASSIGN(m_kept,
                        compages::gpu::Buffer<Dot>::create(
                            COUNT,
                            compages::gpu::BufferKind::Storage,
                            compages::gpu::BufferUsage::Storage));
    COMPAGES_TRY_ASSIGN(
        m_command,
        compages::gpu::Buffer<compages::gpu::DrawIndirectCommand>::create(
            1u,
            compages::gpu::BufferKind::Storage,
            compages::gpu::BufferUsage::Storage));

    COMPAGES_TRY(m_cull.load(CULL));
    COMPAGES_TRY(m_cull.bind("All", m_all));
    COMPAGES_TRY(m_cull.bind("Kept", m_kept));
    COMPAGES_TRY(m_cull.bind("Command", m_command));
    m_cull.set("count", COUNT);

    COMPAGES_TRY(m_kept_points.load(VERTEX, FRAGMENT));
    m_kept_points.vertices(m_kept);
    m_kept_points.primitive(compages::gpu::Primitive::Points);
    return m_kept_points.prepare();
}

void IndirectDraw::draw(compages::world::ViewFrame const& p_frame)
{
    // The circle follows the mouse, or wanders by itself when the mouse is
    // away, so the cull shows as a moving window in the field.
    const compages::core::Vector2f center =
        p_frame.input.mouse_over
            ? compages::world::mouseInClipSpace(p_frame)
            : compages::core::Vector2f(0.55f * std::sin(p_frame.total * 0.7f),
                                       0.40f * std::cos(p_frame.total * 0.5f));

    // No survivor yet: the compute pass counts them into vertex_count.
    m_command.write(compages::gpu::DrawIndirectCommand{ 0u, 1u, 0u, 0u }, 0u);
    m_cull.set("center", center);
    m_cull.set("radius", 0.42f);
    if (!compages::gpu::check(m_cull.dispatchItems(COUNT)))
    {
        return;
    }
    compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib |
                           compages::gpu::Barrier::Command);

    compages::gpu::clear({ 0.03f, 0.03f, 0.05f });
    m_kept_points.drawIndirect(m_command);
}

} // namespace examples
