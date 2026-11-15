// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Workshop.hpp"

#include <algorithm>
#include <cmath>

namespace examples::workshop
{

namespace
{

//! \brief The air, and the hot tip of the tool, in degrees.
constexpr float ROOM = 20.0f;
constexpr float TIP = 300.0f;
//! \brief How far round the tip it heats, in metres.
constexpr float TIP_RADIUS = 0.03f;
//! \brief How fast heat spreads, in m^2/s (ten times steel, to see it).
constexpr float DIFFUSIVITY = 1.0e-4f;
//! \brief How fast the plate cools toward the air, per second.
constexpr float COOLING = 0.08f;

} // namespace

// ----------------------------------------------------------------------------
//! A steel plate of 0.6 m by 0.9 m on the table, under the tool: one value
//! every 1.5 cm. The grid lies on the ground (x, z); its entity is lifted to
//! the top of the table.
// ----------------------------------------------------------------------------
void HotPlate::create(compages::renderer::Scene& p_scene)
{
    const compages::Grid2D grid{ .columns = 41, .rows = 61, .origin = { -0.25f, -0.45f }, .spacing = 0.015f };
    m_temperature = compages::Field<float>(grid, ROOM);
    m_next = m_temperature;
    m_shown = p_scene.show(m_temperature, m_look, "Plate");
    m_shown.position(0.0f, TABLE_TOP + 0.003f, 0.0f);
}

// ----------------------------------------------------------------------------
//! Each point moves toward the mean of its four neighbours (the heat
//! equation, one explicit step) and toward the air. The points under the tip
//! take its temperature when it touches the plate.
// ----------------------------------------------------------------------------
void HotPlate::step(float p_seconds, compages::Vector3f const& p_tool)
{
    compages::Grid2D const& grid = m_temperature.grid();
    const float rate = DIFFUSIVITY * p_seconds / (grid.spacing * grid.spacing); // < 0.25 to stay stable
    const bool touches = (p_tool.y < TABLE_TOP + 0.01f);
    for (int j = 1; j < grid.rows - 1; ++j)
    {
        for (int i = 1; i < grid.columns - 1; ++i)
        {
            const compages::Vector2f p = grid.point(i, j);
            const float t = m_temperature(i, j);
            const float spread = m_temperature(i - 1, j) + m_temperature(i + 1, j) + m_temperature(i, j - 1) +
                                 m_temperature(i, j + 1) - (4.0f * t);
            const bool under = touches && (std::hypot(p.x - p_tool.x, p.y - p_tool.z) < TIP_RADIUS);
            m_next(i, j) = under ? TIP : t + (rate * spread) - (COOLING * p_seconds * (t - ROOM));
        }
    }
    std::swap(m_temperature, m_next);
}

// ----------------------------------------------------------------------------
//! Before the first frame: the World moves the arm in small steps of time,
//! and the tool heats the plate at each one, so a trail shows at once.
// ----------------------------------------------------------------------------
void HotPlate::warmUp(compages::renderer::Scene& p_scene, Arm const& p_arm, float p_seconds)
{
    compages::world::ViewFrame frame;
    frame.elapsed = 0.05f;
    for (frame.total = 0.0f; frame.total < p_seconds; frame.total += frame.elapsed)
    {
        p_scene.update(frame);
        step(frame.elapsed, p_arm.tool.worldPosition());
    }
}

// ----------------------------------------------------------------------------
compages::Status HotPlate::show(compages::renderer::Scene& p_scene)
{
    return p_scene.show(m_shown, m_temperature, m_look);
}

} // namespace examples::workshop
