// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/06a_Mandelbrot.hpp"

#include <cmath>

namespace examples
{

constexpr const char* VERTEX = R"(#version 450 core
out vec2 vClip;

void main()
{
    vec2 corners[3] = vec2[3](vec2(-1.0, -1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));
    vClip = corners[gl_VertexID];
    gl_Position = vec4(vClip, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec2 vClip;

uniform vec2 center;
uniform float scale;
uniform float aspect;

out vec4 oColor;

void main()
{
    // Clip position, stretched by the window, is the complex number c.
    vec2 c = center + vec2(vClip.x * aspect, vClip.y) * scale;
    vec2 z = vec2(0.0);
    int n = 0;
    const int limit = 80;

    // z = z² + c, until it escapes or the iteration budget runs out.
    for (; n < limit; ++n)
    {
        if (dot(z, z) > 4.0)
        {
            break;
        }
        z = vec2(z.x * z.x - z.y * z.y, 2.0 * z.x * z.y) + c;
    }

    // Inside the set stays dark. The escape count is a colour.
    if (n == limit)
    {
        oColor = vec4(0.02, 0.02, 0.05, 1.0);
        return;
    }

    float t = float(n) / float(limit);
    oColor = vec4(0.5 + 0.5 * cos(3.0 + t * 6.2832 + vec3(0.0, 0.6, 1.0)), 1.0);
}
)";

std::string Mandelbrot::description() const
{
    return "No vertex buffer. The shader builds a triangle covering the window "
           "from gl_VertexID, and every pixel is an iteration of z = z² + c. "
           "The mouse is the point the zoom keeps still; time closes in on it. "
           "The counters below should show one program, one pipeline, and "
           "nothing else: drawable.draw(3u) with no vertex given.";
}

compages::Status Mandelbrot::setUp()
{
    return m_screen.load(VERTEX, FRAGMENT);
}

void Mandelbrot::draw(compages::world::ViewFrame const& p_frame)
{
    // The complex number under the mouse is computed at the scale of the last
    // frame, then the view is placed so that the same number is still under the
    // mouse at the new scale. Moving the cursor without zooming does nothing;
    // time is what closes in, and the cursor is what chooses where.
    const float scale = 1.5f * std::exp(-0.18f * p_frame.total);
    const compages::core::Vector2f mouse =
        p_frame.input.mouse_over ? compages::world::mouseInClipSpace(p_frame)
                                 : compages::core::Vector2f(0.0f, 0.0f);
    const compages::core::Vector2f under(
        m_center.x + (mouse.x * aspect(p_frame) * m_scale),
        m_center.y + (mouse.y * m_scale));
    m_center.x = under.x - (mouse.x * aspect(p_frame) * scale);
    m_center.y = under.y - (mouse.y * scale);
    m_scale = scale;

    m_screen["center"] = m_center;
    m_screen["scale"] = m_scale;
    m_screen["aspect"] = aspect(p_frame);

    // Every pixel is painted, so there is nothing to clear.
    m_screen.draw(3u);
}

} // namespace examples
