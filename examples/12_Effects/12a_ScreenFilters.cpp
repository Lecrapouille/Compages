// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "12_Effects/12a_ScreenFilters.hpp"

#include "Common/Gui.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <cmath>

using namespace compages::si::literals;

namespace examples
{

// Each filter writes vec3 effect(vec2 uv): the light of the pixel at uv.
// screenColor holds the light of the view (or what the filter before made),
// screenDepth its depth (1 near, 0 far), screenSize its size in pixels.

// Dark lines where the depth jumps: the outline of every shape.
static constexpr char const* EDGES = R"(
vec3 effect(vec2 uv)
{
    vec2 t = 1.0 / screenSize;
    float d = texture(screenDepth, uv).r;
    float jump = 0.0;
    jump = max(jump, abs(texture(screenDepth, uv + vec2(t.x, 0.0)).r - d));
    jump = max(jump, abs(texture(screenDepth, uv - vec2(t.x, 0.0)).r - d));
    jump = max(jump, abs(texture(screenDepth, uv + vec2(0.0, t.y)).r - d));
    jump = max(jump, abs(texture(screenDepth, uv - vec2(0.0, t.y)).r - d));
    // The depth is reversed (1/distance): a jump counts relative to it.
    float edge = smoothstep(0.08, 0.2, jump / max(d, 1e-6));
    return texture(screenColor, uv).rgb * (1.0 - 0.85 * edge);
}
)";

// The tones of an old photograph.
static constexpr char const* SEPIA = R"(
uniform float amount;

vec3 effect(vec2 uv)
{
    vec3 c = texture(screenColor, uv).rgb;
    float y = dot(c, vec3(0.299, 0.587, 0.114));
    return mix(c, y * vec3(1.2, 1.0, 0.75), amount);
}
)";

// Darker towards the corners, as through an old lens.
static constexpr char const* VIGNETTE = R"(
uniform float strength;

vec3 effect(vec2 uv)
{
    vec2 d = uv - 0.5;
    return texture(screenColor, uv).rgb * (1.0 - strength * dot(d, d) * 2.0);
}
)";

// ----------------------------------------------------------------------------
std::string ScreenFilters::description() const
{
    return "Screen filters (ScreenEffects on a camera): each is a function "
           "vec3 effect(vec2 uv) of a few lines, run on every pixel of the "
           "view, the one after the other. They read the light of the view "
           "before the tone curve (screenColor) and its depth (screenDepth). "
           "Left, the camera as it is; right, a second camera at the same "
           "place with three filters: outlines where the depth jumps, the "
           "tones of an old photograph, a darker border. Each view chooses its "
           "effects.";
}

// ----------------------------------------------------------------------------
compages::Status ScreenFilters::setUp()
{
    m_scene.background(0.6f, 0.75f, 0.9f);
    m_scene.ambient(0.3f, 0.32f, 0.36f);
    m_scene.sun("Sun", compages::Vector3f(1.0f, 0.95f, 0.85f), 1.3f).position(8.0f, 12.0f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene.box("Ground", compages::renderer::color(0.5f, 0.55f, 0.45f)).position(0.0f, -0.1f, 0.0f).scale(30.0f, 0.2f, 30.0f);
    // A small village: houses with roofs, round a well.
    for (int i = 0; i < 7; ++i)
    {
        const float a = 6.2831853f * float(i) / 7.0f;
        const compages::Vector3f at(6.0f * std::cos(a), 0.0f, 6.0f * std::sin(a));
        const compages::si::Radians turn = compages::si::Radians(-a) + 90.0_deg;
        m_scene.box("House", compages::renderer::color(0.85f, 0.75f - 0.05f * float(i % 3), 0.6f))
            .position(at.x, 1.0f, at.z)
            .rotation(turn, { 0.0f, 1.0f, 0.0f })
            .scale(2.0f, 2.0f, 2.5f);
        m_scene.pyramid("Roof", compages::renderer::color(0.65f, 0.25f, 0.2f))
            .position(at.x, 2.75f, at.z)
            .rotation(turn, { 0.0f, 1.0f, 0.0f })
            .scale(2.6f, 1.5f, 3.0f);
    }
    m_scene.cylinder("Well", compages::renderer::color(0.6f, 0.6f, 0.62f)).position(0.0f, 0.5f, 0.0f).scale(1.6f, 1.0f, 1.6f);

    // Two cameras at the same place: the second is a child of the first.
    m_plain = m_scene.camera("Plain").position(0.0f, 7.0f, 16.0f);
    m_plain.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 0.5f, 1.0f };
    m_plain.add<compages::world::Orbit>(compages::Vector3f(0.0f, 1.0f, 0.0f));
    m_filtered = m_scene.camera("Filtered").parent(m_plain);
    m_filtered.get<compages::world::Camera>().viewport = { 0.5f, 0.0f, 0.5f, 1.0f };
    applyFilters();
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void ScreenFilters::applyFilters()
{
    compages::renderer::ScreenEffects effects;
    if (m_edges)
    {
        effects.filters.push_back({ .name = "Edges", .glsl = EDGES, .values = {} });
    }
    if (m_sepia)
    {
        effects.filters.push_back({ .name = "Sepia", .glsl = SEPIA, .values = { { "amount", m_amount } } });
    }
    if (m_vignette)
    {
        effects.filters.push_back({ .name = "Vignette", .glsl = VIGNETTE, .values = { { "strength", m_strength } } });
    }
    m_filtered.set(effects);
}

// ----------------------------------------------------------------------------
void ScreenFilters::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.update(p_frame);
    m_scene.render(m_plain);
    m_scene.render(m_filtered);
}

// ----------------------------------------------------------------------------
void ScreenFilters::controls()
{
    bool changed = ImGui::Checkbox("Outlines (depth)", &m_edges);
    changed |= ImGui::Checkbox("Sepia", &m_sepia);
    changed |= ImGui::SliderFloat("Amount", &m_amount, 0.0f, 1.0f);
    changed |= ImGui::Checkbox("Vignette", &m_vignette);
    changed |= ImGui::SliderFloat("Strength", &m_strength, 0.0f, 1.5f);
    if (changed)
    {
        applyFilters();
    }
}

} // namespace examples
