//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

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

//------------------------------------------------------------------------------
std::string InterleavedTriangle::description() const
{
    return "A triangle from a struct Vertex { position; color; }. Its fields "
           "are "
           "read off the struct at compile time and matched to the shader by "
           "name. The vertices never change, so they are put once into an "
           "Immutable buffer, which the driver may keep where the GPU reads it "
           "fastest, and which can never be written again:\n\n" +
           m_triangle.describe();
}

//------------------------------------------------------------------------------
compages::Status InterleavedTriangle::setUp()
{
    COMPAGES_TRY(m_triangle.load(VERTEX_SHADER, FRAGMENT_SHADER));

    // Whole vertices, in the order the fields are declared.
    const std::array<Vertex, 3u> corners{
        { { { -0.8f, -0.6f }, { 0.1f, 0.9f, 0.9f } },
          { { 0.8f, -0.6f }, { 0.9f, 0.1f, 0.9f } },
          { { 0.0f, 0.8f }, { 0.9f, 0.9f, 0.1f } } }
    };

    // Filled when it is made, never written again. No CPU copy is kept: there
    // will never be anything to send.
    COMPAGES_TRY_ASSIGN(m_vertices,
                        compages::gpu::Buffer<Vertex>::from(
                            corners,
                            { .usage = compages::gpu::BufferUsage::Immutable,
                              .cpu_mirror = false }));

    // The drawable reads the buffer where it is, rather than copying it.
    m_triangle.vertices(m_vertices);
    return m_triangle.prepare();
}

//------------------------------------------------------------------------------
void InterleavedTriangle::draw(compages::world::ViewFrame const&)
{
    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_triangle.draw();
}

} // namespace examples
