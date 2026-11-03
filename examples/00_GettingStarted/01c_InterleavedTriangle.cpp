// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/01c_InterleavedTriangle.hpp"

namespace examples
{

// ----------------------------------------------------------------------------
//! \brief The same shader as 01b: `position` and `color` are the names the
//! fields of Vertex must have.
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

// ----------------------------------------------------------------------------
constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

// ----------------------------------------------------------------------------
std::string InterleavedTriangle::description() const
{
    return "A triangle from a struct Vertex { position; color; }. Each field "
           "feeds the shader input of the same name. The drawable keeps a "
           "copy of the corners and sends it at the first draw.\n\n" +
           m_triangle.describe();
}

// ----------------------------------------------------------------------------
compages::Status InterleavedTriangle::setUp()
{
    COMPAGES_TRY(m_triangle.loadSources(VERTEX_SHADER, FRAGMENT_SHADER));

    // Three whole corners, each written as its fields in the order of the
    // struct: position, then color. They travel to the graphics card at the
    // first draw.
    m_triangle.setVertices<Vertex>(
        { { { -0.8f, -0.6f }, { 0.1f, 0.9f, 0.9f } },
          { { 0.8f, -0.6f }, { 0.9f, 0.1f, 0.9f } },
          { { 0.0f, 0.8f }, { 0.9f, 0.9f, 0.1f } } });
    return m_triangle.prepare();
}

// ----------------------------------------------------------------------------
void InterleavedTriangle::draw(compages::world::ViewFrame const&)
{
    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_triangle.draw();
}

} // namespace examples
