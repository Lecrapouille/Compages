// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/02a_DynamicGeometry.hpp"

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

//! \brief Index of the triangle apex: the top vertex (opposite the flat base).
//! Which vertex the mouse moves. The other two are sent once, at the first
//! draw, and never again.
constexpr std::size_t APEX = 2u;

// ----------------------------------------------------------------------------
std::string DynamicTriangle::description() const
{
    return "The top corner follows the mouse. The drawable keeps the corners, so "
           "the drawable remembers which ones were edited: one corner out of "
           "three travels to the graphics card each frame, not all three. "
           "Nothing here asks for a send: editing the corner is enough.";
}

// ----------------------------------------------------------------------------
compages::Status DynamicTriangle::setUp()
{
    // The same shader as 01b. What changes here is which vertices travel
    // again after the first draw.
    COMPAGES_TRY(m_triangle.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));

    // Three corners, kept by the drawable. The top one will follow the mouse.
    m_triangle.setVertices<Vertex>({ { { -0.8f, -0.7f }, { 1.0f, 0.2f, 0.2f } },
                                     { { 0.8f, -0.7f }, { 0.2f, 1.0f, 0.2f } },
                                     { { 0.0f, 0.7f }, { 0.3f, 0.4f, 1.0f } } });
    return m_triangle.prepare();
}

// ----------------------------------------------------------------------------
void DynamicTriangle::draw(compages::world::ViewFrame const& p_frame)
{
    // editVertex() hands back the corner to change, and remembers it was
    // changed: the next draw sends this one corner and no other. While the
    // mouse is away, the corner stays where it was.
    if (p_frame.input.mouse_over)
    {
        m_triangle.editVertex<Vertex>(APEX).position =
            compages::world::mouseInClipSpace(p_frame);
    }

    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_triangle.draw();
}

} // namespace examples
