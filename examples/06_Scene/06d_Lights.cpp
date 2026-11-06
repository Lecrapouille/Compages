// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06d_Lights.hpp"

#include "Common/Gui.hpp"

#include <cmath>
#include <numbers>

namespace examples
{

// ----------------------------------------------------------------------------
std::string Lights::description() const
{
    return "A dim ambient light, a weak sun, and two coloured lamps going "
           "round. A lamp is an entity: it moves, and its bulb, an unlit "
           "white sphere, hangs from it. Try it: the exposure and the tone "
           "curve that turn the light into screen colours.";
}

// ----------------------------------------------------------------------------
compages::Status Lights::setUp()
{
    m_scene.background(0.02f, 0.02f, 0.03f);
    m_scene.camera().position(0.0f, 4.0f, 7.0f).lookAt(0.0f, 0.0f, 0.0f);
    // Colours are sRGB, as in a colour picker.
    m_scene.ambient(0.15f, 0.15f, 0.2f);
    m_scene.sun("Sun", { 1.0f, 1.0f, 1.0f }, 0.2f);

    m_scene.plane("Ground", compages::renderer::color(0.7f, 0.7f, 0.7f))
        .rotation(-std::numbers::pi_v<float> / 2.0f, { 1.0f, 0.0f, 0.0f })
        .scale(10.0f);
    for (int i = 0; i < 5; ++i)
    {
        m_scene.sphere("Ball", compages::renderer::color(0.8f, 0.8f, 0.8f))
            .position(-2.0f + float(i), 0.5f, 0.0f)
            .scale(0.8f);
    }

    m_red = m_scene.lamp("Red", { 1.0f, 0.2f, 0.1f }, 3.0f, 6.0f);
    m_blue = m_scene.lamp("Blue", { 0.1f, 0.3f, 1.0f }, 3.0f, 6.0f);
    for (compages::world::Entity* lamp : { &m_red, &m_blue })
    {
        m_scene.sphere("Bulb", compages::renderer::unlit(1.0f, 1.0f, 1.0f))
            .parent(*lamp)
            .scale(0.15f);
    }
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void Lights::draw(compages::world::ViewFrame const& p_frame)
{
    const float angle = p_frame.total;
    m_red.position(2.5f * std::cos(angle), 1.2f, 2.5f * std::sin(angle));
    m_blue.position(-2.5f * std::cos(angle), 1.2f, -2.5f * std::sin(angle));
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Lights::controls()
{
    compages::renderer::Environment& environment = m_scene.environment();
    ImGui::SliderFloat("Exposure", &environment.exposure, 0.1f, 4.0f, "%.2f");
    int curve = static_cast<int>(environment.tone_mapping);
    if (ImGui::Combo("Tone curve", &curve, "None\0Reinhard\0ACES\0"))
    {
        environment.tone_mapping =
            static_cast<compages::renderer::ToneMapping>(curve);
    }
}

} // namespace examples
