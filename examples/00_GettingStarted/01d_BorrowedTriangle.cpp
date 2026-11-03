// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/01d_BorrowedTriangle.hpp"

#include <array>

namespace examples
{

// ----------------------------------------------------------------------------
//! \brief The shader of 01c: reads the position and the color of a corner.
constexpr const char* FILL_VERTEX = R"(#version 450 core

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
constexpr const char* FILL_FRAGMENT = R"(#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

// ----------------------------------------------------------------------------
//! \brief Reads only the position: the color in the Buffer is simply skipped.
constexpr const char* OUTLINE_VERTEX = R"(#version 450 core

in vec2 position;

void main()
{
    gl_Position = vec4(position * 1.05, 0.0, 1.0);
}
)";

// ----------------------------------------------------------------------------
constexpr const char* OUTLINE_FRAGMENT = R"(#version 450 core

uniform vec3 tint;
out vec4 oColor;

void main()
{
    oColor = vec4(tint, 1.0);
}
)";

// ----------------------------------------------------------------------------
std::string BorrowedTriangle::description() const
{
    return "The corners live in a Buffer that this demo owns. Two drawables "
           "borrow it: one fills the triangle, the other draws its edges. "
           "The corners exist once on the graphics card, and nothing is "
           "copied.\n\n" +
           m_outline.describe();
}

// ----------------------------------------------------------------------------
compages::Status BorrowedTriangle::setUp()
{
    // The corners go into our own Buffer. They never change, so the preset
    // is Static: written once, no copy kept on the CPU.
    const std::array<Vertex, 3u> corners{
        { { { -0.8f, -0.6f }, { 0.1f, 0.9f, 0.9f } },
          { { 0.8f, -0.6f }, { 0.9f, 0.1f, 0.9f } },
          { { 0.0f, 0.8f }, { 0.9f, 0.9f, 0.1f } } }
    };
    COMPAGES_TRY_ASSIGN(m_corners,
                        compages::gpu::VertexBuffer<Vertex>::from(
                            corners, compages::gpu::BufferUsage::Static));

    // Both drawables read the same Buffer, where it is.
    COMPAGES_TRY(m_fill.loadSources(FILL_VERTEX, FILL_FRAGMENT));
    m_fill.borrowVertices(m_corners);

    COMPAGES_TRY(m_outline.loadSources(OUTLINE_VERTEX, OUTLINE_FRAGMENT));
    m_outline.borrowVertices(m_corners);
    m_outline.state().primitive = compages::gpu::Primitive::LineLoop;
    m_outline["tint"] = compages::Vector3f(1.0f, 1.0f, 1.0f);

    COMPAGES_TRY(m_fill.prepare());
    return m_outline.prepare();
}

// ----------------------------------------------------------------------------
void BorrowedTriangle::draw(compages::world::ViewFrame const&)
{
    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_fill.draw();
    m_outline.draw();
}

} // namespace examples
