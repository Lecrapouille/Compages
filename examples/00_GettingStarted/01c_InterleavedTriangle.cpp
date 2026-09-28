// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/01c_InterleavedTriangle.hpp"

#include <array>

namespace examples
{

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

constexpr const char* FRAGMENT_SHADER = R"(#version 450 core

in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

std::string InterleavedTriangle::description() const
{
    return "A triangle from a struct Vertex { position; color; }. Its fields are "
           "read off the struct at compile time and matched to the shader by "
           "name. The vertices never change, so they are put once into an "
           "Immutable buffer, which the driver may keep where the GPU reads it "
           "fastest, and which can never be written again:\n\n" +
           m_triangle.describe();
}

compages::gpu::Status InterleavedTriangle::setUp()
{
    COMPAGES_TRY(m_triangle.load(VERTEX_SHADER, FRAGMENT_SHADER));

    // Whole vertices, in the order the fields are declared.
    const std::array<Vertex, 3u> corners{ { { { -0.8f, -0.6f }, { 0.1f, 0.9f, 0.9f } },
                                            { { 0.8f, -0.6f }, { 0.9f, 0.1f, 0.9f } },
                                            { { 0.0f, 0.8f }, { 0.9f, 0.9f, 0.1f } } } };

    // Filled when it is made, never written again. No CPU copy is kept: there
    // will never be anything to send.
    COMPAGES_TRY_ASSIGN(m_vertices,
                        compages::gpu::Buffer<Vertex>::from(corners,
                                                  { .usage = compages::gpu::BufferUsage::Immutable,
                                                    .cpu_mirror = false }));

    // The drawable reads the buffer where it is, rather than copying it.
    m_triangle.vertices(m_vertices);
    return m_triangle.prepare();
}

void InterleavedTriangle::draw(Frame const&)
{
    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_triangle.draw();
}

} // namespace examples
