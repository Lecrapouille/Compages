// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Visualization/Chart.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <algorithm>
#include <cmath>

namespace compages::renderer
{

// ----------------------------------------------------------------------------
Status Chart::create(Scene& p_scene, ChartDesc const& p_desc, std::string p_name)
{
    if ((p_desc.width < 16u) || (p_desc.height < 16u) || !(p_desc.seconds > 0.0f) ||
        !(p_desc.low < p_desc.high))
    {
        return failure("a Chart needs at least 16 x 16 pixels, some seconds, and low under high");
    }
    m_desc = p_desc;
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
    }
}

// ----------------------------------------------------------------------------
Status Chart::draw()
{
    const auto w = float(m_desc.width);
    const auto h = float(m_desc.height);
    m_canvas.clear(CanvasTexture::Color::rgb(24, 26, 32));
    // A grid: a line every quarter of the height, every fifth of the time.
    // Lines of several pixels: the picture is read without filtering
    // (CanvasTexture), so a thin line would break where it is made smaller.
    const CanvasTexture::Color grid = CanvasTexture::Color::rgb(60, 64, 74);
    const int thick = std::max(1, int(m_desc.height / 128u));
    for (int k = 1; k < 4; ++k)
    {
        const float y = h * float(k) / 4.0f;
        m_canvas.paintPixelLine({ 0.0f, y }, { w - 1.0f, y }, grid, thick / 2);
    }
    for (int k = 1; k < 5; ++k)
    {
        const float x = w * float(k) / 5.0f;
        m_canvas.paintPixelLine({ x, 0.0f }, { x, h - 1.0f }, grid, thick / 2);
    }
    auto pixel = [this, w, h](compages::Vector2f p_point)
    {
        const float x = (1.0f - ((m_now - p_point.x) / m_desc.seconds)) * (w - 1.0f);
        const float t = (p_point.y - m_desc.low) / (m_desc.high - m_desc.low);
        return compages::Vector2f(x, std::clamp(t, 0.0f, 1.0f) * (h - 1.0f));
    };
    for (Curve const& curve : m_curves)
    {
        for (std::size_t k = 1u; k < curve.points.size(); ++k)
        {
            m_canvas.paintPixelLine(pixel(curve.points[k - 1u]), pixel(curve.points[k]), curve.color, 2 * thick);
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
