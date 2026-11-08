// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "TextDraw.hpp"

#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/VertexLayout.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include <stb_truetype.h>
#pragma GCC diagnostic pop

namespace compages::renderer
{

namespace
{

// The corners are in pixels of the picture, from its bottom left corner.
constexpr char const* LABEL_VERTEX = R"(#version 450 core
in vec2 position;
in vec2 uv;
in vec3 color;
in float halo;
in float opacity;

uniform vec2 viewport;

out vec2 vUv;
out vec3 vColor;
out float vHalo;
out float vOpacity;

void main()
{
    vUv = uv;
    vColor = color;
    vHalo = halo;
    vOpacity = opacity;
    gl_Position = vec4((position / viewport) * 2.0 - 1.0, 0.0, 1.0);
}
)";

// The edge of a letter is where the field is 0.5; fwidth() says how much the
// field changes over one pixel of the picture, so the edge is antialiased
// over one pixel at any size. The halo is the same test at a lower value,
// drawn dark under the letter.
constexpr char const* LABEL_FRAGMENT = R"(#version 450 core
in vec2 vUv;
in vec3 vColor;
in float vHalo;
in float vOpacity;

uniform sampler2D atlas;

out vec4 oColor;

void main()
{
    float field = texture(atlas, vUv).r;
    float soft = max(fwidth(field) * 0.75, 1e-4);
    float letter = smoothstep(0.5 - soft, 0.5 + soft, field);
    float halo = 0.75 * smoothstep(vHalo - soft, vHalo + soft, field);
    // The letter over its black halo.
    float alpha = letter + (halo * (1.0 - letter));
    if ((alpha * vOpacity) < 0.004)
    {
        discard;
    }
    oColor = vec4(vColor * (letter / alpha), alpha * vOpacity);
}
)";

//! \brief A letter baked by stb_truetype, before it is packed.
struct Baked
{
    char32_t code = 0u;
    int width = 0;
    int height = 0;
    std::vector<unsigned char> pixels;
};

// ----------------------------------------------------------------------------
//! \brief A disc of radius \c p_radius baked pixels, as a distance field.
//! \param[in] p_radius The radius of the disc.
//! \return The field, PADDING pixels larger on each side.
// ----------------------------------------------------------------------------
Baked bakeDot(float p_radius)
{
    Baked dot;
    dot.code = FontAtlas::DOT;
    dot.width = int(std::ceil(2.0f * (p_radius + float(FontAtlas::PADDING))));
    dot.height = dot.width;
    dot.pixels.resize(std::size_t(dot.width * dot.height));
    const float centre = 0.5f * float(dot.width);
    for (int y = 0; y < dot.height; ++y)
    {
        for (int x = 0; x < dot.width; ++x)
        {
            const float dx = (float(x) + 0.5f) - centre;
            const float dy = (float(y) + 0.5f) - centre;
            const float inside = p_radius - std::sqrt((dx * dx) + (dy * dy));
            const float value = 128.0f + (inside * FontAtlas::STEPS_PER_PIXEL);
            dot.pixels[std::size_t((y * dot.width) + x)] =
                static_cast<unsigned char>(std::clamp(value, 0.0f, 255.0f));
        }
    }
    return dot;
}

} // namespace

// ----------------------------------------------------------------------------
FontAtlas::Glyph const& FontAtlas::glyph(char32_t p_code) const
{
    if ((p_code >= FIRST) && (p_code <= LAST))
    {
        Glyph const& found = glyphs[p_code - FIRST];
        if ((found.advance > 0.0f) || (p_code == DOT))
        {
            return found;
        }
    }
    return glyphs[U'?' - FIRST];
}

// ----------------------------------------------------------------------------
Result<FontAtlas> bakeFont(std::filesystem::path const& p_file)
{
    std::ifstream stream(p_file, std::ios::binary);
    if (!stream)
    {
        return failure("Cannot open the font " + p_file.string());
    }
    const std::vector<unsigned char> file((std::istreambuf_iterator<char>(stream)),
                                          std::istreambuf_iterator<char>());
    stbtt_fontinfo font;
    const int offset = stbtt_GetFontOffsetForIndex(file.data(), 0);
    if ((offset < 0) || (stbtt_InitFont(&font, file.data(), offset) == 0))
    {
        return failure(p_file.string() + " is not a TrueType or OpenType font");
    }

    FontAtlas atlas;
    const float scale = stbtt_ScaleForPixelHeight(&font, FontAtlas::BAKE_PIXELS);
    int ascent = 0;
    int descent = 0;
    int gap = 0;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &gap);
    atlas.ascent = float(ascent) * scale;
    atlas.descent = float(descent) * scale;

    // Each letter as a distance field: 128 on its edge, PADDING pixels of
    // field around it.
    std::vector<Baked> baked;
    for (char32_t code = FontAtlas::FIRST; code <= FontAtlas::LAST; ++code)
    {
        // The control codes of Latin-1 are not letters; DOT gets a disc.
        const bool control = (code >= FontAtlas::DOT) && (code < 160u);
        if (control || (stbtt_FindGlyphIndex(&font, int(code)) == 0))
        {
            continue;
        }
        FontAtlas::Glyph& glyph = atlas.glyphs[code - FontAtlas::FIRST];
        int advance = 0;
        int bearing = 0;
        stbtt_GetCodepointHMetrics(&font, int(code), &advance, &bearing);
        glyph.advance = float(advance) * scale;

        Baked letter;
        letter.code = code;
        int left = 0;
        int top = 0;
        unsigned char* field = stbtt_GetCodepointSDF(&font,
                                                     scale,
                                                     int(code),
                                                     FontAtlas::PADDING,
                                                     128,
                                                     FontAtlas::STEPS_PER_PIXEL,
                                                     &letter.width,
                                                     &letter.height,
                                                     &left,
                                                     &top);
        if (field == nullptr)
        {
            continue;   // a space: it advances, draws nothing
        }
        letter.pixels.assign(field, field + (letter.width * letter.height));
        stbtt_FreeSDF(field, nullptr);
        glyph.left = float(left);
        glyph.top = float(top);
        baked.emplace_back(std::move(letter));
    }
    if (atlas.glyphs[U'?' - FontAtlas::FIRST].advance <= 0.0f)
    {
        return failure(p_file.string() + " has no '?', written for missing letters");
    }
    Baked dot = bakeDot(0.16f * FontAtlas::BAKE_PIXELS);
    atlas.glyphs[FontAtlas::DOT - FontAtlas::FIRST].left = -0.5f * float(dot.width);
    atlas.glyphs[FontAtlas::DOT - FontAtlas::FIRST].top = -0.5f * float(dot.height);
    baked.emplace_back(std::move(dot));

    // Packed in rows ("shelves"), the tallest first so that a row wastes
    // little height. One pixel apart, so that the linear filter does not
    // read the next letter.
    std::sort(baked.begin(),
              baked.end(),
              [](Baked const& p_a, Baked const& p_b) { return p_a.height > p_b.height; });
    atlas.width = 1024u;
    std::uint32_t x = 0u;
    std::uint32_t y = 0u;
    std::uint32_t row = 0u;
    for (Baked const& letter : baked)
    {
        if ((x + std::uint32_t(letter.width)) > atlas.width)
        {
            x = 0u;
            y += row + 1u;
            row = 0u;
        }
        FontAtlas::Glyph& glyph = atlas.glyphs[letter.code - FontAtlas::FIRST];
        glyph.x = x;
        glyph.y = y;
        glyph.width = std::uint32_t(letter.width);
        glyph.height = std::uint32_t(letter.height);
        x += glyph.width + 1u;
        row = std::max(row, glyph.height);
    }
    // A multiple of four rows, as textures like.
    atlas.height = ((y + row) + 3u) & ~3u;
    atlas.pixels.assign(std::size_t(atlas.width) * atlas.height, std::byte{ 0 });
    for (Baked const& letter : baked)
    {
        FontAtlas::Glyph const& glyph = atlas.glyphs[letter.code - FontAtlas::FIRST];
        for (std::uint32_t row_in = 0u; row_in < glyph.height; ++row_in)
        {
            for (std::uint32_t column = 0u; column < glyph.width; ++column)
            {
                atlas.pixels[((glyph.y + row_in) * atlas.width) + glyph.x + column] =
                    std::byte(letter.pixels[(row_in * glyph.width) + column]);
            }
        }
    }
    return atlas;
}

// ----------------------------------------------------------------------------
std::vector<char32_t> decodeUtf8(std::string_view p_text)
{
    constexpr char32_t REPLACEMENT = 0xFFFDu;
    std::vector<char32_t> codes;
    std::size_t i = 0u;
    while (i < p_text.size())
    {
        const auto lead = static_cast<unsigned char>(p_text[i]);
        // How many bytes follow the first one, and its own bits.
        std::size_t more = 0u;
        char32_t code = lead;
        if (lead >= 0xF0u)
        {
            more = 3u;
            code = lead & 0x07u;
        }
        else if (lead >= 0xE0u)
        {
            more = 2u;
            code = lead & 0x0Fu;
        }
        else if (lead >= 0xC0u)
        {
            more = 1u;
            code = lead & 0x1Fu;
        }
        else if (lead >= 0x80u)
        {
            codes.push_back(REPLACEMENT);   // a continuation byte alone
            ++i;
            continue;
        }
        ++i;
        bool broken = false;
        for (std::size_t k = 0u; k < more; ++k, ++i)
        {
            if ((i >= p_text.size()) ||
                ((static_cast<unsigned char>(p_text[i]) & 0xC0u) != 0x80u))
            {
                broken = true;
                break;
            }
            code = (code << 6u) | (static_cast<unsigned char>(p_text[i]) & 0x3Fu);
        }
        codes.push_back(broken ? REPLACEMENT : code);
    }
    return codes;
}

// ----------------------------------------------------------------------------
Status TextDraw::loadFont(std::filesystem::path const& p_file)
{
    COMPAGES_TRY_ASSIGN(m_atlas, bakeFont(p_file));
    COMPAGES_TRY_ASSIGN(m_texture,
                        compages::gpu::Texture::create(compages::gpu::TextureDesc::image(
                            m_atlas.width, m_atlas.height, compages::gpu::PixelFormat::R8)));
    COMPAGES_TRY(m_texture.write(std::span<const std::byte>(m_atlas.pixels)));

    COMPAGES_TRY(m_program.loadSources(LABEL_VERTEX, LABEL_FRAGMENT));
    // Over the picture: no depth, blended.
    compages::gpu::RenderState state;
    state.depth_test = false;
    state.depth_write = false;
    state.cull = compages::gpu::CullMode::None;
    state.blend = compages::gpu::Blend::alpha();
    COMPAGES_TRY_ASSIGN(m_pipeline,
                        compages::gpu::Pipeline::create<LabelVertex>(
                            m_program, compages::gpu::VertexLayout::of<LabelVertex>(), state));
    COMPAGES_TRY_ASSIGN(m_vertices,
                        compages::gpu::VertexBuffer<LabelVertex>::create(
                            1024u, compages::gpu::BufferUsage::Stream));
    return success();
}

// ----------------------------------------------------------------------------
void TextDraw::quad(FontAtlas::Glyph const& p_glyph,
                     float p_left,
                     float p_top,
                     float p_scale,
                     compages::Vector3f const& p_color,
                     float p_halo)
{
    const float right = p_left + (float(p_glyph.width) * p_scale);
    const float bottom = p_top - (float(p_glyph.height) * p_scale);
    const float u0 = float(p_glyph.x) / float(m_atlas.width);
    const float u1 = float(p_glyph.x + p_glyph.width) / float(m_atlas.width);
    // Row 0 of a letter is its top, and row 0 of the atlas is v = 0.
    const float v0 = float(p_glyph.y) / float(m_atlas.height);
    const float v1 = float(p_glyph.y + p_glyph.height) / float(m_atlas.height);
    const LabelVertex top_left{ { p_left, p_top }, { u0, v0 }, p_color, p_halo, 1.0f };
    const LabelVertex top_right{ { right, p_top }, { u1, v0 }, p_color, p_halo, 1.0f };
    const LabelVertex bottom_left{ { p_left, bottom }, { u0, v1 }, p_color, p_halo, 1.0f };
    const LabelVertex bottom_right{ { right, bottom }, { u1, v1 }, p_color, p_halo, 1.0f };
    for (LabelVertex const& corner :
         { bottom_left, bottom_right, top_right, top_right, top_left, bottom_left })
    {
        m_pending.emplace_back(corner);
    }
}

// ----------------------------------------------------------------------------
void TextDraw::solid(std::array<compages::Vector2f, 4u> const& p_corners,
                     compages::Vector3f const& p_color,
                     float p_opacity)
{
    FontAtlas::Glyph const& dot = m_atlas.glyph(FontAtlas::DOT);
    const compages::Vector2f inside((float(dot.x) + (0.5f * float(dot.width))) / float(m_atlas.width),
                                    (float(dot.y) + (0.5f * float(dot.height))) / float(m_atlas.height));
    for (const std::size_t i : { 0u, 1u, 2u, 2u, 3u, 0u })
    {
        m_pending.push_back({ p_corners[i], inside, p_color, 0.45f, p_opacity });
    }
}

// ----------------------------------------------------------------------------
void TextDraw::frame(Room const& p_room,
                     float p_radius,
                     compages::Vector3f const& p_color,
                     float p_opacity)
{
    // The disc of the dot has a radius of 0.16 baked letters; the glyph is
    // PADDING pixels of field wider on each side. Scaled so that its radius
    // is p_radius, its half reaches past the edge of the disc by the field
    // that draws the rim.
    FontAtlas::Glyph const& dot = m_atlas.glyph(FontAtlas::DOT);
    const float scale = p_radius / (0.16f * FontAtlas::BAKE_PIXELS);
    const float half = 0.5f * float(dot.width) * scale;
    const float halo = haloFor(scale);
    // Four columns and four rows: the outer ones end where the field does,
    // the inner ones sit on the centres of the corners.
    const std::array<float, 4u> xs = { p_room.left + p_radius - half, p_room.left + p_radius,
                                       p_room.right - p_radius, p_room.right - p_radius + half };
    const std::array<float, 4u> ys = { p_room.bottom + p_radius - half, p_room.bottom + p_radius,
                                       p_room.top - p_radius, p_room.top - p_radius + half };
    const float u0 = float(dot.x) / float(m_atlas.width);
    const float uc = (float(dot.x) + (0.5f * float(dot.width))) / float(m_atlas.width);
    const float u1 = float(dot.x + dot.width) / float(m_atlas.width);
    const float v0 = float(dot.y) / float(m_atlas.height);
    const float vc = (float(dot.y) + (0.5f * float(dot.height))) / float(m_atlas.height);
    const float v1 = float(dot.y + dot.height) / float(m_atlas.height);
    // The field is a disc: the same from the bottom or the top.
    const std::array<float, 4u> us = { u0, uc, uc, u1 };
    const std::array<float, 4u> vs = { v1, vc, vc, v0 };
    for (std::size_t row = 0u; row < 3u; ++row)
    {
        for (std::size_t column = 0u; column < 3u; ++column)
        {
            const LabelVertex a{ { xs[column], ys[row] }, { us[column], vs[row] }, p_color, halo, p_opacity };
            const LabelVertex b{ { xs[column + 1u], ys[row] }, { us[column + 1u], vs[row] }, p_color, halo, p_opacity };
            const LabelVertex c{ { xs[column + 1u], ys[row + 1u] }, { us[column + 1u], vs[row + 1u] }, p_color, halo, p_opacity };
            const LabelVertex d{ { xs[column], ys[row + 1u] }, { us[column], vs[row + 1u] }, p_color, halo, p_opacity };
            for (LabelVertex const& corner : { a, b, c, c, d, a })
            {
                m_pending.emplace_back(corner);
            }
        }
    }
}

// ----------------------------------------------------------------------------
float TextDraw::haloFor(float p_scale)
{
    // The atlas is baked at BAKE_PIXELS; a halo of 1.5 pixels of the picture
    // is that many baked pixels of field below the edge.
    return std::clamp(0.5f - ((1.5f / p_scale) * FontAtlas::STEPS_PER_PIXEL / 255.0f),
                      0.05f,
                      0.45f);
}

// ----------------------------------------------------------------------------
float TextDraw::width(std::vector<char32_t> const& p_codes, float p_scale) const
{
    float total = 0.0f;
    for (const char32_t code : p_codes)
    {
        total += m_atlas.glyph(code).advance * p_scale;
    }
    return total;
}

// ----------------------------------------------------------------------------
bool TextDraw::reserve(Room const& p_room)
{
    for (Room const& taken : m_taken)
    {
        if ((p_room.left < taken.right) && (taken.left < p_room.right) &&
            (p_room.bottom < taken.top) && (taken.bottom < p_room.top))
        {
            return false;
        }
    }
    m_taken.emplace_back(p_room);
    return true;
}

// ----------------------------------------------------------------------------
void TextDraw::line(std::vector<char32_t> const& p_codes,
                    float p_pen,
                    float p_baseline,
                    float p_scale,
                    compages::Vector3f const& p_color)
{
    const float halo = haloFor(p_scale);
    for (const char32_t code : p_codes)
    {
        FontAtlas::Glyph const& glyph = m_atlas.glyph(code);
        if (glyph.width > 0u)
        {
            quad(glyph,
                 p_pen + (glyph.left * p_scale),
                 p_baseline - (glyph.top * p_scale),
                 p_scale,
                 p_color,
                 halo);
        }
        p_pen += glyph.advance * p_scale;
    }
}

// ----------------------------------------------------------------------------
void TextDraw::add(Label const& p_label,
                    compages::Vector3f const& p_point,
                    CameraFrame const& p_camera)
{
    if (m_atlas.width == 0u)
    {
        m_missing_font = true;
        return;
    }
    if ((p_label.hide_closer_than > 0.0f) &&
        (compages::vector::norm(p_point - p_camera.position) < p_label.hide_closer_than))
    {
        return;
    }
    // Where the point falls on the picture, in pixels from its bottom left.
    const compages::Vector4f clip =
        p_camera.view_projection * compages::Vector4f(p_point, 1.0f);
    if (clip.w <= 0.0f)
    {
        return;   // behind the camera
    }
    const float x = clip.x / clip.w;
    const float y = clip.y / clip.w;
    if ((std::abs(x) > 1.0f) || (std::abs(y) > 1.0f))
    {
        return;
    }
    const float px = (0.5f * (x + 1.0f)) * float(p_camera.viewport_width);
    const float py = (0.5f * (y + 1.0f)) * float(p_camera.viewport_height);
    const float scale = p_label.pixels / FontAtlas::BAKE_PIXELS;

    // The room it takes: from the dot to the end of the text. A label over
    // text already written this picture is not written: the names stay
    // readable where things crowd. The letters fill about 0.8 of the line:
    // two lines one above the other can be closer than a line apart.
    const std::vector<char32_t> codes = decodeUtf8(p_label.text);
    FontAtlas::Glyph const& dot = m_atlas.glyph(FontAtlas::DOT);
    const float start = px + (0.4f * p_label.pixels);
    if (!reserve({ px + (dot.left * scale), py - (0.4f * p_label.pixels),
                   start + width(codes, scale), py + (0.4f * p_label.pixels) }))
    {
        return;
    }

    // The dot on the point, then the text on its right, centred on it.
    quad(dot, px + (dot.left * scale), py - (dot.top * scale), scale, p_label.color,
         haloFor(scale));
    line(codes, start, py - (0.5f * (m_atlas.ascent + m_atlas.descent) * scale), scale,
         p_label.color);
}

// ----------------------------------------------------------------------------
void TextDraw::add(Tooltip const& p_tooltip,
                   compages::Vector3f const& p_point,
                   CameraFrame const& p_camera)
{
    if (m_atlas.width == 0u)
    {
        m_missing_font = true;
        return;
    }
    const compages::Vector4f clip =
        p_camera.view_projection * compages::Vector4f(p_point, 1.0f);
    if (clip.w <= 0.0f)
    {
        return;   // behind the camera
    }
    const float x = clip.x / clip.w;
    const float y = clip.y / clip.w;
    if ((std::abs(x) > 1.0f) || (std::abs(y) > 1.0f))
    {
        return;
    }
    const float view_width = float(p_camera.viewport_width);
    const float view_height = float(p_camera.viewport_height);
    const float px = (0.5f * (x + 1.0f)) * view_width;
    const float py = (0.5f * (y + 1.0f)) * view_height;

    // The lines and their sizes: the title a quarter taller.
    const float pixels = p_tooltip.pixels;
    const float title_pixels = 1.25f * pixels;
    const float scale = pixels / FontAtlas::BAKE_PIXELS;
    const float title_scale = title_pixels / FontAtlas::BAKE_PIXELS;
    const std::vector<char32_t> title = decodeUtf8(p_tooltip.title);
    std::vector<std::vector<char32_t>> lines;
    for (std::size_t begin = 0u; begin < p_tooltip.text.size();)
    {
        std::size_t end = p_tooltip.text.find('\n', begin);
        if (end == std::string::npos)
        {
            end = p_tooltip.text.size();
        }
        lines.emplace_back(decodeUtf8(std::string_view(p_tooltip.text).substr(begin, end - begin)));
        begin = end + 1u;
    }
    const float pad = 0.6f * pixels;
    const float spacing = 1.25f * pixels;
    const float gap = lines.empty() ? 0.0f : (0.6f * pixels);
    float widest = width(title, title_scale);
    for (std::vector<char32_t> const& line_codes : lines)
    {
        widest = std::max(widest, width(line_codes, scale));
    }
    const float box_width = widest + (2.0f * pad);
    const float box_height = (2.0f * pad) + (1.15f * title_pixels) + gap +
                             (float(lines.size()) * spacing);

    // Above the point, or below it when the top of the picture is too
    // close; slid sideways to stay inside the picture.
    const float stem = 1.8f * pixels;
    const bool above = (py + stem + box_height) <= view_height;
    Room box;
    box.bottom = above ? (py + stem) : (py - stem - box_height);
    box.top = box.bottom + box_height;
    box.left = std::clamp(px - (1.5f * pixels), 2.0f, std::max(2.0f, view_width - box_width - 2.0f));
    box.right = box.left + box_width;
    const Room room{ std::min(box.left, px - pixels), std::min(box.bottom, py - pixels),
                     std::max(box.right, px + pixels), std::max(box.top, py + pixels) };
    if (!reserve(room))
    {
        return;
    }

    // The stem from the point to the edge of the frame, the dot on the
    // point, then the frame over the stem.
    const float radius = std::min(0.5f * pixels, 0.25f * box_height);
    const float foot = std::clamp(px, box.left + radius + 2.0f, box.right - radius - 2.0f);
    const float edge = above ? box.bottom : box.top;
    const compages::Vector2f along(foot - px, edge - py);
    const float length = std::max(compages::vector::norm(along), 1.0e-3f);
    const compages::Vector2f across = compages::Vector2f(-along.y, along.x) * (1.0f / length);
    const compages::Vector2f point(px, py);
    const compages::Vector2f end(foot, edge);
    solid({ point - across, point + across, end + across, end - across },
          p_tooltip.accent, 0.9f);
    const float dot_scale = (0.45f * pixels) / FontAtlas::BAKE_PIXELS;
    FontAtlas::Glyph const& dot = m_atlas.glyph(FontAtlas::DOT);
    quad(dot, px + (dot.left * dot_scale), py - (dot.top * dot_scale), dot_scale,
         p_tooltip.accent, haloFor(dot_scale));
    frame(box, radius, p_tooltip.background, p_tooltip.opacity);

    // The title, a thin line under it, the lines.
    float baseline = box.top - pad - (m_atlas.ascent * title_scale);
    line(title, box.left + pad, baseline, title_scale, p_tooltip.accent);
    baseline += m_atlas.descent * title_scale;
    if (!lines.empty())
    {
        const float rule = baseline - (0.3f * pixels);
        solid({ compages::Vector2f(box.left + pad, rule - 0.5f),
                compages::Vector2f(box.right - pad, rule - 0.5f),
                compages::Vector2f(box.right - pad, rule + 0.5f),
                compages::Vector2f(box.left + pad, rule + 0.5f) },
              p_tooltip.accent, 0.5f);
        baseline -= gap + (m_atlas.ascent * scale);
        for (std::vector<char32_t> const& line_codes : lines)
        {
            line(line_codes, box.left + pad, baseline, scale, p_tooltip.color);
            baseline -= spacing;
        }
    }
}

// ----------------------------------------------------------------------------
void TextDraw::add(Caption const& p_caption, CameraFrame const& p_camera)
{
    if (m_atlas.width == 0u)
    {
        m_missing_font = true;
        return;
    }
    const float scale = p_caption.pixels / FontAtlas::BAKE_PIXELS;
    const float spacing = 1.15f * p_caption.pixels;
    // Pixels from the top left corner, y going down; the picture counts from
    // its bottom left, y going up.
    const float top = float(p_camera.viewport_height) - p_caption.at.y;
    float baseline = top - (m_atlas.ascent * scale);
    std::size_t begin = 0u;
    while (begin <= p_caption.text.size())
    {
        std::size_t end = p_caption.text.find('\n', begin);
        if (end == std::string::npos)
        {
            end = p_caption.text.size();
        }
        const std::vector<char32_t> codes =
            decodeUtf8(std::string_view(p_caption.text).substr(begin, end - begin));
        // Its room, so that no label is written over it.
        m_taken.push_back({ p_caption.at.x, baseline + (m_atlas.descent * scale),
                            p_caption.at.x + width(codes, scale),
                            baseline + (m_atlas.ascent * scale) });
        line(codes, p_caption.at.x, baseline, scale, p_caption.color);
        baseline -= spacing;
        begin = end + 1u;
    }
}

// ----------------------------------------------------------------------------
Status TextDraw::flush(CameraFrame const& p_camera)
{
    if (m_missing_font)
    {
        m_missing_font = false;
        m_pending.clear();
        m_taken.clear();
        return failure("An entity has a Label or a Caption, but the Scene has "
                       "no font: call Scene::font(file)");
    }
    m_taken.clear();
    if (m_pending.empty())
    {
        return success();
    }
    if (m_pending.size() > m_vertices.count())
    {
        COMPAGES_TRY_ASSIGN(m_vertices,
                            compages::gpu::VertexBuffer<LabelVertex>::create(
                                m_pending.size(), compages::gpu::BufferUsage::Stream));
    }
    m_vertices.write(std::span<LabelVertex const>(m_pending));
    m_texture.bind(0u);
    m_program.set("atlas", 0);
    m_program.set("viewport",
                  compages::Vector2f(float(p_camera.viewport_width),
                                     float(p_camera.viewport_height)));
    compages::gpu::draw(m_pipeline, m_vertices.handle(), m_pending.size());
    m_pending.clear();
    return success();
}

} // namespace compages::renderer
