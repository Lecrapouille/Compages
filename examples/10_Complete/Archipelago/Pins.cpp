// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Pins.hpp"

#include <algorithm>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_km;

//! \brief The pins show within this distance of the camera, and are drawn
//! at least this part of their distance high, like the icons of a map.
static constexpr float PINS_WITHIN = compages::toMetres(9.0_km);
static constexpr float PIN_SEEN = 1.0f / 500.0f;

// ----------------------------------------------------------------------------
void Pins::create(compages::renderer::Scene& p_scene,
                  Knowledge const& p_knowledge)
{
    compages::world::World& world = p_scene.world();
    for (Topic const& topic : p_knowledge.topics)
    {
        std::array<float, 3> const& hue = p_knowledge.themes[topic.theme].hue;
        std::vector<compages::world::Entity>& pins = m_pins.emplace_back();
        for (Page const& page : topic.pages)
        {
            compages::world::Entity pin = world.entity(page.title);
            p_scene
                .cylinder("Pin", compages::renderer::color(0.95f, 0.95f, 0.92f))
                .parent(pin)
                .position(0.0f, 6.0f, 0.0f)
                .scale(0.6f, 12.0f, 0.6f);
            p_scene
                .sphere("Pin head",
                        compages::renderer::color(hue[0], hue[1], hue[2]))
                .parent(pin)
                .position(0.0f, 13.0f, 0.0f)
                .scale(4.0f);
            pin.enable(false);
            pins.push_back(pin);
        }
    }
}

// ----------------------------------------------------------------------------
void Pins::place(Land const& p_land)
{
    for (std::size_t r = 0u; r < m_pins.size(); ++r)
    {
        Region const& region = p_land.regions[r];
        for (std::size_t part = 0u; part < region.parts.size(); ++part)
        {
            const Vector2f site = region.parts[part].site;
            m_pins[r][part].position(
                site.x,
                std::max(0.0f, p_land.heightAt(site.x, site.y)),
                site.y);
        }
    }
}

// ----------------------------------------------------------------------------
void Pins::update(Vector3f p_eye)
{
    for (std::vector<compages::world::Entity>& pins : m_pins)
    {
        for (compages::world::Entity& pin : pins)
        {
            const float away = compages::vector::norm(pin.position() - p_eye);
            pin.enable(away < PINS_WITHIN)
                .scale(std::max(1.0f, away * PIN_SEEN));
        }
    }
}

} // namespace examples::archipelago
