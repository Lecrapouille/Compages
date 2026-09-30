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

#include "Compages/Renderer/Render/DebugDraw.hpp"

#include "Compages/GPU/Core/Layout.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Shader.hpp"

#include <array>

namespace compages::renderer
{

namespace
{

constexpr char const* DEBUG_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 color;

uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

constexpr char const* DEBUG_FRAGMENT = R"(#version 450 core
in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

} // namespace

//------------------------------------------------------------------------------
Status DebugDraw::ensureInitialized()
{
    if (m_ready)
    {
        return success();
    }

    COMPAGES_TRY_ASSIGN(
        m_program,
        compages::gpu::Program::fromSources(DEBUG_VERTEX, DEBUG_FRAGMENT));

    const compages::gpu::VertexLayout layout =
        compages::gpu::VertexLayout::of<DebugVertex>();
    compages::gpu::RenderState state;
    state.depth_test = true;
    state.depth_write = true;
    state.cull = compages::gpu::CullMode::None;
    state.primitive = compages::gpu::Primitive::Lines;
    COMPAGES_TRY_ASSIGN(
        m_pipeline,
        compages::gpu::Pipeline::create<DebugVertex>(m_program, layout, state));

    COMPAGES_TRY_ASSIGN(m_vertices,
                        compages::gpu::Buffer<DebugVertex>::create(
                            1024u,
                            compages::gpu::BufferKind::Vertex,
                            compages::gpu::BufferUsage::Dynamic));
    m_ready = true;
    return success();
}

//------------------------------------------------------------------------------
void DebugDraw::clear()
{
    m_pending.clear();
}

//------------------------------------------------------------------------------
void DebugDraw::line(compages::core::Vector3f const& p_a,
                     compages::core::Vector3f const& p_b,
                     compages::core::Vector3f const& p_color)
{
    m_pending.emplace_back(DebugVertex{ p_a, p_color });
    m_pending.emplace_back(DebugVertex{ p_b, p_color });
}

//------------------------------------------------------------------------------
void DebugDraw::box(compages::core::AABB const& p_local_bounds,
                    compages::core::Matrix44f const& p_world_matrix,
                    compages::core::Vector3f const& p_color)
{
    if (p_local_bounds.empty())
    {
        return;
    }

    const compages::core::AABB world =
        p_local_bounds.transformed(p_world_matrix);
    const compages::core::Vector3f& mn = world.min;
    const compages::core::Vector3f& mx = world.max;
    const compages::core::Vector3f corners[8]{
        compages::core::Vector3f(mn.x, mn.y, mn.z),
        compages::core::Vector3f(mx.x, mn.y, mn.z),
        compages::core::Vector3f(mx.x, mx.y, mn.z),
        compages::core::Vector3f(mn.x, mx.y, mn.z),
        compages::core::Vector3f(mn.x, mn.y, mx.z),
        compages::core::Vector3f(mx.x, mn.y, mx.z),
        compages::core::Vector3f(mx.x, mx.y, mx.z),
        compages::core::Vector3f(mn.x, mx.y, mx.z)
    };
    const std::array<std::pair<int, int>, 12u> edges{ { { 0, 1 },
                                                        { 1, 2 },
                                                        { 2, 3 },
                                                        { 3, 0 },
                                                        { 4, 5 },
                                                        { 5, 6 },
                                                        { 6, 7 },
                                                        { 7, 4 },
                                                        { 0, 4 },
                                                        { 1, 5 },
                                                        { 2, 6 },
                                                        { 3, 7 } } };
    for (auto const& edge : edges)
    {
        line(corners[edge.first], corners[edge.second], p_color);
    }
}

//------------------------------------------------------------------------------
void DebugDraw::ray(compages::core::Vector3f const& p_origin,
                    compages::core::Vector3f const& p_direction,
                    float p_length,
                    compages::core::Vector3f const& p_color)
{
    line(p_origin, p_origin + (p_direction * p_length), p_color);
}

//------------------------------------------------------------------------------
Status DebugDraw::flush(CameraFrame const& p_camera)
{
    if (m_pending.empty())
    {
        return success();
    }
    COMPAGES_TRY(ensureInitialized());

    if (m_pending.size() > m_vertices.count())
    {
        COMPAGES_TRY_ASSIGN(m_vertices,
                            compages::gpu::Buffer<DebugVertex>::create(
                                m_pending.size(),
                                compages::gpu::BufferKind::Vertex,
                                compages::gpu::BufferUsage::Dynamic));
    }

    m_vertices.write(std::span<DebugVertex const>(m_pending));
    m_program.set("view", p_camera.view);
    m_program.set("projection", p_camera.projection);
    compages::gpu::draw(m_pipeline, m_vertices.handle(), m_pending.size());
    return success();
}

} // namespace compages::renderer
