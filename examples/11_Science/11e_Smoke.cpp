// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "11_Science/11e_Smoke.hpp"

#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/ShaderSource.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <numbers>
#include <vector>

namespace examples
{

//! \brief 128 x 192 cells, 2 m wide; distances in the shaders are in cells.
static constexpr int COLUMNS = 128;
static constexpr int ROWS = 192;
static constexpr float WIDTH = 2.0f;

// ----------------------------------------------------------------------------
//! One text, six kernels: STEP chooses which one is compiled, and only the
//! blocks it reads or writes are declared (an unread block would be removed
//! by the compiler, and bind() would not find it).
//!   1: move the speeds along themselves, add the lift of the hot smoke;
//!   2: the divergence of the speeds;
//!   3: one Jacobi step of the pressure (laplacian of p = divergence);
//!   4: remove the gradient of the pressure: the speeds lose their divergence;
//!   5: move the smoke along the speeds, add the source, let it fade and
//!      leave by the top.
// ----------------------------------------------------------------------------
static constexpr char const* SMOKE = R"(#version 450 core
layout(local_size_x = 64) in;

uniform int columns;
uniform int rows;
uniform float dt;

#if (STEP == 1) || (STEP == 2) || (STEP == 4) || (STEP == 5)
layout(std430, binding = 0) readonly buffer VelocityIn { vec2 velocityIn[]; };
#endif
#if (STEP == 1) || (STEP == 4)
layout(std430, binding = 1) writeonly buffer VelocityOut { vec2 velocityOut[]; };
#endif
#if (STEP == 1) || (STEP == 3) || (STEP == 4) || (STEP == 5)
layout(std430, binding = 2) readonly buffer ScalarIn { float scalarIn[]; };
#endif
#if (STEP == 2) || (STEP == 3) || (STEP == 5)
layout(std430, binding = 3) writeonly buffer ScalarOut { float scalarOut[]; };
#endif
#if (STEP == 3)
layout(std430, binding = 4) readonly buffer Divergence { float divergence[]; };
#endif

uniform float buoyancy;
uniform float wind;
uniform float time;

int at(int i, int j)
{
    return clamp(j, 0, rows - 1) * columns + clamp(i, 0, columns - 1);
}

bool wall(int i, int j)
{
    return (i <= 0) || (j <= 0) || (i >= columns - 1) || (j >= rows - 1);
}

// Where the smoke comes from: a burner at the bottom, swaying a little.
float source(int i, int j)
{
    float centre = 0.5 * float(columns) + 6.0 * sin(0.7 * time);
    return (j < 12 && abs(float(i) - centre) < 9.0) ? 1.0 : 0.0;
}

#if (STEP == 1) || (STEP == 5)
vec2 velocityAt(vec2 p)
{
    p = clamp(p, vec2(0.0), vec2(columns - 1, rows - 1));
    ivec2 c = min(ivec2(floor(p)), ivec2(columns - 2, rows - 2));
    vec2 f = p - vec2(c);
    return mix(mix(velocityIn[at(c.x, c.y)], velocityIn[at(c.x + 1, c.y)], f.x),
               mix(velocityIn[at(c.x, c.y + 1)], velocityIn[at(c.x + 1, c.y + 1)], f.x), f.y);
}
#endif

#if (STEP == 5)
float scalarAt(vec2 p)
{
    p = clamp(p, vec2(0.0), vec2(columns - 1, rows - 1));
    ivec2 c = min(ivec2(floor(p)), ivec2(columns - 2, rows - 2));
    vec2 f = p - vec2(c);
    return mix(mix(scalarIn[at(c.x, c.y)], scalarIn[at(c.x + 1, c.y)], f.x),
               mix(scalarIn[at(c.x, c.y + 1)], scalarIn[at(c.x + 1, c.y + 1)], f.x), f.y);
}
#endif

void main()
{
    int k = int(gl_GlobalInvocationID.x);
    if (k >= columns * rows)
    {
        return;
    }
    int i = k % columns;
    int j = k / columns;
#if STEP == 1
    vec2 back = vec2(i, j) - dt * velocityIn[k];
    vec2 v = velocityAt(back);
    v += dt * vec2(wind, buoyancy * scalarIn[k]);
    if (source(i, j) > 0.0)
    {
        v = vec2(0.0, 6.0);
    }
    velocityOut[k] = wall(i, j) ? vec2(0.0) : v;
#elif STEP == 2
    scalarOut[k] = 0.5 * (velocityIn[at(i + 1, j)].x - velocityIn[at(i - 1, j)].x +
                          velocityIn[at(i, j + 1)].y - velocityIn[at(i, j - 1)].y);
#elif STEP == 3
    scalarOut[k] = 0.25 * (scalarIn[at(i - 1, j)] + scalarIn[at(i + 1, j)] +
                           scalarIn[at(i, j - 1)] + scalarIn[at(i, j + 1)] - divergence[k]);
#elif STEP == 4
    vec2 gradient = 0.5 * vec2(scalarIn[at(i + 1, j)] - scalarIn[at(i - 1, j)],
                               scalarIn[at(i, j + 1)] - scalarIn[at(i, j - 1)]);
    velocityOut[k] = wall(i, j) ? vec2(0.0) : velocityIn[k] - gradient;
#elif STEP == 5
    vec2 back = vec2(i, j) - dt * velocityIn[k];
    float smoke = scalarAt(back) * (1.0 - 0.02 * dt);   // it thins out
    // The top is open: the smoke leaves there.
    scalarOut[k] = (j >= rows - 3) ? 0.0 : max(smoke, source(i, j));
#endif
}
)";

// ----------------------------------------------------------------------------
std::string Smoke::description() const
{
    return "Smoke rising from a burner: a fluid in 2D, every step on the GPU "
           "(Stam, Stable Fluids). Each frame: the speeds move along "
           "themselves and the hot smoke lifts them, the pressure is found by "
           "Jacobi steps so that no air is created or lost, its gradient is "
           "removed from the speeds, the smoke moves along them. Five compute "
           "shaders come from one GLSL text, chosen by "
           "ShaderSource::define(\"STEP\", ...). scene.show(grid, buffer, look) "
           "draws the smoke, standing up. Try it: lift, wind, Jacobi steps.";
}

// ----------------------------------------------------------------------------
compages::Status Smoke::setUp()
{
    m_grid = compages::Grid2D{ .columns = COLUMNS,
                               .rows = ROWS,
                               .origin = { -0.5f * WIDTH, 0.0f },
                               .spacing = WIDTH / float(COLUMNS - 1) };
    const std::size_t count = m_grid.count();
    for (std::size_t n = 0u; n < 2u; ++n)
    {
        COMPAGES_TRY_ASSIGN(m_velocity[n],
                            compages::gpu::StorageBuffer<compages::Vector2f>::from(
                                std::vector(count, compages::Vector2f(0.0f, 0.0f)),
                                compages::gpu::BufferUsage::Stream));
        COMPAGES_TRY_ASSIGN(m_smoke[n],
                            compages::gpu::StorageBuffer<float>::from(
                                std::vector(count, 0.0f), compages::gpu::BufferUsage::Stream));
        COMPAGES_TRY_ASSIGN(m_pressure[n],
                            compages::gpu::StorageBuffer<float>::from(
                                std::vector(count, 0.0f), compages::gpu::BufferUsage::Stream));
    }
    COMPAGES_TRY_ASSIGN(m_divergence_values,
                        compages::gpu::StorageBuffer<float>::from(
                            std::vector(count, 0.0f), compages::gpu::BufferUsage::Stream));

    compages::gpu::ComputeProgram* const kernels[5] = {
        &m_advect_velocity, &m_divergence, &m_jacobi, &m_project, &m_advect_smoke
    };
    for (int n = 0; n < 5; ++n)
    {
        COMPAGES_TRY(kernels[n]->load(
            compages::gpu::ShaderSource::fromText("smoke.comp", SMOKE).define("STEP", std::to_string(n + 1))));
        kernels[n]->set("columns", COLUMNS);
        kernels[n]->set("rows", ROWS);
    }

    m_scene.background(0.08f, 0.09f, 0.11f);
    m_scene.camera().position(0.0f, 1.5f, 3.4f).add<compages::world::Orbit>(
        compages::Vector3f(0.0f, 1.5f, 0.0f));
    // The field lies on the ground of its grid: stand it up, rows going up.
    m_view = m_scene.show(m_grid, m_smoke[m_current], m_look, "Smoke");
    m_view.rotation(-0.5f * std::numbers::pi_v<float>, { 1.0f, 0.0f, 0.0f });

    // A few seconds of smoke before the first picture.
    for (int k = 0; k < 120; ++k)
    {
        COMPAGES_TRY(step(1.0f / 30.0f));
    }
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! The order of Stable Fluids. Each kernel waits for the one before (a
//! barrier: what a kernel wrote is read by the next).
// ----------------------------------------------------------------------------
compages::Status Smoke::step(float p_dt)
{
    m_time += p_dt;
    const auto items = std::uint32_t(m_grid.count());
    // In cells per second: the solver works in cells.
    const float dt = p_dt * 30.0f;
    auto run = [items](compages::gpu::ComputeProgram& p_kernel) -> compages::Status
    {
        COMPAGES_TRY(p_kernel.dispatchItems(items));
        compages::gpu::barrier(compages::gpu::Barrier::Storage);
        return compages::success();
    };
    const std::size_t a = m_current;
    const std::size_t b = 1u - m_current;

    // 1. The speeds move along themselves; the smoke lifts them: v[a] -> v[b].
    m_advect_velocity.set("dt", dt);
    m_advect_velocity.set("time", m_time);
    m_advect_velocity.set("buoyancy", m_buoyancy * 0.02f);
    m_advect_velocity.set("wind", m_wind * 0.02f);
    COMPAGES_TRY(m_advect_velocity.bind("VelocityIn", m_velocity[a]));
    COMPAGES_TRY(m_advect_velocity.bind("VelocityOut", m_velocity[b]));
    COMPAGES_TRY(m_advect_velocity.bind("ScalarIn", m_smoke[a]));
    COMPAGES_TRY(run(m_advect_velocity));

    // 2. Their divergence: where air would be created or lost.
    COMPAGES_TRY(m_divergence.bind("VelocityIn", m_velocity[b]));
    COMPAGES_TRY(m_divergence.bind("ScalarOut", m_divergence_values));
    COMPAGES_TRY(run(m_divergence));

    // 3. The pressure that cancels it, by Jacobi steps (pressure ping-pong).
    COMPAGES_TRY(m_jacobi.bind("Divergence", m_divergence_values));
    std::size_t p = 0u;
    for (int n = 0; n < m_iterations; ++n)
    {
        COMPAGES_TRY(m_jacobi.bind("ScalarIn", m_pressure[p]));
        COMPAGES_TRY(m_jacobi.bind("ScalarOut", m_pressure[1u - p]));
        COMPAGES_TRY(run(m_jacobi));
        p = 1u - p;
    }

    // 4. Its gradient leaves the speeds: v[b] -> v[a].
    COMPAGES_TRY(m_project.bind("VelocityIn", m_velocity[b]));
    COMPAGES_TRY(m_project.bind("VelocityOut", m_velocity[a]));
    COMPAGES_TRY(m_project.bind("ScalarIn", m_pressure[p]));
    COMPAGES_TRY(run(m_project));

    // 5. The smoke moves along them: smoke[a] -> smoke[b].
    m_advect_smoke.set("dt", dt);
    m_advect_smoke.set("time", m_time);
    COMPAGES_TRY(m_advect_smoke.bind("VelocityIn", m_velocity[a]));
    COMPAGES_TRY(m_advect_smoke.bind("ScalarIn", m_smoke[a]));
    COMPAGES_TRY(m_advect_smoke.bind("ScalarOut", m_smoke[b]));
    COMPAGES_TRY(run(m_advect_smoke));

    // The speeds are back in v[a]; the smoke is now in smoke[b].
    std::swap(m_smoke[a], m_smoke[b]);
    return compages::success();
}

// ----------------------------------------------------------------------------
void Smoke::draw(compages::world::ViewFrame const& p_frame)
{
    if (const compages::Status stepped = step(std::min(p_frame.elapsed, 1.0f / 30.0f)); !stepped)
    {
        compages::gpu::reportError(stepped.error());
    }
    if (const compages::Status shown = m_scene.show(m_view, m_smoke[m_current], m_look); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Smoke::controls()
{
    ImGui::SliderFloat("Lift of the hot smoke", &m_buoyancy, 0.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Wind", &m_wind, -10.0f, 10.0f, "%.1f");
    ImGui::SliderInt("Jacobi steps", &m_iterations, 1, 80);
    int colors = static_cast<int>(m_look.colors);
    if (ImGui::Combo("Colours", &colors, "Viridis\0Inferno\0Coolwarm\0Terrain\0Grey\0"))
    {
        m_look.colors = static_cast<compages::renderer::ColorMap>(colors);
    }
}

} // namespace examples
