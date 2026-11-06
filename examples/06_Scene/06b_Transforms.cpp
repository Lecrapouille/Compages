// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06b_Transforms.hpp"

#include <cmath>

namespace examples
{

// ----------------------------------------------------------------------------
std::string Transforms::description() const
{
    return "Three shapes made and placed in one statement each: position(), "
           "rotation() and scale() chain. Each frame the box turns (rotate() "
           "adds a turn), the ball goes up and down (position() sets a place) "
           "and the cone breathes (scale() sets a size). Helpers, one line "
           "each, show it: a grid, axes hung from the box, the bounds of the "
           "cone.";
}

// ----------------------------------------------------------------------------
compages::Status Transforms::setUp()
{
    m_scene.camera().position(0.0f, 1.5f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);

    m_box = m_scene.box("Box", compages::renderer::color(0.9f, 0.2f, 0.1f))
                .position(-1.5f, 0.0f, 0.0f)
                .rotation(0.5f, { 0.0f, 1.0f, 0.0f });
    m_ball = m_scene.sphere("Ball", compages::renderer::color(0.2f, 0.7f, 0.3f));
    m_cone = m_scene.cone("Cone", compages::renderer::color(0.2f, 0.4f, 0.9f))
                 .position(1.5f, 0.0f, 0.0f)
                 .scale(0.6f, 1.2f, 0.6f);

    // Helpers: lines that show where things are and how they are turned.
    m_scene.grid(6.0f, 12u).position(0.0f, -1.0f, 0.0f);
    m_scene.axes(1.3f).parent(m_box);
    m_scene.showBounds(m_cone.id(), true);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Transforms::draw(compages::world::ViewFrame const& p_frame)
{
    const float t = p_frame.total;
    m_box.rotate(p_frame.elapsed, { 0.0f, 1.0f, 0.0f });
    m_ball.position(0.0f, 0.5f * std::sin(2.0f * t), 0.0f);
    m_cone.scale(0.6f, 1.2f + (0.3f * std::sin(3.0f * t)), 0.6f);
    m_scene.draw(p_frame);
}

} // namespace examples
