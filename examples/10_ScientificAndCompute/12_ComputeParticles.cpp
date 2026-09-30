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

#include "10_ScientificAndCompute/12_ComputeParticles.hpp"

#include <random>
#include <vector>

namespace examples
{

constexpr std::uint32_t COUNT = 4096u;

constexpr const char* STEP = R"(#version 450 core
layout(local_size_x = 64) in;

struct Particle
{
    vec2 position;
    vec2 velocity;
    vec4 color;
};

layout(std430, binding = 0) buffer Particles
{
    Particle particles[];
};

uniform float dt;
uniform uint count;

void main()
{
    uint i = gl_GlobalInvocationID.x;
    if (i >= count)
    {
        return;
    }

    // Move, then bounce on the sides of the box.
    Particle p = particles[i];
    p.position += p.velocity * dt;
    if (abs(p.position.x) > 0.95)
    {
        p.velocity.x = -p.velocity.x;
        p.position.x = clamp(p.position.x, -0.95, 0.95);
    }
    if (abs(p.position.y) > 0.95)
    {
        p.velocity.y = -p.velocity.y;
        p.position.y = clamp(p.position.y, -0.95, 0.95);
    }
    particles[i] = p;
}
)";

constexpr const char* VERTEX = R"(#version 450 core
in vec2 position;
in vec4 color;
out vec4 vColor;

void main()
{
    vColor = color;
    gl_PointSize = 3.0;
    gl_Position = vec4(position, 0.0, 1.0);
}
)";

constexpr const char* FRAGMENT = R"(#version 450 core
in vec4 vColor;
out vec4 oColor;

void main()
{
    // Drop the corners of the square point, so it reads as a disc.
    vec2 fromCentre = gl_PointCoord - vec2(0.5);
    if (dot(fromCentre, fromCentre) > 0.25)
    {
        discard;
    }
    oColor = vColor;
}
)";

//------------------------------------------------------------------------------
std::string ComputeParticles::description() const
{
    return "Four thousand particles bounce in a box. A compute shader moves "
           "them inside a storage buffer, and the drawable reads that same "
           "buffer as its vertices: there is no copy. The barrier between the "
           "two is what makes the writes visible to the draw.";
}

//------------------------------------------------------------------------------
compages::Status ComputeParticles::setUp()
{
    // Positions, speeds and colours decided once. The compute shader only
    // moves them afterwards.
    std::vector<Particle> seed(COUNT);
    std::mt19937 rng(13u);
    std::uniform_real_distribution<float> place(-0.8f, 0.8f);
    std::uniform_real_distribution<float> speed(-0.4f, 0.4f);
    std::uniform_real_distribution<float> hue(0.0f, 1.0f);
    for (Particle& p : seed)
    {
        const float t = hue(rng);
        p.position = compages::core::Vector2f(place(rng), place(rng));
        p.velocity = compages::core::Vector2f(speed(rng), speed(rng));
        p.color = compages::core::Vector4f(
            0.4f + (0.6f * t), 0.5f + (0.4f * (1.0f - t)), 0.9f, 0.9f);
    }

    // Storage: a compute shader may write it, a draw may read it.
    COMPAGES_TRY_ASSIGN(m_particles,
                        compages::gpu::Buffer<Particle>::from(
                            seed,
                            compages::gpu::BufferKind::Storage,
                            compages::gpu::BufferUsage::Storage));

    COMPAGES_TRY(m_step.load(STEP));
    COMPAGES_TRY(m_step.bind("Particles", m_particles));
    m_step.set("count", COUNT);

    COMPAGES_TRY(m_points.load(VERTEX, FRAGMENT));
    m_points.vertices(m_particles);
    m_points.primitive(compages::gpu::Primitive::Points)
        .blend(compages::gpu::Blend::additive());
    return m_points.prepare();
}

//------------------------------------------------------------------------------
void ComputeParticles::draw(compages::world::ViewFrame const& p_frame)
{
    // Move them, then wait until those writes are visible as vertices.
    m_step.set("dt", p_frame.elapsed);
    if (!compages::gpu::check(m_step.dispatchItems(COUNT)))
    {
        return;
    }
    compages::gpu::barrier(compages::gpu::Barrier::VertexAttrib);

    compages::gpu::clear({ 0.02f, 0.02f, 0.04f });
    m_points.draw();
}

} // namespace examples
