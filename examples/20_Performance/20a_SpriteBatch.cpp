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

#include "20_Performance/20a_SpriteBatch.hpp"

#include <random>
#include <vector>

namespace examples
{

constexpr std::uint32_t SIDE = 320u;
constexpr std::uint32_t ATLAS = 128u;
constexpr std::uint32_t TILES = 4u;
constexpr std::uint32_t TILE_PX = ATLAS / TILES;

constexpr const char* VERTEX = R"(#version 450 core
in vec2 center;
in vec2 extent;
in float tile;
in float phase;

uniform float time;
uniform float aspect;

out vec2 vUV;

void main()
{
    // gl_VertexID is the corner of this instance. The vertex record is the
    // sprite: one of them, read four times.
    vec2 corners[4] = vec2[4](vec2(-1.0, -1.0), vec2(1.0, -1.0),
                              vec2(-1.0, 1.0), vec2(1.0, 1.0));
    vec2 corner = corners[gl_VertexID];
    vec2 uv = (corner + 1.0) * 0.5;

    float col = mod(tile, 4.0);
    float row = floor(tile / 4.0);
    vUV = (vec2(col, row) + uv) / 4.0;

    float wobble = 0.012 * sin(time * 1.6 + phase);
    vec2 place = center + (corner * extent) + vec2(wobble, 0.0);
    gl_Position = vec4(place.x / aspect, place.y, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D atlas;
out vec4 oColor;

void main()
{
    vec4 colour = texture(atlas, vUV);
    // A slight inset so neighbouring sprites do not fuse into a sheet.
    vec2 edge = abs(vUV * 4.0 - floor(vUV * 4.0) - 0.5);
    if (max(edge.x, edge.y) > 0.46)
    {
        discard;
    }
    oColor = colour;
}
)";

//------------------------------------------------------------------------------
std::string SpriteBatch::description() const
{
    return "A hundred thousand sprites, one draw. The four corners are made "
           "from gl_VertexID; the vertices hold one record per sprite, read "
           "once per instance because the layout says perInstance(). The "
           "atlas is sixteen tiles computed here. The overlay should show one "
           "draw call, whatever the count.";
}

//------------------------------------------------------------------------------
compages::gpu::Status SpriteBatch::makeAtlas()
{
    COMPAGES_TRY(m_atlas.allocate({ .width = ATLAS,
                                    .height = ATLAS,
                                    .magnify = compages::gpu::Filter::Nearest,
                                    .minify = compages::gpu::Filter::Nearest }));

    // Sixteen tiles, each a colour of its own.
    std::vector<std::uint8_t> pixels(std::size_t{ ATLAS } * ATLAS * 4u);
    for (std::uint32_t y = 0u; y < ATLAS; ++y)
    {
        for (std::uint32_t x = 0u; x < ATLAS; ++x)
        {
            const std::uint32_t tile = ((y / TILE_PX) * TILES) + (x / TILE_PX);
            const float t = static_cast<float>(tile) / 15.0f;
            std::uint8_t* pixel = &pixels[((std::size_t{ y } * ATLAS) + x) * 4u];
            pixel[0] = static_cast<std::uint8_t>(40.0f + (200.0f * t));
            pixel[1] = static_cast<std::uint8_t>(180.0f - (120.0f * t));
            pixel[2] = static_cast<std::uint8_t>(80.0f + (140.0f * (1.0f - t)));
            pixel[3] = 255u;
        }
    }
    return m_atlas.write(std::as_bytes(std::span<const std::uint8_t>(pixels)));
}

//------------------------------------------------------------------------------
compages::gpu::Status SpriteBatch::setUp()
{
    COMPAGES_TRY(makeAtlas());

    // One record per sprite, on a grid. The four corners are not stored.
    std::vector<Sprite> sprites;
    sprites.reserve(std::size_t{ SIDE } * SIDE);
    std::mt19937 rng(7u);
    std::uniform_real_distribution<float> jitter(-0.35f, 0.35f);
    std::uniform_int_distribution<int> tile(0, 15);
    const float step = 2.0f / static_cast<float>(SIDE);
    for (std::uint32_t y = 0u; y < SIDE; ++y)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            sprites.emplace_back(Sprite{
                Vector2f(-1.0f + ((float(x) + 0.5f) * step), -1.0f + ((float(y) + 0.5f) * step)),
                Vector2f(step * 0.42f, step * 0.42f),
                static_cast<float>(tile(rng)),
                jitter(rng) * 8.0f });
        }
    }

    COMPAGES_TRY(m_sprites.load(VERTEX, FRAGMENT));
    // perInstance: the record advances once per sprite, not once per corner.
    m_sprites.vertices(sprites, compages::gpu::VertexLayout::of<Sprite>().perInstance());
    m_sprites.usage(compages::gpu::BufferUsage::Immutable);
    m_sprites.primitive(compages::gpu::Primitive::TriangleStrip);
    m_sprites["atlas"] = m_atlas;
    return m_sprites.prepare();
}

//------------------------------------------------------------------------------
void SpriteBatch::draw(Frame const& p_frame)
{
    // Time wobbles the sprites; aspect keeps them square when the window is not.
    m_sprites["time"] = p_frame.total;
    m_sprites["aspect"] = aspect(p_frame);

    compages::gpu::clear({ 0.04f, 0.04f, 0.06f });
    m_sprites.drawInstanced(m_sprites.count(), 4u);
}

} // namespace examples
