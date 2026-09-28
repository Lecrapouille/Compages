// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/07_PointClouds.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/RenderPass.hpp"

#include <cmath>
#include <vector>

using namespace units::literals;

namespace examples
{

constexpr char const* VERTEX = R"(#version 450 core
in vec3 position;
uniform mat4 mvp;
void main()
{
    gl_PointSize = 2.0;
    gl_Position = mvp * vec4(position, 1.0);
}
)";

constexpr char const* FRAGMENT = R"(#version 450 core
out vec4 oColor;
void main()
{
    oColor = vec4(0.85, 0.55, 0.35, 1.0);
}
)";

std::string PointSphere::description() const
{
    return "Legacy 06_IndexedSphere: a dense UV sphere drawn as GL_POINTS "
           "instead of triangles. Orbit is automatic so --check sees motion.";
}

compages::gpu::Status PointSphere::setUp()
{
    constexpr std::uint32_t lon = 80u;
    constexpr std::uint32_t lat = 40u;
    constexpr float radius = 0.8f;

    // A UV sphere stored as points, not as triangles: one vertex per sample,
    // and nothing naming them.
    std::vector<PointVertex> points;
    points.reserve(lon * lat);
    for (std::uint32_t i = 0u; i < lat; ++i)
    {
        const float v = static_cast<float>(i) / static_cast<float>(lat - 1u);
        const float phi = (v - 0.5f) * M_PIf;
        for (std::uint32_t j = 0u; j < lon; ++j)
        {
            const float u = static_cast<float>(j) / static_cast<float>(lon);
            const float theta = u * 2.0f * M_PIf;
            points.emplace_back(PointVertex{
                Vector3f(radius * std::cos(phi) * std::cos(theta),
                         radius * std::sin(phi),
                         radius * std::cos(phi) * std::sin(theta)) });
        }
    }

    COMPAGES_TRY(m_points.load(VERTEX, FRAGMENT));
    m_points.vertices(points);
    // Points, or the drawable would try to make triangles out of the cloud.
    m_points.primitive(compages::gpu::Primitive::Points);
    return m_points.prepare();
}

void PointSphere::draw(Frame const& p_frame)
{
    // One matrix, because a point cloud has no per-object state beyond where
    // it sits. The rotation is what makes the sphere read as a solid.
    const Matrix44f projection = compages::matrix::perspective(
        units::angle::degree_t(60.0), aspect(p_frame), 0.1f, 10.0f);
    const Matrix44f view = compages::matrix::lookAt(Vector3f(0.0f, 0.0f, 2.5f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f model =
        compages::matrix::rotate(Matrix44f(compages::matrix::Identity),
                       units::angle::radian_t(p_frame.total * 0.6f),
                       Vector3f(0.0f, 1.0f, 0.0f));
    m_points["mvp"] = model * view * projection;

    compages::gpu::clear({ 0.05f, 0.07f, 0.10f });
    m_points.draw();
}

} // namespace examples
