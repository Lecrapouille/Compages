// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_ScientificAndCompute/11b_GrayScott.hpp"

#include <vector>

namespace examples
{

constexpr std::uint32_t SIZE = 256u;
constexpr int STEPS_PER_FRAME = 8;

constexpr const char* SCREEN_VERTEX = R"(#version 450 core
out vec2 vUV;

void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vec2 position = corners[gl_VertexID];
    vUV = position * 0.5 + 0.5;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* STEP_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D previous;
uniform vec2 texel;
out vec4 oColor;

void main()
{
    // U in red, V in green. The laplacian is this cell against its four
    // neighbours: diffusion, before the reaction.
    vec2 centre = texture(previous, vUV).rg;
    vec2 laplacian = -centre;
    laplacian += texture(previous, vUV + vec2(texel.x, 0.0)).rg;
    laplacian += texture(previous, vUV - vec2(texel.x, 0.0)).rg;
    laplacian += texture(previous, vUV + vec2(0.0, texel.y)).rg;
    laplacian += texture(previous, vUV - vec2(0.0, texel.y)).rg;

    // Feed U, kill V, and turn U into V where they meet.
    const float F = 0.037;
    const float K = 0.06;
    const float Du = 0.16;
    const float Dv = 0.08;
    float u = centre.r;
    float v = centre.g;
    float uvv = u * v * v;
    u += (Du * laplacian.r) - uvv + (F * (1.0 - u));
    v += (Dv * laplacian.g) + uvv - ((F + K) * v);
    oColor = vec4(clamp(u, 0.0, 1.0), clamp(v, 0.0, 1.0), 0.0, 1.0);
}
)";

constexpr const char* SHOW_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    float v = texture(image, vUV).g;
    oColor = vec4(0.5 + 0.5 * cos(3.2 + v * 6.5 + vec3(0.1, 0.7, 1.1)), 1.0);
}
)";

std::string GrayScott::description() const
{
    return "The same ping-pong as Game of Life, holding two concentrations as "
           "32-bit floats rather than a bit per cell. U is fed, V is killed, "
           "and where they meet they make more V. The display pass is what "
           "turns those numbers into a picture; the simulation never heard of "
           "a colour.";
}

compages::gpu::Status GrayScott::setUp()
{
    // Two float textures, each wired to a framebuffer. Nearest, because a
    // cell must not blend with its neighbour before the laplacian does.
    for (int i = 0; i < 2; ++i)
    {
        COMPAGES_TRY(m_field[i].allocate({ .format = compages::gpu::PixelFormat::RGBA32F,
                                           .width = SIZE,
                                           .height = SIZE,
                                           .magnify = compages::gpu::Filter::Nearest,
                                           .minify = compages::gpu::Filter::Nearest,
                                           .wrap_x = compages::gpu::Wrap::Repeat,
                                           .wrap_y = compages::gpu::Wrap::Repeat }));
        COMPAGES_TRY(m_target[i].attach(m_field[i]));
    }

    // U everywhere, and a square of V in the middle to start the reaction.
    std::vector<Vector4f> cells(std::size_t{ SIZE } * SIZE, Vector4f(1.0f, 0.0f, 0.0f, 1.0f));
    const std::uint32_t mid = SIZE / 2u;
    for (std::uint32_t y = mid - 12u; y < mid + 12u; ++y)
    {
        for (std::uint32_t x = mid - 12u; x < mid + 12u; ++x)
        {
            cells[(std::size_t{ y } * SIZE) + x] = Vector4f(0.5f, 0.25f, 0.0f, 1.0f);
        }
    }
    COMPAGES_TRY(m_field[0].write(std::as_bytes(std::span<const Vector4f>(cells))));

    COMPAGES_TRY(m_step.load(SCREEN_VERTEX, STEP_FRAGMENT));
    m_step["texel"] = Vector2f(1.0f / float(SIZE), 1.0f / float(SIZE));
    COMPAGES_TRY(m_show.load(SCREEN_VERTEX, SHOW_FRAGMENT));
    return compages::gpu::success();
}

void GrayScott::draw(Frame const&)
{
    // Several steps per frame, so that a pattern shows within seconds.
    for (int i = 0; i < STEPS_PER_FRAME; ++i)
    {
        const int next = 1 - m_current;
        m_step["previous"] = m_field[m_current];
        compages::gpu::RenderPass into(m_target[next], { .clear_color = false,
                                               .clear_depth = false });
        m_step.draw(3u);
        m_current = next;
    }

    m_show["image"] = m_field[m_current];
    m_show.draw(3u);
}

} // namespace examples
