// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06m_ManyLights.hpp"

#include "Common/Gui.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <random>

namespace examples
{

static constexpr int MOST = 200;
static constexpr float PI = std::numbers::pi_v<float>;

// ----------------------------------------------------------------------------
std::string ManyLights::description() const
{
    return "Two hundred coloured lamps over a dark floor, in one picture. The "
           "four nearest the camera have uniforms of their own (and could "
           "cast shadows); the Renderer sorts the others into clusters, the "
           "picture cut into 16 x 9 tiles and the depth into 24 slices that "
           "grow with the distance (LightClusters, after Olsson et al. 2012). "
           "A pixel reads only the lamps of its cluster, so the cost follows "
           "how many lamps reach a pixel, not how many there are. Try it: how "
           "many, their range.";
}

// ----------------------------------------------------------------------------
compages::Status ManyLights::setUp()
{
    m_scene.background(0.01f, 0.01f, 0.015f);
    m_scene.ambient(0.03f, 0.03f, 0.04f);
    m_scene.environment().default_light_color = compages::Vector3f(0.0f, 0.0f, 0.0f);
    m_scene.camera().position(0.0f, 7.0f, 13.0f).add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.0f, 0.0f));

    m_scene.plane("Floor", compages::renderer::color(0.55f, 0.55f, 0.58f))
        .rotation(-0.5f * PI, { 1.0f, 0.0f, 0.0f })
        .scale(24.0f);
    // Pillars and balls, for the light to fall on.
    for (int i = -3; i <= 3; ++i)
    {
        for (int j = -3; j <= 3; ++j)
        {
            const bool pillar = ((i + j) % 2) == 0;
            compages::world::Entity thing =
                pillar ? m_scene.box("Pillar", compages::renderer::color(0.7f, 0.7f, 0.72f))
                       : m_scene.sphere("Ball", compages::renderer::color(0.75f, 0.75f, 0.75f));
            thing.position(float(i) * 3.0f, pillar ? 0.9f : 0.45f, float(j) * 3.0f)
                .scale(pillar ? compages::Vector3f(0.6f, 1.8f, 0.6f) : compages::Vector3f(0.9f, 0.9f, 0.9f));
        }
    }

    std::mt19937 random(17u);
    std::uniform_real_distribution<float> unit(0.0f, 1.0f);
    for (int n = 0; n < MOST; ++n)
    {
        // A saturated colour: a hue round the wheel.
        const float hue = unit(random) * 6.0f;
        const float x = 1.0f - std::abs(std::fmod(hue, 2.0f) - 1.0f);
        const int sector = int(hue);
        const compages::Vector3f colour = (sector == 0) ? compages::Vector3f(1.0f, x, 0.0f)
                                        : (sector == 1) ? compages::Vector3f(x, 1.0f, 0.0f)
                                        : (sector == 2) ? compages::Vector3f(0.0f, 1.0f, x)
                                        : (sector == 3) ? compages::Vector3f(0.0f, x, 1.0f)
                                        : (sector == 4) ? compages::Vector3f(x, 0.0f, 1.0f)
                                                        : compages::Vector3f(1.0f, 0.0f, x);
        Firefly fly;
        fly.lamp = m_scene.lamp("Firefly", colour, 1.1f, m_range);
        m_scene.sphere("Glow", compages::renderer::unlit(colour.x, colour.y, colour.z))
            .parent(fly.lamp)
            .scale(0.08f)
            .get<compages::renderer::MeshRenderer>()
            .flags = compages::renderer::RenderFlags::NoShadowCaster;
        fly.radius = 0.5f + (2.0f * unit(random));
        fly.speed = (0.3f + unit(random)) * ((unit(random) < 0.5f) ? -1.0f : 1.0f);
        fly.phase = 2.0f * PI * unit(random);
        fly.height = 0.25f + (1.6f * unit(random));
        fly.centre = compages::Vector2f((unit(random) - 0.5f) * 20.0f, (unit(random) - 0.5f) * 20.0f);
        m_flies.push_back(fly);
    }
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void ManyLights::draw(compages::world::ViewFrame const& p_frame)
{
    if (m_moving)
    {
        m_time += std::min(p_frame.elapsed, 0.1f);
    }
    for (std::size_t n = 0u; n < m_flies.size(); ++n)
    {
        Firefly& fly = m_flies[n];
        const float a = fly.phase + (fly.speed * m_time);
        fly.lamp.enable(int(n) < m_shown)
            .position(fly.centre.x + (fly.radius * std::cos(a)),
                      fly.height + (0.2f * std::sin(2.3f * a)),
                      fly.centre.y + (fly.radius * std::sin(a)));
        fly.lamp.get<compages::world::PointLight>().range = m_range;
    }
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void ManyLights::controls()
{
    ImGui::SliderInt("Lamps", &m_shown, 0, MOST);
    ImGui::SliderFloat("Range (m)", &m_range, 0.5f, 6.0f, "%.1f");
    ImGui::Checkbox("Moving", &m_moving);
}

} // namespace examples
