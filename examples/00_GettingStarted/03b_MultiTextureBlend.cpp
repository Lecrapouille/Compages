// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/03b_MultiTextureBlend.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"

#include <algorithm>
#include <cmath>
#include <span>

namespace examples
{

constexpr char const* VERTEX = R"(#version 450 core
in vec2 position;
in vec2 uv;
out vec2 vUV;
void main()
{
    // Once across the quad: the blend map is a map of the whole ground, and
    // repeating it is what made the same pattern show four times.
    vUV = uv;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr char const* FRAGMENT = R"(#version 450 core
in vec2 vUV;
uniform sampler2D backgroundTexture;
uniform sampler2D rTexture;
uniform sampler2D gTexture;
uniform sampler2D bTexture;
uniform sampler2D blendMap;
out vec4 oColor;
void main()
{
    // Each channel of the blend map is the weight of one material. What is
    // left over is the background.
    vec4 blend = texture(blendMap, vUV);
    float backAmount = 1.0 - (blend.r + blend.g + blend.b);
    // The materials are small patches of ground: stretched over the whole
    // quad they would be a blur, so they are read a few times across it.
    vec2 detail = vUV * 6.0;
    vec4 colour = texture(backgroundTexture, detail) * backAmount;
    colour += texture(rTexture, detail) * blend.r;
    colour += texture(gTexture, detail) * blend.g;
    colour += texture(bTexture, detail) * blend.b;
    oColor = colour;
}
)";

//! \brief The sampler each texture file feeds, in the order of m_textures.
constexpr std::array<std::pair<char const*, char const*>, 5u> MAPS{ {
    { "blendMap", "blendMap.png" },
    { "backgroundTexture", "grassy2.png" },
    { "rTexture", "mud.png" },
    { "gTexture", "grassFlowers.png" },
    { "bTexture", "path.png" },
} };

std::string MultiTextureBlend::description() const
{
    return "A blend map mixes four ground materials on one quad: its red, "
           "green "
           "and blue say how much mud, flowers and path there is at each "
           "place, "
           "and what is left is grass. Five textures, one draw.\n\n"
           "Drag with the left button on the picture to paint the blend map. "
           "In "
           "Try it, pick the layer (mud, flowers, path, or erase back to "
           "grass).";
}

compages::Status MultiTextureBlend::setUp()
{
    COMPAGES_TRY(m_plane.load(VERTEX, FRAGMENT));
    m_plane["position"] = {
        { -0.9f, -0.9f }, { 0.9f, -0.9f }, { -0.9f, 0.9f }, { 0.9f, 0.9f }
    };
    m_plane["uv"] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
    m_plane.primitive(compages::gpu::Primitive::TriangleStrip);

    for (std::size_t i = 0u; i < MAPS.size(); ++i)
    {
        const std::string path = dataPath(MAPS[i].second);
        if (path.empty())
        {
            return compages::failure("03b_MultiTextureBlend needs textures in "
                                     "external/Compages-data/");
        }
        const compages::gpu::LoadOptions options =
            (i == 0u) ? compages::gpu::LoadOptions::data()
                      : compages::gpu::LoadOptions::picture();
        COMPAGES_TRY(m_textures[i].load(path, options));
        m_plane[MAPS[i].first] = m_textures[i];
    }

    m_blend_width = m_textures[0u].width();
    m_blend_height = m_textures[0u].height();
    std::vector<std::byte> pixels;
    COMPAGES_TRY_ASSIGN(pixels, m_textures[0u].read());
    m_blend_pixels.resize(pixels.size());
    for (std::size_t i = 0u; i < pixels.size(); ++i)
    {
        m_blend_pixels[i] = static_cast<std::uint8_t>(pixels[i]);
    }
    return m_plane.prepare();
}

void MultiTextureBlend::paintAt(float p_u, float p_v)
{
    if ((m_blend_width == 0u) || (m_blend_height == 0u))
    {
        return;
    }
    const int center_x = int(p_u * float(m_blend_width));
    const int center_y = int(p_v * float(m_blend_height));
    const int radius = int(std::max(1.0f, m_brush));
    const int min_x = std::max(0, center_x - radius);
    const int max_x = int(m_blend_width) - 1;
    const int min_y = std::max(0, center_y - radius);
    const int max_y = int(m_blend_height) - 1;
    const float radius_sq = float(radius * radius);

    for (int y = min_y; y <= std::min(center_y + radius, max_y); ++y)
    {
        for (int x = min_x; x <= std::min(center_x + radius, max_x); ++x)
        {
            const float dx = float(x - center_x);
            const float dy = float(y - center_y);
            if ((dx * dx) + (dy * dy) > radius_sq)
            {
                continue;
            }
            const std::size_t at =
                (static_cast<std::size_t>(y) * m_blend_width +
                 static_cast<std::size_t>(x)) *
                4u;
            if (at + 3u >= m_blend_pixels.size())
            {
                continue;
            }
            const float strength = 0.35f;
            auto add = [strength](std::uint8_t& p_channel)
            {
                p_channel = static_cast<std::uint8_t>(
                    std::min(255, int(p_channel) + int(strength * 255.0f)));
            };
            auto sub = [strength](std::uint8_t& p_channel)
            {
                p_channel = static_cast<std::uint8_t>(
                    std::max(0, int(p_channel) - int(strength * 255.0f)));
            };

            switch (m_paint_layer)
            {
                case PaintLayer::Mud:
                    add(m_blend_pixels[at]);
                    break;
                case PaintLayer::Flowers:
                    add(m_blend_pixels[at + 1u]);
                    break;
                case PaintLayer::Path:
                    add(m_blend_pixels[at + 2u]);
                    break;
                case PaintLayer::Erase:
                    sub(m_blend_pixels[at]);
                    sub(m_blend_pixels[at + 1u]);
                    sub(m_blend_pixels[at + 2u]);
                    break;
            }

            float r = float(m_blend_pixels[at]) / 255.0f;
            float g = float(m_blend_pixels[at + 1u]) / 255.0f;
            float b = float(m_blend_pixels[at + 2u]) / 255.0f;
            const float sum = r + g + b;
            if (sum > 1.0f)
            {
                r /= sum;
                g /= sum;
                b /= sum;
                m_blend_pixels[at] = static_cast<std::uint8_t>(r * 255.0f);
                m_blend_pixels[at + 1u] = static_cast<std::uint8_t>(g * 255.0f);
                m_blend_pixels[at + 2u] = static_cast<std::uint8_t>(b * 255.0f);
            }
        }
    }
    m_blend_dirty = true;
}

void MultiTextureBlend::uploadBlendMapIfNeeded()
{
    if (!m_blend_dirty)
    {
        return;
    }
    m_blend_dirty = false;
    compages::gpu::check(m_textures[0u].write(
        std::as_bytes(std::span<const std::uint8_t>(m_blend_pixels))));
}

void MultiTextureBlend::draw(compages::world::ViewFrame const& p_frame)
{
    if (p_frame.input.mouse_over && p_frame.input.mouse_left)
    {
        const compages::core::Vector2f clip = compages::world::mouseInClipSpace(p_frame);
        const float u = std::clamp((clip.x + 0.9f) / 1.8f, 0.0f, 1.0f);
        const float v = std::clamp((clip.y + 0.9f) / 1.8f, 0.0f, 1.0f);
        paintAt(u, v);
    }
    uploadBlendMapIfNeeded();

    compages::gpu::clear({ 0.08f, 0.10f, 0.12f });
    m_plane.draw();
}

void MultiTextureBlend::controls()
{
    if (ImGui::BeginTabBar("##blend_layers"))
    {
        if (ImGui::BeginTabItem("Mud (red)"))
        {
            m_paint_layer = PaintLayer::Mud;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Flowers (green)"))
        {
            m_paint_layer = PaintLayer::Flowers;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Path (blue)"))
        {
            m_paint_layer = PaintLayer::Path;
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Grass (erase)"))
        {
            m_paint_layer = PaintLayer::Erase;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::SliderFloat("Brush size", &m_brush, 4.0f, 40.0f);
    ImGui::TextUnformatted(
        "Left drag on the picture paints the selected layer.");
}

} // namespace examples
