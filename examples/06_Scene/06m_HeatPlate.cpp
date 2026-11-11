// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06m_HeatPlate.hpp"

#include "Common/Gui.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>

namespace examples
{

//! \brief A plate of copper 2 m wide, 96 x 96 points.
static constexpr int POINTS = 96;
static constexpr float WIDTH = 2.0f;
//! \brief How fast heat spreads in copper, in m^2/s.
static constexpr float DIFFUSIVITY = 1.1e-4f;
//! \brief The air around (the edges are kept at it) and the heaters.
static constexpr float ROOM = 20.0f;
static constexpr float HEATER = 300.0f;
static constexpr float HEATER_RADIUS = 0.12f;

// ----------------------------------------------------------------------------
std::string HeatPlate::description() const
{
    return "A field shown in one call. The temperature of a copper plate is a "
           "Field<float> on a Grid2D: the CPU solves the heat equation on it "
           "(each point moves toward the mean of its four neighbours), two "
           "heaters wander on it, the edges stay at room temperature. "
           "scene.show(entity, field, look) sends the new values each frame; "
           "the GPU turns them into colours (Inferno, 0 to 300 degrees) and "
           "into a relief. Try it: the colour map, the relief, the light, the "
           "speed of time.";
}

// ----------------------------------------------------------------------------
compages::Status HeatPlate::setUp()
{
    const float spacing = WIDTH / float(POINTS - 1);
    const compages::Grid2D grid{ .columns = POINTS,
                                 .rows = POINTS,
                                 .origin = { -0.5f * WIDTH, -0.5f * WIDTH },
                                 .spacing = spacing };
    m_temperature = compages::Field<float>(grid, ROOM);
    m_next = m_temperature;

    m_scene.background(0.55f, 0.6f, 0.66f);
    m_scene.camera().position(0.0f, 2.0f, 2.4f).add<compages::world::Orbit>();
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.2f);
    // A few minutes of heating first, so the heat has spread when it shows.
    m_time = 240.0f;
    spread(m_time, m_time);
    m_plate = m_scene.show(m_temperature, m_look, "Plate");
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! Explicit steps, each short enough to stay stable (dt < h^2 / 4D): the new
//! value of a point is its value plus D dt / h^2 times the Laplacian of its
//! four neighbours.
// ----------------------------------------------------------------------------
void HeatPlate::spread(float p_seconds, float p_time)
{
    compages::Grid2D const& grid = m_temperature.grid();
    const float h2 = grid.spacing * grid.spacing;
    const float limit = 0.2f * h2 / DIFFUSIVITY;
    const int steps = std::max(1, int(std::ceil(p_seconds / limit)));
    const float rate = DIFFUSIVITY * (p_seconds / float(steps)) / h2;

    // Two heaters on slow loops.
    const compages::Vector2f heaters[2] = {
        { 0.55f * std::cos(0.004f * p_time), 0.45f * std::sin(0.006f * p_time) },
        { -0.5f * std::cos(0.003f * p_time), -0.5f * std::sin(0.005f * p_time + 1.0f) }
    };
    for (int step = 0; step < steps; ++step)
    {
        for (int j = 0; j < grid.rows; ++j)
        {
            for (int i = 0; i < grid.columns; ++i)
            {
                const compages::Vector2f p = grid.point(i, j);
                const bool edge = (i == 0) || (j == 0) || (i == grid.columns - 1) ||
                                  (j == grid.rows - 1);
                const bool heated = std::any_of(
                    std::begin(heaters), std::end(heaters),
                    [&p](compages::Vector2f p_heater)
                    { return std::hypot(p.x - p_heater.x, p.y - p_heater.y) < HEATER_RADIUS; });
                const float t = m_temperature(i, j);
                m_next(i, j) = edge ? ROOM
                               : heated
                                   ? HEATER
                                   : t + (rate * (m_temperature(i - 1, j) + m_temperature(i + 1, j) +
                                                  m_temperature(i, j - 1) + m_temperature(i, j + 1) -
                                                  (4.0f * t)));
            }
        }
        std::swap(m_temperature, m_next);
    }
}

// ----------------------------------------------------------------------------
void HeatPlate::draw(compages::world::ViewFrame const& p_frame)
{
    const float seconds = m_speed * std::min(p_frame.elapsed, 0.05f);
    m_time += seconds;
    spread(seconds, m_time);
    if (const compages::Status shown = m_scene.show(m_plate, m_temperature, m_look); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void HeatPlate::controls()
{
    int colors = static_cast<int>(m_look.colors);
    if (ImGui::Combo("Colours", &colors, "Viridis\0Inferno\0Coolwarm\0Terrain\0Grey\0"))
    {
        m_look.colors = static_cast<compages::renderer::ColorMap>(colors);
    }
    ImGui::SliderFloat("Relief (m per degree)", &m_look.relief, 0.0f, 0.004f, "%.4f");
    ImGui::Checkbox("Lit", &m_look.lit);
    ImGui::SliderFloat("Time (s per s)", &m_speed, 0.0f, 1000.0f, "%.0f");
}

} // namespace examples
