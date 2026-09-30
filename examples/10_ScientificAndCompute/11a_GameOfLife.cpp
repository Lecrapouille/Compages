// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_ScientificAndCompute/11a_GameOfLife.hpp"

#include <cstdint>
#include <random>
#include <vector>

namespace examples
{

constexpr std::uint32_t SIZE = 256u;

//! \brief A triangle covering the target, made from gl_VertexID: no vertices.
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
    // The eight cells around this one. The centre is the cell itself, counted
    // apart so a live cell is not its own neighbour.
    int neighbours = 0;
    for (int y = -1; y <= 1; ++y)
    {
        for (int x = -1; x <= 1; ++x)
        {
            if ((x == 0) && (y == 0))
            {
                continue;
            }
            neighbours += int(texture(previous, vUV + (vec2(x, y) * texel)).r > 0.5);
        }
    }

    // Stay alive with two or three neighbours, be born with three.
    bool alive = texture(previous, vUV).r > 0.5;
    bool next = alive ? ((neighbours == 2) || (neighbours == 3))
                      : (neighbours == 3);
    oColor = vec4(next ? 1.0 : 0.0);
}
)";

constexpr const char* SHOW_FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D image;
out vec4 oColor;

void main()
{
    float cell = texture(image, vUV).r;
    oColor = vec4(vec3(0.06, 0.08, 0.1) + (vec3(0.35, 0.85, 0.45) * cell), 1.0);
}
)";

std::string GameOfLife::description() const
{
    return "Each cell is a pixel. A shader covering the texture reads this "
           "generation and writes the next into a second texture; the two "
           "swap. A shader cannot read the picture it is writing, which is why "
           "there are two of everything. The world wraps around its edges.";
}

compages::Status GameOfLife::setUp()
{
    // Two textures of one byte per cell, each wired to a framebuffer so that
    // a pass can write into it. Nearest: a cell is a cell, never a blend.
    // Repeat: reading past an edge reads the other side.
    for (int i = 0; i < 2; ++i)
    {
        COMPAGES_TRY(
            m_field[i].allocate({ .format = compages::gpu::PixelFormat::R8,
                                  .width = SIZE,
                                  .height = SIZE,
                                  .magnify = compages::gpu::Filter::Nearest,
                                  .minify = compages::gpu::Filter::Nearest,
                                  .wrap_x = compages::gpu::Wrap::Repeat,
                                  .wrap_y = compages::gpu::Wrap::Repeat }));
        COMPAGES_TRY(m_target[i].attach(m_field[i]));
    }

    // One cell in five alive, at random.
    std::vector<std::uint8_t> cells(std::size_t{ SIZE } * SIZE);
    std::mt19937 rng(2026u);
    std::uniform_int_distribution<int> coin(0, 4);
    for (std::uint8_t& cell : cells)
    {
        cell = (coin(rng) == 0) ? 255u : 0u;
    }
    COMPAGES_TRY(
        m_field[0].write(std::as_bytes(std::span<const std::uint8_t>(cells))));

    COMPAGES_TRY(m_step.load(SCREEN_VERTEX, STEP_FRAGMENT));
    m_step["texel"] =
        compages::core::Vector2f(1.0f / float(SIZE), 1.0f / float(SIZE));
    COMPAGES_TRY(m_show.load(SCREEN_VERTEX, SHOW_FRAGMENT));
    return compages::success();
}

void GameOfLife::draw(compages::world::ViewFrame const&)
{
    // Read the current generation, write the next one into the other texture.
    // The pass takes the size of its target, not of the window.
    const int next = 1 - m_current;
    m_step["previous"] = m_field[m_current];
    {
        compages::gpu::RenderPass into(
            m_target[next], { .clear_color = false, .clear_depth = false });
        m_step.draw(3u);
    }
    m_current = next;

    m_show["image"] = m_field[m_current];
    m_show.draw(3u);
}

} // namespace examples
