// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/DebugDraw.hpp"

#include "Compages/GPU/VertexLayout.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"
#include "Offer.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace compages::renderer
{

namespace
{

constexpr char const* DEBUG_VERTEX = R"(#version 450 core
in vec3 position;
in vec3 color;

uniform mat4 view;
uniform mat4 projection;

struct V2F
{
    vec3 color;
};
out V2F v2f;

void main()
{
    v2f.color = color;
    gl_Position = projection * view * vec4(position, 1.0);
}
)";

constexpr char const* DEBUG_FRAGMENT = R"(#version 450 core
struct V2F
{
    vec3 color;
};
in V2F v2f;
#include <compages/present.glsl>
out vec4 oColor;

void main()
{
    oColor = vec4(screen(v2f.color), 1.0);
}
)";

} // namespace

Status DebugDraw::ensureInitialized()
{
    if (m_ready)
    {
        return success();
    }

    compages::gpu::ResolvedSource fragment;
    COMPAGES_TRY_ASSIGN(fragment, shaders::compute("debug-lines.frag", DEBUG_FRAGMENT).resolve());
    COMPAGES_TRY(m_program.loadSources(DEBUG_VERTEX, fragment.text));

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
                        compages::gpu::VertexBuffer<DebugVertex>::create(
                            1024u, compages::gpu::BufferUsage::Stream));
    m_ready = true;
    return success();
}

void DebugDraw::clear()
{
    m_pending.clear();
}

void DebugDraw::line(compages::Vector3f const& p_a,
                     compages::Vector3f const& p_b,
                     compages::Vector3f const& p_color)
{
    m_pending.emplace_back(DebugVertex{ p_a, p_color });
    m_pending.emplace_back(DebugVertex{ p_b, p_color });
}

void DebugDraw::box(compages::AABB const& p_local_bounds,
                    compages::Matrix44f const& p_world_matrix,
                    compages::Vector3f const& p_color)
{
    if (p_local_bounds.empty())
    {
        return;
    }

    const compages::AABB world =
        p_local_bounds.transformed(p_world_matrix);
    const compages::Vector3f& mn = world.min;
    const compages::Vector3f& mx = world.max;
    const compages::Vector3f corners[8]{
        compages::Vector3f(mn.x, mn.y, mn.z),
        compages::Vector3f(mx.x, mn.y, mn.z),
        compages::Vector3f(mx.x, mx.y, mn.z),
        compages::Vector3f(mn.x, mx.y, mn.z),
        compages::Vector3f(mn.x, mn.y, mx.z),
        compages::Vector3f(mx.x, mn.y, mx.z),
        compages::Vector3f(mx.x, mx.y, mx.z),
        compages::Vector3f(mn.x, mx.y, mx.z)
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

void DebugDraw::ray(compages::Vector3f const& p_origin,
                    compages::Vector3f const& p_direction,
                    float p_length,
                    compages::Vector3f const& p_color)
{
    line(p_origin, p_origin + (p_direction * p_length), p_color);
}

void DebugDraw::polyline(std::span<const compages::Vector3f> p_points,
                         compages::Vector3f const& p_color)
{
    for (std::size_t k = 1u; k < p_points.size(); ++k)
    {
        line(p_points[k - 1u], p_points[k], p_color);
    }
}

void DebugDraw::arrow(compages::Vector3f const& p_from,
                      compages::Vector3f const& p_to,
                      compages::Vector3f const& p_color)
{
    line(p_from, p_to, p_color);
    const float length = compages::vector::norm(p_to - p_from);
    if (length <= 0.0f)
    {
        return;
    }
    const compages::Vector3f dir = (p_to - p_from) / length;
    // Two directions across the arrow: any vector not along it, crossed.
    const compages::Vector3f other = (std::abs(dir.y) < 0.9f) ? compages::Vector3f(0.0f, 1.0f, 0.0f)
                                                               : compages::Vector3f(1.0f, 0.0f, 0.0f);
    const compages::Vector3f side = compages::vector::normalize(compages::vector::cross(dir, other));
    const compages::Vector3f up = compages::vector::cross(dir, side);
    const float head = 0.2f * length;
    const compages::Vector3f base = p_to - dir * head;
    for (compages::Vector3f const& across : { side, side * -1.0f, up, up * -1.0f })
    {
        line(p_to, base + across * (0.4f * head), p_color);
    }
}

void DebugDraw::frame(compages::Matrix44f const& p_pose, float p_length)
{
    const compages::Vector3f origin = compages::transformPoint(p_pose, { 0.0f, 0.0f, 0.0f });
    line(origin, compages::transformPoint(p_pose, { p_length, 0.0f, 0.0f }), { 1.0f, 0.2f, 0.2f });
    line(origin, compages::transformPoint(p_pose, { 0.0f, p_length, 0.0f }), { 0.2f, 1.0f, 0.2f });
    line(origin, compages::transformPoint(p_pose, { 0.0f, 0.0f, p_length }), { 0.3f, 0.5f, 1.0f });
}

void DebugDraw::frustum(compages::Matrix44f const& p_pose,
                        compages::world::Camera const& p_lens,
                        float p_aspect,
                        float p_length,
                        compages::Vector3f const& p_color)
{
    // Half the width and height of the picture at a distance d: a perspective
    // lens grows with d, an orthographic one does not.
    const bool perspective = (p_lens.projection == compages::world::Camera::Projection::Perspective);
    const float slope = std::tan(0.5f * float(units::angle::radian_t(p_lens.fov).value()));
    auto corners = [&](float p_distance)
    {
        const float h = perspective ? slope * p_distance : p_lens.ortho_half_height;
        const float w = h * p_aspect;
        return std::array<compages::Vector3f, 4>{
            compages::transformPoint(p_pose, { -w, -h, -p_distance }),
            compages::transformPoint(p_pose, { w, -h, -p_distance }),
            compages::transformPoint(p_pose, { w, h, -p_distance }),
            compages::transformPoint(p_pose, { -w, h, -p_distance }) };
    };
    const float near = std::min(p_lens.near_plane, p_length);
    const std::array<compages::Vector3f, 4> a = corners(near);
    const std::array<compages::Vector3f, 4> b = corners(p_length);
    for (std::size_t k = 0u; k < 4u; ++k)
    {
        line(a[k], a[(k + 1u) % 4u], p_color);
        line(b[k], b[(k + 1u) % 4u], p_color);
        line(a[k], b[k], p_color);
    }
}

Status DebugDraw::flush(RenderSnapshot const& p_view)
{
    if (m_pending.empty())
    {
        return success();
    }
    COMPAGES_TRY(ensureInitialized());

    if (m_pending.size() > m_vertices.count())
    {
        COMPAGES_TRY_ASSIGN(m_vertices,
                            compages::gpu::VertexBuffer<DebugVertex>::create(
                                m_pending.size(), compages::gpu::BufferUsage::Stream));
    }

    m_vertices.write(std::span<DebugVertex const>(m_pending));
    m_program.set("view", p_view.camera.view);
    m_program.set("projection", p_view.camera.reversed_projection);
    offer(m_program, "toneMapping", static_cast<int>(p_view.environment.tone_mapping));
    offer(m_program, "linearTarget", p_view.linear_target);
    compages::gpu::draw(m_pipeline, m_vertices.handle(), m_pending.size());
    return success();
}

} // namespace compages::renderer
