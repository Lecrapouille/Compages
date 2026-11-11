// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "11_Science/11g_FlowAroundCylinder.hpp"

#include "Common/Gui.hpp"
#include "Compages/Fields/Streamline.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <cmath>
#include <numbers>

namespace examples
{

//! \brief A channel 6 x 3 m, 121 x 61 points; the column, 0.5 m of radius.
static constexpr int COLUMNS = 121;
static constexpr int ROWS = 61;
static constexpr float SPACING = 0.05f;
static constexpr float RADIUS = 0.5f;

// ----------------------------------------------------------------------------
std::string FlowAroundCylinder::description() const
{
    return "A field of vectors: the wind around a column (ideal flow: a "
           "uniform wind, a doublet, and a vortex for the turn). "
           "scene.show(Field<Vector2f>, ArrowLook) draws arrows coloured by "
           "the speed; compages::streamline() follows the wind from the left "
           "edge; under them the pressure (Bernoulli: faster is lower), a "
           "Field<float> in Coolwarm. With a turn the wind is faster on one "
           "side: the column is pushed sideways (the Magnus effect). Try it: "
           "the speed, the turn, what is shown.";
}

// ----------------------------------------------------------------------------
//! In polar coordinates around the column, a uniform wind U plus a doublet
//! and a vortex of strength G: u_r = U (1 - R^2/r^2) cos t,
//! u_t = -U (1 + R^2/r^2) sin t + G / (2 pi r). The pressure coefficient is
//! 1 - (|v| / U)^2.
// ----------------------------------------------------------------------------
void FlowAroundCylinder::blow()
{
    compages::Grid2D const& grid = m_wind.grid();
    const float circulation = m_turn * 2.0f * std::numbers::pi_v<float> * RADIUS * m_speed;
    for (std::size_t k = 0u; k < m_wind.size(); ++k)
    {
        const compages::Vector2f p = grid.point(k);
        const float r = std::hypot(p.x, p.y);
        if (r < RADIUS)
        {
            m_wind[k] = compages::Vector2f(0.0f, 0.0f);
            m_pressure[k] = 1.0f;
            continue;
        }
        const float t = std::atan2(p.y, p.x);
        const float ratio = (RADIUS * RADIUS) / (r * r);
        const float ur = m_speed * (1.0f - ratio) * std::cos(t);
        const float ut = (-m_speed * (1.0f + ratio) * std::sin(t)) +
                         (circulation / (2.0f * std::numbers::pi_v<float> * r));
        const compages::Vector2f v((ur * std::cos(t)) - (ut * std::sin(t)),
                                   (ur * std::sin(t)) + (ut * std::cos(t)));
        m_wind[k] = v;
        const float relative = std::hypot(v.x, v.y) / std::max(m_speed, 1.0e-3f);
        m_pressure[k] = 1.0f - (relative * relative);
    }
    m_lines.clear();
    for (int n = 0; n < 15; ++n)
    {
        const compages::Vector2f start = grid.point(0, 2 + (n * 4));
        m_lines.push_back(compages::streamline(m_wind, start, 0.04f, 400));
    }
}

// ----------------------------------------------------------------------------
compages::Status FlowAroundCylinder::setUp()
{
    const compages::Grid2D grid{ .columns = COLUMNS,
                                 .rows = ROWS,
                                 .origin = { -0.5f * SPACING * float(COLUMNS - 1),
                                             -0.5f * SPACING * float(ROWS - 1) },
                                 .spacing = SPACING };
    m_wind = compages::Field<compages::Vector2f>(grid, compages::Vector2f(0.0f, 0.0f));
    m_pressure = compages::Field<float>(grid, 0.0f);
    blow();

    m_scene.background(0.08f, 0.09f, 0.11f);
    m_scene.camera().position(0.0f, 4.5f, 3.2f).add<compages::world::Orbit>();
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.2f);
    m_floor = m_scene.show(m_pressure, m_pressure_look, "Pressure");
    m_arrows = m_scene.show(m_wind, m_arrows_look, "Wind");
    m_arrows.position(0.0f, 0.01f, 0.0f);
    m_scene.cylinder("Column", compages::renderer::color(0.7f, 0.7f, 0.72f))
        .scale(2.0f * RADIUS, 1.0f, 2.0f * RADIUS)
        .position(0.0f, 0.5f, 0.0f);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void FlowAroundCylinder::draw(compages::world::ViewFrame const& p_frame)
{
    compages::Status shown = m_scene.show(m_floor, m_pressure, m_pressure_look);
    if (shown)
    {
        shown = m_scene.show(m_arrows, m_wind, m_arrows_look);
    }
    if (!shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_arrows.enable(m_show_arrows);
    if (m_show_lines)
    {
        for (std::vector<compages::Vector2f> const& line : m_lines)
        {
            for (std::size_t k = 1u; k < line.size(); ++k)
            {
                m_scene.debug().line(compages::Vector3f(line[k - 1u].x, 0.02f, line[k - 1u].y),
                                     compages::Vector3f(line[k].x, 0.02f, line[k].y),
                                     compages::Vector3f(0.1f, 0.1f, 0.15f));
            }
        }
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void FlowAroundCylinder::controls()
{
    bool changed = ImGui::SliderFloat("Wind (m/s)", &m_speed, 0.1f, 2.0f, "%.2f");
    changed = ImGui::SliderFloat("Turn of the column", &m_turn, -1.5f, 1.5f, "%.2f") || changed;
    if (changed)
    {
        m_arrows_look.high = 2.0f * m_speed * (1.0f + std::abs(m_turn));
        blow();
    }
    ImGui::Checkbox("Arrows", &m_show_arrows);
    ImGui::Checkbox("Streamlines", &m_show_lines);
}

} // namespace examples
