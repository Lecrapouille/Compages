// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "06_Scene/06n_VolumetricFog.hpp"

#include "Common/Gui.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace examples
{

static constexpr float PI = std::numbers::pi_v<float>;

// ----------------------------------------------------------------------------
std::string VolumetricFogDemo::description() const
{
    return "Fog that the light crosses (Environment::volumetric). The Renderer "
           "keeps a 3D picture that follows the camera, 128 x 72 tiles of the "
           "picture times 64 slices of the depth (froxels, after Wronski, "
           "SIGGRAPH 2014). A compute shader puts in each froxel how thick the "
           "fog is and the light it receives, with the shadows of the sun and "
           "of the lamps; a second one sums them from the camera outward. The "
           "sun comes through the slats of the roof in shafts; two lamps cast "
           "halos and shadows in the air. Try it: on and off (the plain fog), "
           "how thick, how forward the light goes, the clumps.";
}

// ----------------------------------------------------------------------------
compages::Status VolumetricFogDemo::setUp()
{
    m_scene.background(0.05f, 0.06f, 0.08f);
    m_scene.ambient(0.05f, 0.055f, 0.07f);
    m_scene.environment().fog_color = compages::Vector3f(0.05f, 0.06f, 0.08f);
    m_scene.environment().fog_density = 0.03f;
    m_scene.environment().lamp_shadow_resolution = 512u;
    compages::renderer::VolumetricFog& fog = m_scene.environment().volumetric;
    fog.enabled = true;
    fog.density = 0.035f;
    fog.height = 0.0f;
    fog.falloff = 0.25f;
    fog.color = compages::Vector3f(0.85f, 0.88f, 0.95f);
    fog.anisotropy = 0.6f;
    fog.noise = 0.5f;
    fog.distance = 40.0f;

    m_scene.sun("Sun", compages::Vector3f(1.0f, 0.88f, 0.7f), 1.8f).position(6.0f, 14.0f, -5.0f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene.camera().position(-8.5f, 1.8f, 4.0f).add<compages::world::Orbit>(compages::Vector3f(1.0f, 2.2f, -1.0f));

    const compages::renderer::Look stone = compages::renderer::color(0.55f, 0.53f, 0.5f);
    m_scene.box("Floor", stone).position(0.0f, -0.1f, 0.0f).scale(24.0f, 0.2f, 24.0f);
    // A hall closed on three sides; a roof of slats with gaps, held by two
    // rows of columns: the sun comes in through the gaps only.
    const compages::renderer::Look wall = compages::renderer::color(0.45f, 0.43f, 0.4f);
    m_scene.box("Wall", wall).position(0.0f, 3.1f, -7.2f).scale(22.0f, 6.4f, 0.4f);
    m_scene.box("Wall", wall).position(0.0f, 3.1f, 7.2f).scale(22.0f, 6.4f, 0.4f);
    m_scene.box("Wall", wall).position(11.0f, 3.1f, 0.0f).scale(0.4f, 6.4f, 14.8f);
    for (int k = -6; k <= 6; ++k)
    {
        m_scene.box("Slat", compages::renderer::color(0.3f, 0.28f, 0.26f))
            .position(float(k) * 1.6f, 6.2f, 0.0f)
            .scale(1.0f, 0.25f, 14.8f);
    }
    for (int i = -3; i <= 3; ++i)
    {
        for (float z : { -5.5f, 5.5f })
        {
            m_scene.cylinder("Column", stone).position(float(i) * 3.2f, 3.0f, z).scale(0.7f, 6.0f, 0.7f);
        }
    }
    for (int i = 0; i < 5; ++i)
    {
        m_scene.box("Crate", compages::renderer::color(0.6f, 0.45f, 0.3f))
            .position(-5.0f + (2.6f * float(i)), 0.5f, (i % 2 == 0) ? 1.5f : -1.0f)
            .rotation(0.4f * float(i), compages::Vector3f(0.0f, 1.0f, 0.0f));
    }

    // Two lamps going round, casting shadows (in the air too).
    m_red = m_scene.lamp("Red", compages::Vector3f(1.0f, 0.25f, 0.15f), 3.0f, 9.0f);
    m_blue = m_scene.lamp("Blue", compages::Vector3f(0.2f, 0.45f, 1.0f), 3.0f, 9.0f);
    for (compages::world::Entity* lamp : { &m_red, &m_blue })
    {
        lamp->get<compages::world::PointLight>().shadows = true;
        m_scene.sphere("Bulb", compages::renderer::unlit(1.0f, 1.0f, 1.0f))
            .parent(*lamp)
            .scale(0.15f)
            .get<compages::renderer::MeshRenderer>()
            .flags = compages::renderer::RenderFlags::NoShadowCaster;
    }
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void VolumetricFogDemo::draw(compages::world::ViewFrame const& p_frame)
{
    m_time += std::min(p_frame.elapsed, 0.1f);
    const float a = 0.4f * m_time;
    m_red.position(4.0f * std::cos(a), 1.6f, 3.0f * std::sin(a));
    m_blue.position(4.0f * std::cos(a + PI), 2.2f, 3.0f * std::sin(a + PI));
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void VolumetricFogDemo::controls()
{
    compages::renderer::VolumetricFog& fog = m_scene.environment().volumetric;
    ImGui::Checkbox("Volumetric (off: the plain fog)", &fog.enabled);
    ImGui::SliderFloat("Density (per metre)", &fog.density, 0.0f, 0.25f, "%.3f");
    ImGui::SliderFloat("Thins out with height", &fog.falloff, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Forward (anisotropy)", &fog.anisotropy, -0.5f, 0.9f, "%.2f");
    ImGui::SliderFloat("Clumps", &fog.noise, 0.0f, 1.0f, "%.2f");
    ImGui::ColorEdit3("Colour", &fog.color.x);
}

} // namespace examples
