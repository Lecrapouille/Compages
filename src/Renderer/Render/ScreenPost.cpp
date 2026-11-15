// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "ScreenPost.hpp"

#include "Compages/GPU/RenderPass.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"

#include <algorithm>
#include <cmath>

namespace compages::renderer
{

namespace
{

// One triangle larger than the screen; v2f.uv goes from 0 to 1 across the part
// drawn into.
constexpr char const* SCREEN_VERTEX = R"(#version 450 core
struct V2F
{
    vec2 uv;
};
out V2F v2f;

void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    v2f.uv = 0.5 * corners[gl_VertexID] + 0.5;
    gl_Position = vec4(corners[gl_VertexID], 0.0, 1.0);
}
)";

// Half the size: the mean of four pixels, keeping only the light above the
// threshold (zero keeps all of it).
constexpr char const* DOWN_FRAGMENT = R"(#version 450 core
struct V2F
{
    vec2 uv;
};
in V2F v2f;
uniform sampler2D source;
uniform float threshold;
out vec4 oColor;

void main()
{
    vec2 t = 1.0 / vec2(textureSize(source, 0));
    vec3 c = 0.25 * (texture(source, v2f.uv + t * vec2(-0.5, -0.5)).rgb + texture(source, v2f.uv + t * vec2(0.5, -0.5)).rgb +
                     texture(source, v2f.uv + t * vec2(-0.5, 0.5)).rgb + texture(source, v2f.uv + t * vec2(0.5, 0.5)).rgb);
    float bright = max(c.r, max(c.g, c.b));
    oColor = vec4(c * (max(bright - threshold, 0.0) / max(bright, 1e-4)), 1.0);
}
)";

// Twice the size: a tent of 3 x 3 pixels, added to what the larger level
// holds. The halo widens at each level.
constexpr char const* UP_FRAGMENT = R"(#version 450 core
struct V2F
{
    vec2 uv;
};
in V2F v2f;
uniform sampler2D source;
out vec4 oColor;

void main()
{
    vec2 t = 1.0 / vec2(textureSize(source, 0));
    vec3 c = 4.0 * texture(source, v2f.uv).rgb;
    c += 2.0 * (texture(source, v2f.uv + vec2(t.x, 0.0)).rgb + texture(source, v2f.uv - vec2(t.x, 0.0)).rgb +
                texture(source, v2f.uv + vec2(0.0, t.y)).rgb + texture(source, v2f.uv - vec2(0.0, t.y)).rgb);
    c += texture(source, v2f.uv + t).rgb + texture(source, v2f.uv - t).rgb + texture(source, v2f.uv + vec2(t.x, -t.y)).rgb +
         texture(source, v2f.uv + vec2(-t.x, t.y)).rgb;
    oColor = vec4(c / 16.0, 1.0);
}
)";

// The last pass: the light, its bloom, the tone curve and the sRGB encoding.
constexpr char const* PRESENT_FRAGMENT = R"(#version 450 core
struct V2F
{
    vec2 uv;
};
in V2F v2f;
uniform sampler2D screenColor;
uniform sampler2D bloomMap;
uniform float bloomStrength;
#include <compages/present.glsl>
out vec4 oColor;

void main()
{
    vec3 c = texture(screenColor, v2f.uv).rgb + bloomStrength * texture(bloomMap, v2f.uv).rgb;
    oColor = vec4(toSrgb(toneCurve(max(c, 0.0))), 1.0);
}
)";

// What every filter has before its text, and the main() after it.
constexpr char const* FILTER_HEAD = R"(#version 450 core
struct V2F
{
    vec2 uv;
};
in V2F v2f;
uniform sampler2D screenColor;
uniform sampler2D screenDepth;
uniform vec2 screenSize;
out vec4 oColor;
)";

constexpr char const* FILTER_TAIL = R"(
void main()
{
    oColor = vec4(effect(v2f.uv), 1.0);
}
)";

constexpr std::size_t MAX_LEVELS = 6u;

// ----------------------------------------------------------------------------
Status load(compages::gpu::Drawable& p_drawable, std::string const& p_name, std::string const& p_fragment)
{
    compages::gpu::ResolvedSource fragment;
    COMPAGES_TRY_ASSIGN(fragment, shaders::compute(p_name.c_str(), p_fragment.c_str()).resolve());
    COMPAGES_TRY(p_drawable.loadSources(SCREEN_VERTEX, fragment.text));
    p_drawable.state().depth_test = false;
    p_drawable.state().depth_write = false;
    p_drawable.state().cull = compages::gpu::CullMode::None;
    return success();
}

// ----------------------------------------------------------------------------
//! The inverse of toneCurve() in present.glsl, for one channel from 0 to 1.
// ----------------------------------------------------------------------------
float untone(float p_shown, ToneMapping p_tone)
{
    // The sRGB encoding of the screen, undone first.
    const float c = std::clamp(p_shown, 0.0f, 1.0f);
    const float linear = (c <= 0.04045f) ? (c / 12.92f) : std::pow((c + 0.055f) / 1.055f, 2.4f);
    switch (p_tone)
    {
        case ToneMapping::Reinhard:
        {
            const float y = std::min(linear, 0.999f);
            return y / (1.0f - y);
        }
        case ToneMapping::Aces:
        {
            const float a = (2.43f * linear) - 2.51f;
            const float b = (0.59f * linear) - 0.03f;
            const float d = std::sqrt(std::max((b * b) - (4.0f * a * 0.14f * linear), 0.0f));
            return (-b - d) / (2.0f * a);
        }
        case ToneMapping::None:
        default:
            return linear;
    }
}

} // namespace

// ----------------------------------------------------------------------------
compages::Vector4f ScreenPost::linearClear(compages::Vector4f p_shown, ToneMapping p_tone)
{
    return { untone(p_shown.x, p_tone), untone(p_shown.y, p_tone), untone(p_shown.z, p_tone), p_shown.w };
}

// ----------------------------------------------------------------------------
Status ScreenPost::make(Picture& p_picture, std::uint32_t p_width, std::uint32_t p_height)
{
    if ((p_picture.width == p_width) && (p_picture.height == p_height) && p_picture.color.valid())
    {
        return success();
    }
    p_picture = Picture{};
    COMPAGES_TRY_ASSIGN(p_picture.color, compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::RGBA16F,
                                                                         .width = p_width,
                                                                         .height = p_height }));
    COMPAGES_TRY_ASSIGN(p_picture.framebuffer, compages::gpu::Framebuffer::create(p_picture.color));
    p_picture.width = p_width;
    p_picture.height = p_height;
    return success();
}

// ----------------------------------------------------------------------------
Result<compages::gpu::Framebuffer const*> ScreenPost::target(std::uint32_t p_width, std::uint32_t p_height)
{
    if (!m_framebuffer.valid() || (p_width != m_width) || (p_height != m_height))
    {
        m_framebuffer = {};
        COMPAGES_TRY_ASSIGN(m_color, compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::RGBA16F,
                                                                      .width = p_width,
                                                                      .height = p_height }));
        COMPAGES_TRY_ASSIGN(m_depth, compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::Depth32F,
                                                                      .width = p_width,
                                                                      .height = p_height,
                                                                      .magnify = compages::gpu::Filter::Nearest,
                                                                      .minify = compages::gpu::Filter::Nearest }));
        COMPAGES_TRY_ASSIGN(m_framebuffer, compages::gpu::Framebuffer::create(m_color, m_depth));
        m_width = p_width;
        m_height = p_height;
    }
    return &m_framebuffer;
}

// ----------------------------------------------------------------------------
Status ScreenPost::ensurePrograms()
{
    if (m_ready)
    {
        return success();
    }
    COMPAGES_TRY(load(m_down, "bloom-down.frag", DOWN_FRAGMENT));
    COMPAGES_TRY(load(m_up, "bloom-up.frag", UP_FRAGMENT));
    m_up.state().blend = compages::gpu::Blend::additive();
    COMPAGES_TRY(load(m_present, "present.frag", PRESENT_FRAGMENT));
    m_ready = true;
    return success();
}

// ----------------------------------------------------------------------------
Result<compages::gpu::Drawable*> ScreenPost::filter(ScreenFilter const& p_filter)
{
    auto found = m_filters.find(p_filter.glsl);
    if (found == m_filters.end())
    {
        compages::gpu::Drawable drawable;
        const Status loaded =
            load(drawable, p_filter.name + ".frag", std::string(FILTER_HEAD) + p_filter.glsl + FILTER_TAIL);
        if (!loaded)
        {
            return failure("the screen filter \"" + p_filter.name + "\": " + loaded.error());
        }
        found = m_filters.emplace(p_filter.glsl, std::move(drawable)).first;
    }
    return &found->second;
}

// ----------------------------------------------------------------------------
//! Down to a sixty-fourth of the size, then back up, each level adding its
//! light to the one above: level 0 ends with the halo of every size.
// ----------------------------------------------------------------------------
Status ScreenPost::bloom(Bloom const& p_bloom, compages::gpu::Texture const& p_source)
{
    std::uint32_t width = m_width;
    std::uint32_t height = m_height;
    std::size_t count = 0u;
    m_levels.resize(MAX_LEVELS);
    while ((count < MAX_LEVELS) && (width >= 16u) && (height >= 16u))
    {
        width /= 2u;
        height /= 2u;
        COMPAGES_TRY(make(m_levels[count], width, height));
        ++count;
    }
    m_level_count = count;
    compages::gpu::Texture const* source = &p_source;
    for (std::size_t k = 0u; k < count; ++k)
    {
        compages::gpu::RenderPass pass(m_levels[k].framebuffer, { .clear_color = false, .clear_depth = false });
        m_down["source"] = *source;
        m_down["threshold"] = (k == 0u) ? std::max(p_bloom.threshold, 0.0f) : 0.0f;
        m_down.draw(3u);
        source = &m_levels[k].color;
    }
    for (std::size_t k = count; k-- > 1u;)
    {
        compages::gpu::RenderPass pass(m_levels[k - 1u].framebuffer, { .clear_color = false, .clear_depth = false });
        m_up["source"] = m_levels[k].color;
        m_up.draw(3u);
    }
    return success();
}

// ----------------------------------------------------------------------------
Status ScreenPost::finish(ScreenEffects const& p_effects, RenderSnapshot const& p_view)
{
    COMPAGES_TRY(ensurePrograms());
    const compages::Vector2f size{ float(m_width), float(m_height) };

    // The filters, each reading what the previous one made.
    compages::gpu::Texture const* light = &m_color;
    for (std::size_t k = 0u; k < p_effects.filters.size(); ++k)
    {
        ScreenFilter const& f = p_effects.filters[k];
        compages::gpu::Drawable* drawable = nullptr;
        COMPAGES_TRY_ASSIGN(drawable, filter(f));
        Picture& out = m_turns[k % 2u];
        COMPAGES_TRY(make(out, m_width, m_height));
        compages::gpu::RenderPass pass(out.framebuffer, { .clear_color = false, .clear_depth = false });
        compages::gpu::Program& program = drawable->program();
        if (program.has("screenColor"))
        {
            (*drawable)["screenColor"] = *light;
        }
        if (program.has("screenDepth"))
        {
            (*drawable)["screenDepth"] = m_depth;
        }
        if (program.has("screenSize"))
        {
            program.set("screenSize", size);
        }
        for (auto const& [name, value] : f.values)
        {
            if (!program.has(name))
            {
                return failure("the screen filter \"" + f.name + "\" has no uniform \"" + name + "\"");
            }
            program.set(name, value);
        }
        drawable->draw(3u);
        light = &out.color;
    }

    const bool bloom_on = p_effects.bloom.enabled && (p_effects.bloom.strength > 0.0f);
    if (bloom_on)
    {
        COMPAGES_TRY(bloom(p_effects.bloom, *light));
    }

    // Into the pass open on the screen: the tone curve, the screen colours.
    m_present["screenColor"] = *light;
    const bool halo = bloom_on && (m_level_count > 0u);
    m_present["bloomMap"] = halo ? m_levels[0].color : *light;
    m_present["bloomStrength"] = halo ? p_effects.bloom.strength : 0.0f;
    compages::gpu::Program& program = m_present.program();
    program.set("toneMapping", static_cast<int>(p_view.environment.tone_mapping));
    m_present.draw(3u);
    return success();
}

} // namespace compages::renderer
