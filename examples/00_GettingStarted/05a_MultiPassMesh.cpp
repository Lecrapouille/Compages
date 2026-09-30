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

#include "00_GettingStarted/05a_MultiPassMesh.hpp"

#include "Common/ColoredCube.hpp"
#include "Compages/Core/Transformation.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

//! \brief Reads every field of the vertex. The other two shaders deliberately
//! do not, so that the linker drops what they never use and the three pipelines
//! do not all share one way of reading. The block is written out in each shader
//! rather than pasted: a `const char*` does not join with a literal next to it.
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

//! \brief Colour is declared nowhere. The linker drops it from the vertex, so
//! this pipeline reads a shorter prefix of the same buffer.
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

constexpr const char* NORMALS_FRAGMENT = R"(#version 450 core
in vec3 vNormal;
out vec4 oColor;

void main()
{
    oColor = vec4(normalize(vNormal) * 0.5 + 0.5, 1.0);
}
)";

//! \brief Position only. A depth-only pass or a shadow pass looks like this:
//! same vertices, a shader that has no use for the rest.
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

constexpr const char* WIRE_FRAGMENT = R"(#version 450 core
out vec4 oColor;

void main()
{
    oColor = vec4(0.85, 0.85, 0.9, 1.0);
}
)";

constexpr float H = 0.55f;

//------------------------------------------------------------------------------
std::string MultiPassMesh::description() const
{
    return "The level beneath compages::gpu::Drawable: one Buffer<Vertex>, "
           "three "
           "programs and their pipelines, one uniform block. The left "
           "pass lights the mesh, the middle paints its normals, the right "
           "draws only the edges. They share the vertices and the matrices; "
           "they do not share a way of reading a vertex, because each shader "
           "asks for a different subset and the linker drops the rest. The "
           "counters below should show three pipelines, more than one vertex "
           "reader, and three buffers: vertices, indices, and the block.";
}

//------------------------------------------------------------------------------
compages::Status
MultiPassMesh::makePipeline(compages::gpu::Program& p_program,
                            compages::gpu::Pipeline& p_pipeline,
                            char const* p_vertex,
                            char const* p_fragment,
                            compages::gpu::RenderState const& p_state)
{
    // The program is the shader. The pipeline is how that shader reads a
    // Vertex, and which state it draws with. The three passes share the type.
    COMPAGES_TRY(p_program.load(p_vertex, p_fragment));
    COMPAGES_TRY_ASSIGN(
        p_pipeline,
        compages::gpu::Pipeline::create<Vertex>(p_program, p_state));
    return compages::success();
}

//------------------------------------------------------------------------------
compages::Status MultiPassMesh::setUp()
{
    // The cube of 04_DepthAndTransforms, from Common/ColoredCube.hpp. The
    // vertices wait on the CPU and travel at the first draw. The indices never
    // change, so they go straight to an immutable buffer.
    const CubeMesh cube = createCube(2.0f * H);
    m_vertices.assign(cube.vertices);
    COMPAGES_TRY_ASSIGN(
        m_indices, compages::gpu::Buffer<std::uint16_t>::indices(cube.indices));

    compages::gpu::RenderState solid;
    solid.depth_test = true;
    solid.cull = compages::gpu::CullMode::Back;

    compages::gpu::RenderState wire;
    wire.depth_test = true;
    wire.polygon = compages::gpu::PolygonMode::Line;
    // Both sides, so the far edges stay visible instead of being culled as the
    // back of a filled solid would be.
    wire.cull = compages::gpu::CullMode::None;

    COMPAGES_TRY(
        makePipeline(m_lit_program, m_lit, LIT_VERTEX, LIT_FRAGMENT, solid));
    COMPAGES_TRY(makePipeline(
        m_normals_program, m_normals, NORMALS_VERTEX, NORMALS_FRAGMENT, solid));
    COMPAGES_TRY(
        makePipeline(m_wire_program, m_wire, WIRE_VERTEX, WIRE_FRAGMENT, wire));

    // Any of the three programs will do: they declare the same block on the
    // same point. The other two are told nothing, because a binding point
    // belongs to the device, not to a program.
    COMPAGES_TRY_ASSIGN(m_transforms,
                        compages::gpu::TypedUniformBlock<Transforms>::create(
                            m_lit_program, "Transforms"));

    return compages::success();
}

//------------------------------------------------------------------------------
void MultiPassMesh::draw(compages::world::ViewFrame const& p_frame)
{
    const compages::core::Matrix44f identity(compages::core::matrix::Identity);

    Transforms transforms;
    transforms.model =
        compages::core::rotate(identity,
                               units::angle::radian_t(p_frame.total * 0.6f),
                               compages::core::Vector3f(0.35f, 1.0f, 0.15f));
    transforms.view =
        compages::core::lookAt(compages::core::Vector3f(0.0f, 0.0f, 2.6f),
                               compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                               compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    // Each third of the window is a camera of its own, so the aspect is that of
    // one third rather than of the whole, or the cube would look like a brick.
    transforms.projection = compages::core::perspective(
        55.0_deg, aspect(p_frame) / 3.0f, 0.1f, 20.0f);

    m_transforms.assign(transforms);
    if (!compages::gpu::check(m_transforms.bind()))
    {
        return;
    }

    // Three passes side by side, each opened over the window pass the gallery
    // holds, each clearing its own third.
    const std::uint32_t third = p_frame.width / 3u;
    compages::gpu::Pipeline const* pipelines[3] = { &m_lit,
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
        compages::gpu::drawIndexed(*pipelines[i], m_vertices, m_indices);
    }
}

} // namespace examples
