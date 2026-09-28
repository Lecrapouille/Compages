// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/02_DynamicGeometry.hpp"

namespace examples
{

constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec2 position;
in vec3 color;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

//! \brief Which vertex the mouse moves. The other two are sent once, at the
//! first draw, and never again.
constexpr std::size_t APEX = 2u;

std::string DynamicTriangle::description() const
{
    return "The apex follows the mouse. The drawable remembers which vertices "
           "were written, so one vertex out of three travels to the device each "
           "frame rather than all three. Nothing here asks for an upload: "
           "writing the vertex is the whole of it.";
}

compages::gpu::Status DynamicTriangle::setUp()
{
    // The same shader as 01b. What changes here is which vertices travel
    // again after the first draw.
    COMPAGES_TRY(m_triangle.load(VERTEX_SHADER, FRAGMENT_SHADER));

    // Three vertices, once. The apex is the one draw() rewrites.
    m_triangle.vertices<Vertex>({ { { -0.8f, -0.7f }, { 1.0f, 0.2f, 0.2f } },
                                  { {  0.8f, -0.7f }, { 0.2f, 1.0f, 0.2f } },
                                  { {  0.0f,  0.7f }, { 0.3f, 0.4f, 1.0f } } });
    return m_triangle.prepare();
}

void DynamicTriangle::draw(Frame const& p_frame)
{
    // A reference into the drawable: taking it is what marks the vertex as
    // changed, so this one vertex is what the next draw sends. It stays where
    // it was while the mouse is away.
    if (p_frame.input.mouse_over)
    {
        m_triangle.vertex<Vertex>(APEX).position = mouseInClipSpace(p_frame);
    }

    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_triangle.draw();
}

} // namespace examples
