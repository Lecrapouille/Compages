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

#include "10_ScientificAndCompute/10a_HeightMap.hpp"

#include "Compages/Core/Transformation.hpp"

#include <cmath>
#include <vector>

using namespace units::literals;

namespace examples
{

constexpr std::uint32_t SIDE = 320u;
constexpr float EXTENT = 1.6f;
constexpr float STEP = (2.0f * EXTENT) / static_cast<float>(SIDE - 1u);

constexpr const char* VERTEX = R"(#version 450 core
in vec3 position;
in vec3 normal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vNormal;
out float vHeight;

void main()
{
    vNormal = mat3(model) * normal;
    vHeight = position.y;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec3 vNormal;
in float vHeight;
out vec4 oColor;

void main()
{
    // Height paints the colour, the normal paints the light, so a wave reads
    // as a surface and not as a flat gradient.
    const vec3 toLight = normalize(vec3(0.35, 0.8, 0.45));
    float lit = 0.2 + (0.8 * max(dot(normalize(vNormal), toLight), 0.0));
    vec3 low = vec3(0.15, 0.25, 0.45);
    vec3 high = vec3(0.85, 0.75, 0.45);
    float t = clamp(vHeight * 2.5 + 0.45, 0.0, 1.0);
    oColor = vec4(mix(low, high, t) * lit, 1.0);
}
)";

//------------------------------------------------------------------------------
static float heightAt(float p_x, float p_z, float p_time)
{
    return (0.18f * std::sin((6.0f * p_x) + p_time) *
            std::cos((6.0f * p_z) + (p_time * 0.7f))) +
           (0.07f * std::sin((12.0f * p_x * p_z) + (p_time * 1.3f)));
}

//------------------------------------------------------------------------------
//! \brief The normal of the surface, from the function itself: the lighting
//! follows the waves without a second pass over the neighbours.
static compages::core::Vector3f normalAt(float p_x, float p_z, float p_time)
{
    const float y = heightAt(p_x, p_z, p_time);
    const compages::core::Vector3f dx(
        STEP, heightAt(p_x + STEP, p_z, p_time) - y, 0.0f);
    const compages::core::Vector3f dz(
        0.0f, heightAt(p_x, p_z + STEP, p_time) - y, STEP);
    return compages::core::vector::normalize(
        compages::core::vector::cross(dz, dx));
}

//------------------------------------------------------------------------------
std::string HeightMap::description() const
{
    return "A hundred thousand vertices whose height is a function of place "
           "and "
           "time, decided on the CPU. They are changed where they are, through "
           "vertices<Vertex>(), and all of them travel at the next draw. The "
           "triangles naming the points never move, so the indices are sent "
           "once. 02 changed one vertex; this changes them all.";
}

//------------------------------------------------------------------------------
compages::Status HeightMap::setUp()
{
    COMPAGES_TRY(m_surface.load(VERTEX, FRAGMENT));

    // A flat grid: x and z never change, only the height will.
    std::vector<Vertex> grid;
    grid.reserve(static_cast<std::size_t>(SIDE) * SIDE);
    for (std::uint32_t z = 0u; z < SIDE; ++z)
    {
        for (std::uint32_t x = 0u; x < SIDE; ++x)
        {
            grid.emplace_back(
                Vertex{ compages::core::Vector3f(-EXTENT + (float(x) * STEP),
                                                 0.0f,
                                                 -EXTENT + (float(z) * STEP)),
                        compages::core::Vector3f(0.0f, 1.0f, 0.0f) });
        }
    }

    // Two triangles per cell of the grid.
    std::vector<std::uint32_t> indices;
    indices.reserve(static_cast<std::size_t>(SIDE - 1u) * (SIDE - 1u) * 6u);
    for (std::uint32_t z = 0u; z < (SIDE - 1u); ++z)
    {
        for (std::uint32_t x = 0u; x < (SIDE - 1u); ++x)
        {
            const std::uint32_t here = (z * SIDE) + x;
            indices.insert(indices.end(),
                           { here,
                             here + SIDE,
                             here + 1u,
                             here + 1u,
                             here + SIDE,
                             here + SIDE + 1u });
        }
    }

    m_surface.vertices(grid);
    m_surface.indices(indices);
    m_surface.depthTest();
    m_surface["view"] =
        compages::core::lookAt(compages::core::Vector3f(0.0f, 1.8f, 3.2f),
                               compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                               compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    return m_surface.prepare();
}

//------------------------------------------------------------------------------
void HeightMap::draw(compages::world::ViewFrame const& p_frame)
{
    // Every vertex, in place. Taking the span is what marks the whole range
    // dirty, so the next draw sends all of them and none of the indices.
    const float t = p_frame.total;
    for (Vertex& v : m_surface.vertices<Vertex>())
    {
        v.position.y = heightAt(v.position.x, v.position.z, t);
        v.normal = normalAt(v.position.x, v.position.z, t);
    }

    m_surface["model"] = compages::core::rotate(
        compages::core::Matrix44f(compages::core::matrix::Identity),
        units::angle::radian_t(t * 0.25f),
        compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    m_surface["projection"] =
        compages::core::perspective(50.0_deg, aspect(p_frame), 0.1f, 20.0f);

    compages::gpu::clear({ 0.06f, 0.07f, 0.1f });
    compages::gpu::clearDepth();
    m_surface.draw();
}

} // namespace examples
