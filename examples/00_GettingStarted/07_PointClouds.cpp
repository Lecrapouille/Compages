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

//------------------------------------------------------------------------------
std::string PointSphere::description() const
{
    return "Legacy 06_IndexedSphere: a dense UV sphere drawn as GL_POINTS "
           "instead of triangles. Orbit is automatic so --check sees motion.";
}

//------------------------------------------------------------------------------
compages::Status PointSphere::setUp()
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
            points.emplace_back(PointVertex{ compages::core::Vector3f(
                radius * std::cos(phi) * std::cos(theta),
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

//------------------------------------------------------------------------------
void PointSphere::draw(compages::world::ViewFrame const& p_frame)
{
    // One matrix, because a point cloud has no per-object state beyond where
    // it sits. The rotation is what makes the sphere read as a solid.
    const compages::core::Matrix44f projection = compages::core::perspective(
        units::angle::degree_t(60.0), aspect(p_frame), 0.1f, 10.0f);
    const compages::core::Matrix44f view =
        compages::core::lookAt(compages::core::Vector3f(0.0f, 0.0f, 2.5f),
                               compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                               compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Matrix44f model = compages::core::rotate(
        compages::core::Matrix44f(compages::core::matrix::Identity),
        units::angle::radian_t(p_frame.total * 0.6f),
        compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    m_points["mvp"] = projection * view * model;

    compages::gpu::clear({ 0.05f, 0.07f, 0.10f });
    m_points.draw();
}

} // namespace examples
