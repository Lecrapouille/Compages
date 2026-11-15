// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "12_Effects/12b_Bloom.hpp"

#include "Common/Gui.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace examples
{

// ----------------------------------------------------------------------------
std::string BloomDemo::description() const
{
    return "Bloom (ScreenEffects::bloom on the camera). With screen effects, "
           "the view is drawn in linear light, where a bulb is brighter than "
           "the white of the screen. The light above the threshold is kept, "
           "made half as large five times, then enlarged again and summed: a "
           "halo of every size, added back before the tone curve. Try it: on "
           "and off, the threshold (lower: the lit floor glows too), the "
           "strength.";
}

// ----------------------------------------------------------------------------
compages::Status BloomDemo::setUp()
{
    m_scene.background(0.01f, 0.01f, 0.03f);
    m_scene.ambient(0.03f, 0.03f, 0.05f);
    m_scene.sun("Moon", compages::Vector3f(0.5f, 0.6f, 0.9f), 0.25f).position(-4.0f, 10.0f, 6.0f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene.box("Floor", compages::renderer::color(0.5f, 0.5f, 0.52f)).position(0.0f, -0.1f, 0.0f).scale(20.0f, 0.2f, 20.0f);
    for (int i = 0; i < 5; ++i)
    {
        m_scene.cylinder("Pillar", compages::renderer::color(0.7f, 0.68f, 0.65f))
            .position(-6.0f + 3.0f * float(i), 1.5f, -3.0f)
            .scale(0.6f, 3.0f, 0.6f);
    }

    // Lamps carrying a bulb: an unlit sphere, as bright as the screen allows
    // without effects, and much brighter in linear light.
    const std::array<compages::Vector3f, 4> colours = { compages::Vector3f(1.0f, 0.45f, 0.15f),
                                                        compages::Vector3f(0.2f, 0.6f, 1.0f),
                                                        compages::Vector3f(0.4f, 1.0f, 0.4f),
                                                        compages::Vector3f(1.0f, 0.3f, 0.8f) };
    for (compages::Vector3f const& c : colours)
    {
        compages::world::Entity lamp = m_scene.lamp("Lamp", c, 2.5f, 7.0f);
        m_scene.sphere("Bulb", compages::renderer::unlit(c.x, c.y, c.z))
            .parent(lamp)
            .scale(0.3f)
            .get<compages::renderer::MeshRenderer>()
            .flags = compages::renderer::RenderFlags::NoShadowCaster;
        m_lamps.push_back(lamp);
    }

    m_camera = m_scene.camera().position(0.0f, 4.0f, 11.0f);
    m_camera.add<compages::world::Orbit>(compages::Vector3f(0.0f, 1.0f, 0.0f));
    m_camera.set(compages::renderer::ScreenEffects{
        .filters = {}, .bloom = { .enabled = true, .threshold = 1.0f, .strength = 0.6f } });
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void BloomDemo::draw(compages::world::ViewFrame const& p_frame)
{
    m_time += std::min(p_frame.elapsed, 0.1f);
    for (std::size_t i = 0; i < m_lamps.size(); ++i)
    {
        const float a = (0.5f * m_time) + (1.5707963f * float(i));
        m_lamps[i].position(5.0f * std::cos(a), 1.0f + 0.5f * std::sin(2.0f * a), 2.0f + 3.0f * std::sin(a));
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void BloomDemo::controls()
{
    compages::renderer::Bloom& bloom = m_camera.get<compages::renderer::ScreenEffects>().bloom;
    ImGui::Checkbox("Bloom", &bloom.enabled);
    ImGui::SliderFloat("Threshold", &bloom.threshold, 0.0f, 4.0f, "%.2f");
    ImGui::SliderFloat("Strength", &bloom.strength, 0.0f, 2.0f, "%.2f");
}

} // namespace examples
