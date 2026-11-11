// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "11_Science/11b_RippleTank.hpp"

#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <vector>

namespace examples
{

//! \brief A tank 4 m wide, 160 x 160 points.
static constexpr int POINTS = 160;
static constexpr float WIDTH = 4.0f;
//! \brief Steps of the waves per frame.
static constexpr int STEPS = 4;

// ----------------------------------------------------------------------------
//! The wave equation, one step: next = 2 now - before + c^2 Laplacian(now),
//! a little damped; the walls hold the water at rest. Three blocks with their
//! own bindings, so that the three buffers can be given in any order.
// ----------------------------------------------------------------------------
static constexpr char const* WAVES = R"(#version 450 core
layout(local_size_x = 64) in;
layout(std430, binding = 0) readonly buffer Before { float before[]; };
layout(std430, binding = 1) readonly buffer Now { float now[]; };
layout(std430, binding = 2) writeonly buffer Next { float next[]; };

uniform int columns;
uniform int rows;
uniform float courant2;   // (c dt / h)^2, under 0.5 to stay stable
uniform float damping;
uniform vec3 drop;        // where (in points), how high (0: no drop)

void main()
{
    int k = int(gl_GlobalInvocationID.x);
    if (k >= columns * rows)
    {
        return;
    }
    int i = k % columns;
    int j = k / columns;
    if ((i == 0) || (j == 0) || (i == columns - 1) || (j == rows - 1))
    {
        next[k] = 0.0;
        return;
    }
    float laplacian = now[k - 1] + now[k + 1] + now[k - columns] + now[k + columns] - 4.0 * now[k];
    float height = (2.0 * now[k] - before[k] + courant2 * laplacian) * damping;
    float d = distance(vec2(i, j), drop.xy);
    height += drop.z * max(0.0, 1.0 - d / 3.0);
    next[k] = height;
}
)";

// ----------------------------------------------------------------------------
std::string RippleTank::description() const
{
    return "A field computed and drawn on the GPU. A compute shader steps the "
           "wave equation on three storage buffers (the surface before, now "
           "and next) and drops of rain fall in. scene.show(grid, buffer, look) "
           "draws the buffer of now as it is: colours (Coolwarm, below and "
           "above rest) and a relief. The CPU never reads a height. Try it: "
           "rain, damping, colours, relief.";
}

// ----------------------------------------------------------------------------
compages::Status RippleTank::setUp()
{
    const float spacing = WIDTH / float(POINTS - 1);
    m_grid = compages::Grid2D{ .columns = POINTS,
                               .rows = POINTS,
                               .origin = { -0.5f * WIDTH, -0.5f * WIDTH },
                               .spacing = spacing };
    const std::vector<float> rest(m_grid.count(), 0.0f);
    for (compages::gpu::StorageBuffer<float>& heights : m_heights)
    {
        COMPAGES_TRY_ASSIGN(heights,
                            compages::gpu::StorageBuffer<float>::from(
                                rest, compages::gpu::BufferUsage::Stream));
    }
    COMPAGES_TRY(m_waves.loadSource(WAVES));
    m_waves.set("columns", POINTS);
    m_waves.set("rows", POINTS);
    m_waves.set("courant2", 0.3f);

    m_scene.background(0.55f, 0.6f, 0.66f);
    m_scene.camera().position(0.0f, 3.0f, 4.0f).add<compages::world::Orbit>();
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.3f);
    m_surface = m_scene.show(m_grid, m_heights[m_now], m_look, "Water");

    // A few drops before the first picture.
    for (int k = 0; k < 120; ++k)
    {
        COMPAGES_TRY(step((k % 40 == 0) ? compages::Vector3f(float(40 + k / 2), float(60 + k / 3), 1.0f)
                                        : compages::Vector3f(0.0f, 0.0f, 0.0f)));
    }
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! The buffers turn: before <- now <- next. Each step waits for the one
//! before (a barrier), and the draw waits for the last.
// ----------------------------------------------------------------------------
compages::Status RippleTank::step(compages::Vector3f p_drop)
{
    const std::size_t before = (m_now + 2u) % 3u;
    const std::size_t next = (m_now + 1u) % 3u;
    COMPAGES_TRY(m_waves.bind("Before", m_heights[before]));
    COMPAGES_TRY(m_waves.bind("Now", m_heights[m_now]));
    COMPAGES_TRY(m_waves.bind("Next", m_heights[next]));
    m_waves.set("damping", m_damping);
    m_waves.set("drop", p_drop);
    COMPAGES_TRY(m_waves.dispatchItems(std::uint32_t(m_grid.count())));
    compages::gpu::barrier(compages::gpu::Barrier::Storage);
    m_now = next;
    return compages::success();
}

// ----------------------------------------------------------------------------
void RippleTank::draw(compages::world::ViewFrame const& p_frame)
{
    std::uniform_real_distribution<float> place(10.0f, float(POINTS - 10));
    m_next_drop -= p_frame.elapsed;
    for (int k = 0; k < STEPS; ++k)
    {
        compages::Vector3f drop(0.0f, 0.0f, 0.0f);
        if ((k == 0) && (m_rain > 0.0f) && (m_next_drop <= 0.0f))
        {
            drop = compages::Vector3f(place(m_random), place(m_random), 0.8f);
            m_next_drop = 1.0f / m_rain;
        }
        if (const compages::Status stepped = step(drop); !stepped)
        {
            compages::gpu::reportError(stepped.error());
        }
    }
    if (const compages::Status shown = m_scene.show(m_surface, m_heights[m_now], m_look); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void RippleTank::controls()
{
    ImGui::SliderFloat("Rain (drops per s)", &m_rain, 0.0f, 20.0f, "%.1f");
    ImGui::SliderFloat("Damping", &m_damping, 0.98f, 1.0f, "%.4f");
    int colors = static_cast<int>(m_look.colors);
    if (ImGui::Combo("Colours", &colors, "Viridis\0Inferno\0Coolwarm\0Terrain\0Grey\0"))
    {
        m_look.colors = static_cast<compages::renderer::ColorMap>(colors);
    }
    ImGui::SliderFloat("Relief (m per unit)", &m_look.relief, 0.0f, 0.5f, "%.2f");
    ImGui::Checkbox("Lit", &m_look.lit);
}

} // namespace examples
