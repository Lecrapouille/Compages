// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_ScientificAndCompute/15_Lorenz.hpp"

#include "Compages/Core/Transformation.hpp"

#include <cmath>

using namespace units::literals;

namespace examples
{

constexpr std::size_t CAPACITY = 20000u;
constexpr std::size_t STEPS_PER_FRAME = 40u;
constexpr float DT = 0.005f;
constexpr float SIGMA = 10.0f;
constexpr float RHO = 28.0f;
constexpr float BETA = 8.0f / 3.0f;

constexpr const char* VERTEX = R"(#version 450 core
in vec3 position;
in vec3 color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 vColor;

void main()
{
    vColor = color;
    gl_Position = projection * view * model * vec4(position, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec3 vColor;
out vec4 oColor;

void main()
{
    oColor = vec4(vColor, 1.0);
}
)";

std::string Lorenz::description() const
{
    return "A line that grows. Each step of the attractor is one emplace_back(), "
           "and the next draw sends only the points added since the last one. "
           "HeightMap rewrote every vertex; this appends. The trail stops at "
           "twenty thousand points.";
}

void Lorenz::step()
{
    // One Euler step of the attractor, then one vertex at the end of the trail.
    const Vector3f s = m_state;
    m_state.x += DT * SIGMA * (s.y - s.x);
    m_state.y += DT * ((s.x * (RHO - s.z)) - s.y);
    m_state.z += DT * ((s.x * s.y) - (BETA * s.z));

    // The attractor spans tens of units: bring it in front of the camera.
    const Vector3f point(m_state.x * 0.04f, (m_state.z * 0.04f) - 1.0f, m_state.y * 0.04f);
    const Vector3f color(0.5f + (0.5f * std::tanh(m_state.x * 0.05f)),
                         0.35f + (0.45f * std::tanh(m_state.y * 0.05f)),
                         0.7f + (0.3f * std::tanh((m_state.z - 25.0f) * 0.04f)));
    m_trail.emplace_back(Vertex{ point, color });
}

compages::gpu::Status Lorenz::setUp()
{
    COMPAGES_TRY(m_trail.load(VERTEX, FRAGMENT));
    m_trail.primitive(compages::gpu::Primitive::LineStrip).depthTest();
    m_trail["view"] = compages::matrix::lookAt(Vector3f(0.0f, 0.2f, 3.4f),
                                     Vector3f(0.0f, 0.0f, 0.0f),
                                     Vector3f(0.0f, 1.0f, 0.0f));

    // Enough of a trail that the first frame already looks like the attractor.
    for (std::size_t i = 0u; i < 800u; ++i)
    {
        step();
    }
    return m_trail.prepare();
}

void Lorenz::draw(Frame const& p_frame)
{
    // A few new points per frame. Past the capacity the trail stops growing;
    // what is already there keeps turning.
    for (std::size_t i = 0u; (i < STEPS_PER_FRAME) && (m_trail.count() < CAPACITY); ++i)
    {
        step();
    }

    m_trail["model"] = compages::matrix::rotate(Matrix44f(compages::matrix::Identity),
                                      units::angle::radian_t(p_frame.total * 0.35f),
                                      Vector3f(0.15f, 1.0f, 0.1f));
    m_trail["projection"] =
        compages::matrix::perspective(50.0_deg, aspect(p_frame), 0.1f, 20.0f);

    compages::gpu::clear({ 0.04f, 0.04f, 0.06f });
    compages::gpu::clearDepth();
    m_trail.draw();
}

} // namespace examples
