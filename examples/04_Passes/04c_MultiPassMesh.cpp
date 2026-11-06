// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "04_Passes/04c_MultiPassMesh.hpp"

#include "Common/ColoredCube.hpp"
#include "Compages/Core/Transformation.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

// ----------------------------------------------------------------------------
//! \brief Reads every field of the vertex. The other two shaders read fewer,
//! to show that each one takes only what it needs from the same Buffer. The
//! block is written out in each shader rather than pasted: a `const char*`
//! does not join with a literal next to it.
// ----------------------------------------------------------------------------
constexpr const char* LIT_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;
in vec3 color;

layout(std140, binding = 0) uniform Transforms
{
    mat4 projection;
    mat4 view;
    mat4 model;
} u;

out vec3 vNormal;
out vec3 vColor;

void main()
{
    vNormal = mat3(u.model) * normal;
    vColor = color;
    gl_Position = u.projection * u.view * u.model * vec4(position, 1.0);
}
)";

// ----------------------------------------------------------------------------
constexpr const char* LIT_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in vec3 vColor;
out vec4 oColor;

void main()
{
    const vec3 toLight = normalize(vec3(0.4, 0.8, 0.6));
    float lit = 0.25 + (0.75 * max(dot(normalize(vNormal), toLight), 0.0));
    oColor = vec4(vColor * lit, 1.0);
}
)";

// ----------------------------------------------------------------------------
//! \brief Colour is declared nowhere: this shader simply skips that field of
//! the same Buffer.
// ----------------------------------------------------------------------------
constexpr const char* NORMALS_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;

layout(std140, binding = 0) uniform Transforms
{
    mat4 projection;
    mat4 view;
    mat4 model;
} u;

out vec3 vNormal;

void main()
{
    vNormal = mat3(u.model) * normal;
    gl_Position = u.projection * u.view * u.model * vec4(position, 1.0);
}
)";

// ----------------------------------------------------------------------------
constexpr const char* NORMALS_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
out vec4 oColor;

void main()
{
    oColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}
)";

// ----------------------------------------------------------------------------
//! \brief Position only. A depth-only pass or a shadow pass looks like this:
//! same vertices, a shader that has no use for the rest.
// ----------------------------------------------------------------------------
constexpr const char* WIRE_VERTEX = R"(#version 450 core
in vec3 position;

layout(std140, binding = 0) uniform Transforms
{
    mat4 projection;
    mat4 view;
    mat4 model;
} u;

void main()
{
    gl_Position = u.projection * u.view * u.model * vec4(position, 1.0);
}
)";

// ----------------------------------------------------------------------------
constexpr const char* WIRE_FRAGMENT = R"(#version 450 core
out vec4 oColor;

void main()
{
    oColor = vec4(0.85, 0.85, 0.9, 1.0);
}
)";

constexpr float H = 0.55f;

// ----------------------------------------------------------------------------
std::string MultiPassMesh::description() const
{
    return "One cube, three drawables, one Buffer. The cube lives once on the "
           "graphics card, in a Buffer this demo owns, and the three drawables "
           "borrow it. The left one lights the cube, the middle one paints its "
           "normals, the right one draws only its edges. Each shader reads "
           "only the fields it needs. The three also share the matrices, "
           "through one uniform block.";
}

// ----------------------------------------------------------------------------
compages::Status MultiPassMesh::setUp()
{
    // The cube of 03c_DepthAndTransforms, from Common/ColoredCube.hpp. It
    // never changes shape, and three drawables read it: so it goes once into
    // our own Static VertexBuffer, and each drawable borrows it.
    const CubeMesh cube = createCube(2.0f * H);
    COMPAGES_TRY_ASSIGN(m_vertices,
                        compages::gpu::VertexBuffer<Vertex>::from(
                            cube.vertices, compages::gpu::BufferUsage::Static));

    COMPAGES_TRY(m_lit.loadSources(LIT_VERTEX, LIT_FRAGMENT));
    COMPAGES_TRY(m_normals.loadSources(NORMALS_VERTEX, NORMALS_FRAGMENT));
    COMPAGES_TRY(m_wire.loadSources(WIRE_VERTEX, WIRE_FRAGMENT));
    for (compages::gpu::Drawable* pass : { &m_lit, &m_normals, &m_wire })
    {
        pass->borrowVertices(m_vertices);
        pass->indices(cube.indices);
        pass->state().depth_test = true;
        pass->state().cull = compages::gpu::CullMode::Back;
    }

    // The wireframe draws the edges of the triangles, on both sides, so the
    // far edges stay visible instead of being hidden as a back face.
    m_wire.state().polygon = compages::gpu::PolygonMode::Line;
    m_wire.state().cull = compages::gpu::CullMode::None;

    // Any of the three shaders will do: they declare the same block on the
    // same point. The other two are told nothing, because a binding point
    // belongs to the graphics card, not to a shader.
    COMPAGES_TRY_ASSIGN(m_transforms,
                        compages::gpu::TypedUniformBlock<Transforms>::create(
                            m_lit.program(), "Transforms"));

    COMPAGES_TRY(m_lit.prepare());
    COMPAGES_TRY(m_normals.prepare());
    return m_wire.prepare();
}

// ----------------------------------------------------------------------------
void MultiPassMesh::draw(compages::world::ViewFrame const& p_frame)
{
    const compages::Matrix44f identity(compages::matrix::Identity);

    Transforms transforms;
    transforms.model =
        compages::rotate(identity,
                         units::angle::radian_t(p_frame.total * 0.6f),
                         compages::Vector3f(0.35f, 1.0f, 0.15f));

    transforms.view = compages::lookAt(compages::Vector3f(0.0f, 0.0f, 2.6f),
                                       compages::Vector3f(0.0f, 0.0f, 0.0f),
                                       compages::Vector3f(0.0f, 1.0f, 0.0f));

    // Each third of the window is a camera of its own, so the aspect is that of
    // one third rather than of the whole, or the cube would look like a brick.
    transforms.projection =
        compages::perspective(55.0_deg, aspect(p_frame) / 3.0f, 0.1f, 20.0f);

    m_transforms.assign(transforms);
    if (!compages::gpu::check(m_transforms.bind()))
    {
        return;
    }

    // Three passes side by side, each opened over the window pass the gallery
    // holds, each clearing its own third.
    const std::uint32_t third = p_frame.width / 3u;
    std::array<compages::gpu::Drawable*, 3> passes = { &m_lit,
                                                       &m_normals,
                                                       &m_wire };
    for (std::uint32_t i = 0u; i < 3u; ++i)
    {
        const std::uint32_t width =
            (i == 2u) ? (p_frame.width - 2u * third) : third;
        compages::gpu::RenderPass pass(
            { .x = i * third,
              .width = width,
              .height = p_frame.height,
              .color = { 0.07f, 0.07f, 0.1f, 1.0f } });
        passes[i]->draw();
    }
}

} // namespace examples
