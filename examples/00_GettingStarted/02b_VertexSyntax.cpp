// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/02b_VertexSyntax.hpp"

#include "Common/Gui.hpp"

#include <cmath>
#include <vector>

namespace examples
{

// ----------------------------------------------------------------------------
//! \brief One shader for the nine: a small triangle moved to its cell.
constexpr const char* VERTEX_SHADER = R"(#version 450 core

in vec2 position;
in vec3 color;
uniform vec2 offset;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = vec4(position * 0.25 + offset, 0.0, 1.0);
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

//! \brief What each triangle shows, in the order of the grid.
constexpr std::array<const char*, 9u> SYNTAX{
    "d[\"position\"] = {...};  d[\"color\"].set(i, c)",
    "d[\"position\"] = positions  (a whole attribute)",
    "d.setVertices<V>({ {...}, {...}, {...} })  (braces)",
    "d.setVertices(data) + d.editVertex<V>(i)",
    "d.setVertices(data) + d.editVertices<V>(first, count)",
    "d.setVertices(data) + d.editVertices<V>()",
    "d.emplace_back(V{ position, color })",
    "d.borrowVertices(VertexBuffer made Static)",
    "d.borrowVertices(VertexBuffer made Dynamic); buffer[i] = ...; upload()"
};

//! \brief The triangle every cell starts from.
const std::vector<compages::Vector2f> POSITIONS{ { -1.0f, -0.8f },
                                                 { 1.0f, -0.8f },
                                                 { 0.0f, 0.9f } };
const std::vector<compages::Vector3f> COLORS{ { 0.9f, 0.3f, 0.3f },
                                              { 0.3f, 0.9f, 0.3f },
                                              { 0.3f, 0.4f, 0.9f } };

// ----------------------------------------------------------------------------
std::string VertexSyntax::description() const
{
    return "Nine triangles, one per way of writing vertices: by name, from a "
           "struct, one at a time, or borrowed from a Buffer we own; then "
           "every way of changing them. The panel says which is which. The "
           "result does not matter, the syntax does.";
}

// ----------------------------------------------------------------------------
compages::Status VertexSyntax::setUp()
{
    std::vector<Vertex> corners;
    corners.reserve(POSITIONS.size());
    for (std::size_t i = 0u; i < POSITIONS.size(); ++i)
    {
        corners.emplace_back(POSITIONS[i], COLORS[i]);
    }

    // Same shader for all nine, each moved to its cell of a 3 x 3 grid.
    for (std::size_t i = 0u; i < m_all.size(); ++i)
    {
        COMPAGES_TRY(m_all[i]->loadSources(VERTEX_SHADER, FRAGMENT_SHADER));
        (*m_all[i])["offset"] = compages::Vector2f(
            -0.65f + 0.65f * float(i % 3u), 0.6f - 0.6f * float(i / 3u));
    }

    // 1. By name, one attribute at a time, as in 01b. Always changeable.
    m_by_name["position"] = POSITIONS;
    m_by_name["color"] = COLORS;

    // 2. By name too; draw() gives the whole position attribute each frame.
    m_whole_attribute["position"] = POSITIONS;
    m_whole_attribute["color"] = COLORS;

    // 3. From a struct, written between braces.
    m_braces.setVertices<Vertex>({ { POSITIONS[0], COLORS[0] },
                                   { POSITIONS[1], COLORS[1] },
                                   { POSITIONS[2], COLORS[2] } });

    // 4 to 6. From a container of structs (a std::vector here), changed each
    // frame in three ways (see draw()).
    m_edit_one.setVertices(corners);
    m_edit_run.setVertices(corners);
    m_edit_all.setVertices(corners);

    // 7. One vertex at a time, added at the end.
    for (Vertex const& corner : corners)
    {
        m_grown.emplace_back(corner);
    }

    // 8. Borrowed from a Buffer we own, which never changes.
    COMPAGES_TRY_ASSIGN(m_static,
                        compages::gpu::VertexBuffer<Vertex>::from(
                            corners, compages::gpu::BufferUsage::Static));
    m_borrow_static.borrowVertices(m_static);

    // 9. Borrowed from a Buffer we own that keeps a CPU copy, so that we can
    // change it ourselves and send it with upload().
    COMPAGES_TRY_ASSIGN(m_dynamic,
                        compages::gpu::VertexBuffer<Vertex>::from(
                            corners, compages::gpu::BufferUsage::Dynamic));
    m_borrow_dynamic.borrowVertices(m_dynamic);

    for (compages::gpu::Drawable* drawable : m_all)
    {
        COMPAGES_TRY(drawable->prepare());
    }
    return compages::success();
}

// ----------------------------------------------------------------------------
void VertexSyntax::draw(compages::world::ViewFrame const& p_frame)
{
    const float t = p_frame.total;
    const float wave = 0.5f + 0.5f * std::sin(2.0f * t);

    // 1. One value of one attribute, by name.
    m_by_name["color"].set(0u, compages::Vector3f(wave, 0.3f, 0.3f));

    // 2. A whole attribute, by name: here the triangle turns.
    std::vector<compages::Vector2f> turned;
    turned.reserve(POSITIONS.size());
    for (compages::Vector2f const& p : POSITIONS)
    {
        turned.emplace_back(p.x * std::cos(t) - p.y * std::sin(t),
                            p.x * std::sin(t) + p.y * std::cos(t));
    }
    m_whole_attribute["position"] = turned;

    // 3. Nothing changes.

    // 4. One vertex: the top corner goes up and down.
    m_edit_one.editVertex<Vertex>(2u).position.y = 0.4f + 0.5f * wave;

    // 5. A run of vertices: the first two change color.
    for (Vertex& vertex : m_edit_run.editVertices<Vertex>(0u, 2u))
    {
        vertex.color = compages::Vector3f(0.3f, wave, 0.9f);
    }

    // 6. All the vertices: the triangle breathes.
    const float scale = 0.7f + 0.3f * wave;
    std::size_t i = 0u;
    for (Vertex& vertex : m_edit_all.editVertices<Vertex>())
    {
        vertex.position = POSITIONS[i++] * scale;
    }

    // 7 and 8: nothing changes.

    // 9. Our own Buffer: change its CPU copy, then send it ourselves. A
    // borrowed Buffer is ours, so the drawable does not send it for us.
    m_dynamic[1u].position.x = 0.5f + 0.5f * wave;
    if (!compages::gpu::check(m_dynamic.upload()))
    {
        return;
    }

    compages::gpu::clear({ 0.1f, 0.1f, 0.15f });
    for (compages::gpu::Drawable* drawable : m_all)
    {
        drawable->draw();
    }
}

// ----------------------------------------------------------------------------
void VertexSyntax::controls()
{
    ImGui::TextUnformatted("Left to right, top to bottom:");
    for (std::size_t i = 0u; i < SYNTAX.size(); ++i)
    {
        ImGui::Text("%zu. %s", i + 1u, SYNTAX[i]);
    }
}

} // namespace examples
