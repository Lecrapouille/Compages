// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/01b_Triangle.hpp"

namespace examples
{

//! \brief The shader is where everything starts: it declares two attributes,
//! position and color, and those names are all the C++ side needs to know.
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

std::string Triangle::description() const
{
    return "A shader and its data, filled by name: drawable[\"position\"] and "
           "drawable[\"color\"] are the attributes the shader declares. The "
           "values "
           "are stored interleaved and sent to the GPU at the first draw:\n\n" +
           m_triangle.describe();
}

compages::Status Triangle::setUp()
{
    // The one step that can fail for a reason outside the program: a shader
    // that does not compile. COMPAGES_TRY hands the compiler log to the
    // gallery.
    COMPAGES_TRY(m_triangle.load(VERTEX_SHADER, FRAGMENT_SHADER));

    // One value per vertex, attribute by attribute. A misspelled name or a
    // vec2 given three numbers is a message on the screen, listing what the
    // shader really declares.
    m_triangle["position"] = { { -0.8f, -0.6f },
                               { 0.8f, -0.6f },
                               { 0.0f, 0.8f } };
    m_triangle["color"] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

    // Optional: checks everything now rather than at the first frame.
    return m_triangle.prepare();
}

void Triangle::draw(compages::world::ViewFrame const&)
{
    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    m_triangle.draw();
}

} // namespace examples
