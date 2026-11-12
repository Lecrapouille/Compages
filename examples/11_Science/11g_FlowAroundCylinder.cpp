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
static constexpr float HALF_X = 0.5f * SPACING * float(COLUMNS - 1);
static constexpr float HALF_Y = 0.5f * SPACING * float(ROWS - 1);
//! \brief The O-grid: 144 points round the column (and the first again, to
//! close the ring), 48 from the wall to the edge of the channel. 144 puts a
//! point on each corner of the channel (perimeter 18 m, corners at 1.5,
//! 7.5, 10.5 and 16.5 m from the middle of the right side).
static constexpr int AROUND = 144;
static constexpr int OUT = 48;
static constexpr float PI = std::numbers::pi_v<float>;

// ----------------------------------------------------------------------------
std::string FlowAroundCylinder::description() const
{
    return "A field of vectors: the wind around a column (ideal flow: a "
           "uniform wind, a doublet, and a vortex for the turn). "
           "scene.show(Field<Vector2f>, ArrowLook) draws arrows coloured by "
           "the speed; compages::streamline() follows the wind from the left "
           "edge; under them the pressure (Bernoulli: faster is lower). On a "
           "regular Grid2D the column is a staircase of square cells; on a "
           "CurvilinearGrid fitted round it (an O-grid: i round the column, j "
           "away from it, tight at the wall) the wall is a line of the grid. "
           "Try it: the grid, its lines, the speed, the turn.";
}

// ----------------------------------------------------------------------------
//! \brief The edge of the channel at a length \c p_s along it, from the
//! middle of its right side, anticlockwise.
// ----------------------------------------------------------------------------
static compages::Vector2f edgeAt(float p_s)
{
    const float w = 2.0f * HALF_X;
    const float h = 2.0f * HALF_Y;
    float s = std::fmod(p_s + HALF_Y, 2.0f * (w + h)); // from the bottom right corner
    if (s < h)
    {
        return { HALF_X, -HALF_Y + s };
    }
    s -= h;
    if (s < w)
    {
        return { HALF_X - s, HALF_Y };
    }
    s -= w;
    if (s < h)
    {
        return { -HALF_X, HALF_Y - s };
    }
    s -= h;
    return { -HALF_X + s, -HALF_Y };
}

// ----------------------------------------------------------------------------
//! \brief Point (i, j) of the O-grid: on the wall of the column for j = 0,
//! on the edge of the channel for the last j (equal steps along it), along
//! the straight line between. The steps grow away from the wall,
//! (exp(b u) - 1) / (exp(b) - 1): small where the wind turns fast.
// ----------------------------------------------------------------------------
static compages::Vector2f fitted(int p_i, int p_j)
{
    const float perimeter = 4.0f * (HALF_X + HALF_Y);
    const compages::Vector2f edge = edgeAt(perimeter * float(p_i % AROUND) / float(AROUND));
    const float far = std::hypot(edge.x, edge.y);
    const compages::Vector2f wall = edge * (RADIUS / far);
    constexpr float B = 3.0f;
    const float u = float(p_j) / float(OUT - 1);
    const float f = (std::exp(B * u) - 1.0f) / (std::exp(B) - 1.0f);
    return wall + ((edge - wall) * f);
}

// ----------------------------------------------------------------------------
//! In polar coordinates around the column, a uniform wind U plus a doublet
//! and a vortex of strength G: u_r = U (1 - R^2/r^2) cos t,
//! u_t = -U (1 + R^2/r^2) sin t + G / (2 pi r). The pressure coefficient is
//! 1 - (|v| / U)^2. The same formula fills a field on any grid.
// ----------------------------------------------------------------------------
void FlowAroundCylinder::blow()
{
    const float circulation = m_turn * 2.0f * PI * RADIUS * m_speed;
    auto fill = [&](auto& p_wind, auto& p_pressure) {
        for (std::size_t k = 0u; k < p_wind.size(); ++k)
        {
            const compages::Vector2f p = p_wind.grid().point(k);
            const float r = std::hypot(p.x, p.y);
            if (r < RADIUS - 1.0e-4f)
            {
                p_wind[k] = compages::Vector2f(0.0f, 0.0f);
                p_pressure[k] = 1.0f;
                continue;
            }
            const float t = std::atan2(p.y, p.x);
            const float ratio = (RADIUS * RADIUS) / (r * r);
            const float ur = m_speed * (1.0f - ratio) * std::cos(t);
            const float ut = (-m_speed * (1.0f + ratio) * std::sin(t)) + (circulation / (2.0f * PI * r));
            const compages::Vector2f v((ur * std::cos(t)) - (ut * std::sin(t)), (ur * std::sin(t)) + (ut * std::cos(t)));
            p_wind[k] = v;
            const float relative = std::hypot(v.x, v.y) / std::max(m_speed, 1.0e-3f);
            p_pressure[k] = 1.0f - (relative * relative);
        }
    };
    fill(m_wind, m_pressure);
    fill(m_fitted_wind, m_fitted_pressure);
    m_lines.clear();
    for (int n = 0; n < 15; ++n)
    {
        const compages::Vector2f start(-HALF_X + 0.02f, -HALF_Y + 0.1f + (float(n) * 0.2f));
        m_lines.push_back((m_grid == 0) ? compages::streamline(m_wind, start, 0.04f, 400)
                                        : compages::streamline(m_fitted_wind, start, 0.04f, 400));
    }
}

// ----------------------------------------------------------------------------
compages::Status FlowAroundCylinder::setUp()
{
    const compages::Grid2D grid{ .columns = COLUMNS,
                                 .rows = ROWS,
                                 .origin = { -HALF_X, -HALF_Y },
                                 .spacing = SPACING };
    m_wind = compages::Field<compages::Vector2f>(grid, compages::Vector2f(0.0f, 0.0f));
    m_pressure = compages::Field<float>(grid, 0.0f);
    compages::CurvilinearGrid ogrid;
    COMPAGES_TRY_ASSIGN(ogrid, compages::CurvilinearGrid::create(AROUND + 1, OUT, fitted));
    m_fitted_wind = compages::Field<compages::Vector2f, compages::CurvilinearGrid>(ogrid, compages::Vector2f(0.0f, 0.0f));
    m_fitted_pressure = compages::Field<float, compages::CurvilinearGrid>(ogrid, 0.0f);
    blow();

    m_scene.background(0.08f, 0.09f, 0.11f);
    m_scene.camera().position(0.0f, 4.5f, 3.2f).add<compages::world::Orbit>();
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.2f);
    m_floor = m_scene.show(m_pressure, m_pressure_look, "Pressure");
    m_arrows = m_scene.show(m_wind, m_arrows_look, "Wind");
    m_arrows.position(0.0f, 0.01f, 0.0f);
    m_fitted_floor = m_scene.show(m_fitted_pressure, m_pressure_look, "Pressure, fitted grid");
    m_fitted_arrows = m_scene.show(m_fitted_wind, m_arrows_look, "Wind, fitted grid");
    m_fitted_arrows.position(0.0f, 0.01f, 0.0f);
    m_scene.cylinder("Column", compages::renderer::color(0.7f, 0.7f, 0.72f))
        .scale(2.0f * RADIUS, 1.0f, 2.0f * RADIUS)
        .position(0.0f, 0.5f, 0.0f);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void FlowAroundCylinder::drawGridLines()
{
    const compages::Vector3f ink(0.3f, 0.32f, 0.38f);
    auto at = [](compages::Vector2f p_p) { return compages::Vector3f(p_p.x, 0.02f, p_p.y); };
    if (m_grid == 0)
    {
        compages::Grid2D const& grid = m_wind.grid();
        for (int i = 0; i < grid.columns; i += 2)
        {
            m_scene.debug().line(at(grid.point(i, 0)), at(grid.point(i, grid.rows - 1)), ink);
        }
        for (int j = 0; j < grid.rows; j += 2)
        {
            m_scene.debug().line(at(grid.point(0, j)), at(grid.point(grid.columns - 1, j)), ink);
        }
        return;
    }
    compages::CurvilinearGrid const& grid = m_fitted_wind.grid();
    for (int j = 0; j < grid.rows; ++j)
    {
        for (int i = 0; i + 1 < grid.columns; ++i)
        {
            m_scene.debug().line(at(grid.point(i, j)), at(grid.point(i + 1, j)), ink);
        }
    }
    for (int i = 0; i < grid.columns; i += 2)
    {
        for (int j = 0; j + 1 < grid.rows; ++j)
        {
            m_scene.debug().line(at(grid.point(i, j)), at(grid.point(i, j + 1)), ink);
        }
    }
}

// ----------------------------------------------------------------------------
void FlowAroundCylinder::draw(compages::world::ViewFrame const& p_frame)
{
    const bool fitted = (m_grid == 1);
    compages::Status shown = fitted ? m_scene.show(m_fitted_floor, m_fitted_pressure, m_pressure_look)
                                    : m_scene.show(m_floor, m_pressure, m_pressure_look);
    if (shown)
    {
        shown = fitted ? m_scene.show(m_fitted_arrows, m_fitted_wind, m_arrows_look)
                       : m_scene.show(m_arrows, m_wind, m_arrows_look);
    }
    if (!shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_floor.enable(!fitted);
    m_fitted_floor.enable(fitted);
    m_arrows.enable(m_show_arrows && !fitted);
    m_fitted_arrows.enable(m_show_arrows && fitted);
    if (m_show_grid)
    {
        drawGridLines();
    }
    if (m_show_lines)
    {
        for (std::vector<compages::Vector2f> const& line : m_lines)
        {
            for (std::size_t k = 1u; k < line.size(); ++k)
            {
                m_scene.debug().line(compages::Vector3f(line[k - 1u].x, 0.025f, line[k - 1u].y),
                                     compages::Vector3f(line[k].x, 0.025f, line[k].y),
                                     compages::Vector3f(0.1f, 0.1f, 0.15f));
            }
        }
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void FlowAroundCylinder::controls()
{
    bool changed = ImGui::Combo("Grid", &m_grid, "Regular (Grid2D)\0Fitted round the column (CurvilinearGrid)\0");
    changed = ImGui::SliderFloat("Wind (m/s)", &m_speed, 0.1f, 2.0f, "%.2f") || changed;
    changed = ImGui::SliderFloat("Turn of the column", &m_turn, -1.5f, 1.5f, "%.2f") || changed;
    if (changed)
    {
        m_arrows_look.high = 2.0f * m_speed * (1.0f + std::abs(m_turn));
        blow();
    }
    ImGui::Checkbox("Arrows", &m_show_arrows);
    ImGui::Checkbox("Streamlines", &m_show_lines);
    ImGui::Checkbox("Grid lines", &m_show_grid);
}

} // namespace examples
