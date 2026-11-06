// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06c_Hierarchy.hpp"

#include <cmath>
#include <numbers>

namespace examples
{

// ----------------------------------------------------------------------------
std::string Hierarchy::description() const
{
    return "A car is an entity without a shape. The body, the cabin and four "
           "wheels hang from it with parent(): only the car is moved each "
           "frame, and every part follows. The axes hung from the car show "
           "this pivot: X forward, Y up.";
}

// ----------------------------------------------------------------------------
compages::Status Hierarchy::setUp()
{
    m_scene.camera().position(0.0f, 5.0f, 7.0f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
    // A plane stands in XY: lay it down.
    m_scene.plane("Ground", compages::renderer::color(0.3f, 0.35f, 0.3f))
        .rotation(-std::numbers::pi_v<float> / 2.0f, { 1.0f, 0.0f, 0.0f })
        .scale(10.0f);

    // The pivot, then the parts placed relative to it.
    m_car = m_world.entity("Car");
    m_scene.axes(2.2f).parent(m_car);
    const auto red = compages::renderer::color(0.8f, 0.1f, 0.1f);
    const auto black = compages::renderer::color(0.05f, 0.05f, 0.05f);
    m_scene.box("Body", red).parent(m_car).position(0.0f, 0.5f, 0.0f).scale(2.0f, 0.4f, 1.0f);
    m_scene.box("Cabin", red).parent(m_car).position(-0.2f, 0.9f, 0.0f).scale(1.0f, 0.4f, 0.8f);
    for (const float x : { -0.7f, 0.7f })
    {
        for (const float z : { -0.55f, 0.55f })
        {
            // A cylinder stands along Y: turn it to roll along X.
            m_scene.cylinder("Wheel", black)
                .parent(m_car)
                .position(x, 0.3f, z)
                .rotation(std::numbers::pi_v<float> / 2.0f, { 1.0f, 0.0f, 0.0f })
                .scale(0.6f, 0.2f, 0.6f);
        }
    }
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Hierarchy::draw(compages::world::ViewFrame const& p_frame)
{
    // Round a circle of radius 2.5, nose forward.
    const float angle = 0.5f * p_frame.total;
    m_car.position(2.5f * std::cos(angle), 0.0f, -2.5f * std::sin(angle))
        .rotation(angle + (std::numbers::pi_v<float> / 2.0f), { 0.0f, 1.0f, 0.0f });
    m_scene.draw(p_frame);
}

} // namespace examples
