// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Visualization/Chart.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>

namespace compages::renderer
{

namespace
{

// ----------------------------------------------------------------------------
//! \brief The glyphs of the small font: 3 x 5 dots, a row of 3 bits each,
//! from the top.
// ----------------------------------------------------------------------------
std::array<std::uint8_t, 5> glyph(char p_char)
{
    switch (p_char)
    {
        case '0': return { 7, 5, 5, 5, 7 };
        case '1': return { 2, 6, 2, 2, 7 };
        case '2': return { 7, 1, 7, 4, 7 };
        case '3': return { 7, 1, 7, 1, 7 };
        case '4': return { 5, 5, 7, 1, 1 };
        case '5': return { 7, 4, 7, 1, 7 };
        case '6': return { 7, 4, 7, 5, 7 };
        case '7': return { 7, 1, 1, 1, 1 };
        case '8': return { 7, 5, 7, 5, 7 };
        case '9': return { 7, 5, 7, 1, 7 };
        case '-': return { 0, 0, 7, 0, 0 };
        case '.': return { 0, 0, 0, 0, 2 };
        case 's': return { 3, 4, 2, 1, 6 };
        default: return { 0, 0, 0, 0, 0 };
    }
}

// ----------------------------------------------------------------------------
//! \brief A round step near \c p_rough: 1, 2 or 5 times a power of ten.
// ----------------------------------------------------------------------------
float roundStep(float p_rough)
{
    const float power = std::pow(10.0f, std::floor(std::log10(std::max(p_rough, 1.0e-12f))));
    const float scaled = p_rough / power;
    return power * ((scaled < 1.5f) ? 1.0f : (scaled < 3.5f) ? 2.0f : (scaled < 7.5f) ? 5.0f : 10.0f);
}

// ----------------------------------------------------------------------------
//! \brief A number with as many decimals as the step needs.
// ----------------------------------------------------------------------------
std::string number(float p_value, float p_step)
{
    const int decimals = std::clamp(int(-std::floor(std::log10(p_step) + 1.0e-4f)), 0, 4);
    std::array<char, 32> text{};
    std::snprintf(text.data(), text.size(), "%.*f", decimals, double(p_value));
    std::string result(text.data());
    return (result == "-0") ? "0" : result;
}

} // namespace

// ----------------------------------------------------------------------------
Status Chart::create(Scene& p_scene, ChartDesc const& p_desc, std::string p_name)
{
    if ((p_desc.width < 16u) || (p_desc.height < 16u) || !(p_desc.seconds > 0.0f) ||
        !(p_desc.low < p_desc.high))
    {
        return failure("a Chart needs at least 16 x 16 pixels, some seconds, and low under high");
    }
    m_desc = p_desc;
    m_low = p_desc.low;
    m_high = p_desc.high;
    COMPAGES_TRY(m_canvas.create(p_desc.width, p_desc.height));
    COMPAGES_TRY(m_canvas.bind(p_scene.assets(), std::move(p_name)));
    return draw();
}

// ----------------------------------------------------------------------------
std::size_t Chart::series(CanvasTexture::Color p_color)
{
    m_curves.push_back(Curve{ .color = p_color, .points = {} });
    return m_curves.size() - 1u;
}

// ----------------------------------------------------------------------------
void Chart::add(std::size_t p_series, float p_time, float p_value)
{
    if (p_series >= m_curves.size())
    {
        return;
    }
    m_now = std::max(m_now, p_time);
    std::deque<compages::Vector2f>& points = m_curves[p_series].points;
    points.emplace_back(p_time, p_value);
    // Keep one point before the window, so the curve reaches its left edge.
    while ((points.size() > 2u) && (points[1].x < (m_now - m_desc.seconds)))
    {
        points.pop_front();
    }
}

// ----------------------------------------------------------------------------
void Chart::range(float p_low, float p_high)
{
    if (p_low < p_high)
    {
        m_desc.low = p_low;
        m_desc.high = p_high;
        m_low = p_low;
        m_high = p_high;
    }
}

// ----------------------------------------------------------------------------
//! The values shown, out to the next round numbers of the grid.
// ----------------------------------------------------------------------------
void Chart::fit()
{
    float lowest = std::numeric_limits<float>::max();
    float highest = std::numeric_limits<float>::lowest();
    for (Curve const& curve : m_curves)
    {
        for (compages::Vector2f const& point : curve.points)
        {
            if (point.x >= m_now - m_desc.seconds)
            {
                lowest = std::min(lowest, point.y);
                highest = std::max(highest, point.y);
            }
        }
    }
    if (lowest > highest)
    {
        m_low = m_desc.low;
        m_high = m_desc.high;
        return;
    }
    const float least = 0.1f * (m_desc.high - m_desc.low);
    if (highest - lowest < least)
    {
        const float middle = 0.5f * (lowest + highest);
        lowest = middle - (0.5f * least);
        highest = middle + (0.5f * least);
    }
    // Out to the round numbers around them; one more step where a curve
    // would touch the frame.
    const float step = roundStep((highest - lowest) / 4.0f);
    m_low = std::floor(lowest / step) * step;
    m_high = std::ceil(highest / step) * step;
    const float touch = 0.03f * (m_high - m_low);
    if (lowest - m_low < touch)
    {
        m_low -= step;
    }
    if (m_high - highest < touch)
    {
        m_high += step;
    }
}

// ----------------------------------------------------------------------------
compages::Vector2f Chart::pixelOf(float p_time, float p_value) const
{
    const float x = m_left + ((1.0f - ((m_now - p_time) / m_desc.seconds)) * (m_right - m_left));
    const float t = std::clamp((p_value - m_low) / (m_high - m_low), 0.0f, 1.0f);
    return { x, m_bottom + (t * (m_top - m_bottom)) };
}

// ----------------------------------------------------------------------------
void Chart::write(std::string const& p_text, float p_x, float p_y, bool p_right, CanvasTexture::Color p_color)
{
    const int advance = 4 * m_dot;
    const int width = (int(p_text.size()) * advance) - m_dot;
    int x = int(p_right ? (p_x - float(width)) : (p_x - (0.5f * float(width))));
    const int y = int(p_y);
    for (char c : p_text)
    {
        const std::array<std::uint8_t, 5> rows = glyph(c);
        for (int row = 0; row < 5; ++row)
        {
            for (int column = 0; column < 3; ++column)
            {
                if ((rows[std::size_t(row)] >> (2 - column)) & 1u)
                {
                    const int px = x + (column * m_dot);
                    const int py = y + ((4 - row) * m_dot);
                    m_canvas.fillPixels(px, py, px + m_dot - 1, py + m_dot - 1, p_color);
                }
            }
        }
        x += advance;
    }
}

// ----------------------------------------------------------------------------
//! A line every round step of the values and of the time, their numbers
//! outside the frame. Lines of several pixels: the picture is read without
//! filtering (CanvasTexture), so a thin line would break where it is made
//! smaller.
// ----------------------------------------------------------------------------
void Chart::drawAxes()
{
    const CanvasTexture::Color grid = CanvasTexture::Color::rgb(48, 53, 64);
    const CanvasTexture::Color frame = CanvasTexture::Color::rgb(96, 103, 118);
    const CanvasTexture::Color ink = CanvasTexture::Color::rgb(178, 184, 198);
    const int thick = std::max(0, int(m_desc.height / 256u));
    m_canvas.fillPixels(int(m_left), int(m_bottom), int(m_right), int(m_top), CanvasTexture::Color::rgb(27, 30, 38));

    const float step = roundStep((m_high - m_low) / 4.0f);
    for (float value = std::ceil(m_low / step) * step; value <= m_high + (0.001f * step); value += step)
    {
        const float y = pixelOf(m_now, value).y;
        m_canvas.paintPixelLine({ m_left, y }, { m_right, y }, grid, thick);
        if (m_dot > 0)
        {
            write(number(value, step), m_left - float(2 * m_dot), y - (2.5f * float(m_dot)), true, ink);
        }
    }
    const float every = roundStep(m_desc.seconds / 5.0f);
    for (float ago = 0.0f; ago <= m_desc.seconds + (0.001f * every); ago += every)
    {
        const float x = pixelOf(m_now - ago, m_low).x;
        m_canvas.paintPixelLine({ x, m_bottom }, { x, m_top }, grid, thick);
        if (m_dot > 0)
        {
            write((ago == 0.0f) ? std::string("0") : ("-" + number(ago, every) + "s"), x,
                  m_bottom - float(7 * m_dot), false, ink);
        }
    }
    m_canvas.paintPixelLine({ m_left, m_bottom }, { m_right, m_bottom }, frame, thick);
    m_canvas.paintPixelLine({ m_left, m_top }, { m_right, m_top }, frame, thick);
    m_canvas.paintPixelLine({ m_left, m_bottom }, { m_left, m_top }, frame, thick);
    m_canvas.paintPixelLine({ m_right, m_bottom }, { m_right, m_top }, frame, thick);
}

// ----------------------------------------------------------------------------
Status Chart::draw()
{
    const auto w = float(m_desc.width);
    const auto h = float(m_desc.height);
    // Numbers only where they fit: a dot of the font is a 1/100 of the height.
    m_dot = ((m_desc.width >= 128u) && (m_desc.height >= 64u)) ? std::max(1, int(m_desc.height / 100u)) : 0;
    m_left = (m_dot > 0) ? float(22 * m_dot) : 1.0f;
    m_bottom = (m_dot > 0) ? float(10 * m_dot) : 1.0f;
    m_right = w - ((m_dot > 0) ? float(4 * m_dot) : 2.0f);
    m_top = h - ((m_dot > 0) ? float(4 * m_dot) : 2.0f);
    if (m_desc.scale == ChartScale::FollowValues)
    {
        fit();
    }
    m_canvas.clear(CanvasTexture::Color::rgb(18, 20, 26));
    drawAxes();
    const int thick = std::max(1, int(m_desc.height / 128u));
    for (Curve const& curve : m_curves)
    {
        for (std::size_t k = 1u; k < curve.points.size(); ++k)
        {
            m_canvas.paintPixelLine(pixelOf(curve.points[k - 1u].x, curve.points[k - 1u].y),
                                    pixelOf(curve.points[k].x, curve.points[k].y), curve.color, thick);
        }
        if (!curve.points.empty())
        {
            // The newest value: a dot on the right edge.
            const compages::Vector2f last = pixelOf(curve.points.back().x, curve.points.back().y);
            const int r = 3 * thick;
            m_canvas.fillPixels(int(last.x) - r, int(last.y) - r, int(last.x) + r, int(last.y) + r, curve.color);
        }
    }
    return m_canvas.sync();
}

// ----------------------------------------------------------------------------
void Chart::applyLook(Scene& p_scene, compages::world::EntityId p_entity)
{
    m_canvas.applyLook(p_scene, p_entity);
}

} // namespace compages::renderer
