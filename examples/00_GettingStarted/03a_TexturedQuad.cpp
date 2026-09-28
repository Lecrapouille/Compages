// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/03a_TexturedQuad.hpp"

#include <cmath>
#include <vector>

namespace examples
{

//! \brief How many pixels across the checkerboard is, and how large one square of
//! it is.
constexpr std::uint32_t SIZE = 256u;
constexpr std::uint32_t SQUARE = 32u;

constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec2 position;
in vec2 uv;

uniform float scale;

out vec2 vUV;

void main()
{
    vUV = uv * scale;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec2 vUV;

uniform sampler2D image;

out vec4 oColor;

void main()
{
    oColor = texture(image, vUV);
}
)";

std::string TexturedQuad::description() const
{
    return "A checkerboard computed in C++, sent once, and read by the shader. "
           "The texture is given to the sampler by name, like the uniform scale: "
           "quad[\"image\"] = texture. The corners are drawn as a triangle strip, "
           "part of the render state of the drawable. Nearest filtering and "
           "repeating wrap, so the squares stay square and the image tiles as it "
           "zooms.";
}

compages::gpu::Status TexturedQuad::makeTexture()
{
    // A 2D image of four bytes a pixel. Nearest, because a checkerboard is data
    // with edges rather than a photograph: mixing neighbours would invent grey
    // where the design says black or white. Repeat, so that it tiles.
    COMPAGES_TRY(m_texture.allocate(compages::gpu::TextureDesc::image(SIZE, SIZE)
                                        .filter(compages::gpu::Filter::Nearest)
                                        .wrap(compages::gpu::Wrap::Repeat)));

    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(SIZE) * SIZE * 4u, 0u);
    for (std::uint32_t y = 0u; y < SIZE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIZE; ++x)
        {
            const bool light = (((x / SQUARE) + (y / SQUARE)) % 2u) == 0u;
            // A gradient over the checkerboard, so that the tiling is visible: with
            // flat squares alone, one copy of the texture looks like the next.
            const auto ramp = static_cast<std::uint8_t>((x * 255u) / SIZE);

            const std::size_t at =
                ((static_cast<std::size_t>(y) * SIZE) + x) * 4u;
            pixels[at] = light ? 230u : 40u;
            pixels[at + 1u] = ramp;
            pixels[at + 2u] = light ? 90u : 160u;
            pixels[at + 3u] = 255u;
        }
    }

    return m_texture.write(std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

compages::gpu::Status TexturedQuad::setUp()
{
    COMPAGES_TRY(m_quad.load(VERTEX_SHADER, FRAGMENT_SHADER));
    COMPAGES_TRY(makeTexture());

    // The order a triangle strip reads: bottom left, bottom right, top left, top
    // right. Going round the outline instead is what turns a strip into a bow tie.
    m_quad["position"] = { { -0.9f, -0.9f }, { 0.9f, -0.9f }, { -0.9f, 0.9f }, { 0.9f, 0.9f } };
    m_quad["uv"]       = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
    m_quad["image"]    = m_texture;
    m_quad.primitive(compages::gpu::Primitive::TriangleStrip);
    return m_quad.prepare();
}

void TexturedQuad::draw(Frame const& p_frame)
{
    // Between one and three copies of the texture across the quad, which is what
    // makes the repeating wrap visible. A uniform keeps its value until written
    // again, so this is the only one set per frame.
    m_quad["scale"] = 2.0f + std::sin(p_frame.total * 0.7f);

    compages::gpu::clear({ 0.05f, 0.05f, 0.08f });
    m_quad.draw();
}

} // namespace examples
