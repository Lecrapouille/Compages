// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Labels.hpp"

#include "Compages/Renderer/Components/Text.hpp"

#include <algorithm>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_km;
using units::literals::operator""_m;

static constexpr float ISLAND_NAMES_BEYOND = compages::toMetres(3.5_km);
static constexpr float REGION_NAMES_WITHIN = compages::toMetres(9.0_km);
static constexpr float PART_NAMES_WITHIN = compages::toMetres(3.2_km);
//! \brief A pin is drawn at least this part of its distance to the camera
//! high, like the icons of a map.
static constexpr float PIN_SEEN = 1.0f / 500.0f;

// ----------------------------------------------------------------------------
std::string shortTitle(std::string const& p_title)
{
    constexpr std::size_t MOST = 40u;
    if (p_title.size() <= MOST)
    {
        return p_title;
    }
    std::size_t cut = MOST;
    while ((cut > 0u) &&
           ((static_cast<unsigned char>(p_title[cut]) & 0xC0u) == 0x80u))
    {
        --cut;
    }
    return p_title.substr(0u, cut) + "…";
}

//! \brief Set a component when shown, remove it when not.
template <typename Component>
static void show(compages::world::Entity p_entity,
                 bool p_shown,
                 Component const& p_component)
{
    if (p_shown)
    {
        p_entity.set(p_component);
    }
    else if (p_entity.has<Component>())
    {
        p_entity.remove<Component>();
    }
}

// ----------------------------------------------------------------------------
void Labels::create(compages::renderer::Scene& p_scene,
                    Knowledge const& p_knowledge)
{
    compages::world::World& world = p_scene.world();
    for (Theme const& theme : p_knowledge.themes)
    {
        m_islands.push_back(world.entity(theme.name));
    }
    for (Topic const& topic : p_knowledge.topics)
    {
        m_regions.push_back(world.entity(topic.name));
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
void Labels::place(Land const& p_land, Knowledge const& p_knowledge)
{
    for (std::size_t k = 0u; k < m_islands.size(); ++k)
    {
        m_islands[k]
            .position(p_land.islands[k].peak +
                      Vector3f(0.0f, compages::toMetres(80.0_m), 0.0f))
            .set(compages::renderer::Label{ .text = p_knowledge.themes[k].name,
                                            .color = { 1.0f, 0.97f, 0.88f },
                                            .pixels = 24.0f,
                                            .hide_closer_than =
                                                ISLAND_NAMES_BEYOND });
    }
    auto ground = [&p_land](Vector2f p_at)
    { return std::max(0.0f, p_land.heightAt(p_at.x, p_at.y)); };
    for (std::size_t r = 0u; r < m_regions.size(); ++r)
    {
        Region const& region = p_land.regions[r];
        m_regions[r].position(region.site.x,
                              ground(region.site) + compages::toMetres(30.0_m),
                              region.site.y);
        for (std::size_t part = 0u; part < region.parts.size(); ++part)
        {
            const Vector2f site = region.parts[part].site;
            m_pins[r][part].position(site.x, ground(site), site.y);
        }
    }
}

// ----------------------------------------------------------------------------
void Labels::update(Vector3f p_eye, Knowledge const& p_knowledge, bool p_names)
{
    for (std::size_t r = 0u; r < m_regions.size(); ++r)
    {
        const float distance =
            compages::vector::norm(m_regions[r].position() - p_eye);
        show(m_regions[r],
             p_names && (distance < REGION_NAMES_WITHIN),
             compages::renderer::Label{ .text = p_knowledge.topics[r].name,
                                        .color = { 1.0f, 1.0f, 1.0f },
                                        .pixels = 15.0f,
                                        .hide_closer_than = 0.0f });
        Topic const& topic = p_knowledge.topics[r];
        for (std::size_t k = 0u; k < m_pins[r].size(); ++k)
        {
            // Within the distance of the names of the regions, at least as
            // large as the eye needs; a link named as its region (its first
            // link, often) is already named.
            compages::world::Entity pin = m_pins[r][k];
            const float away = compages::vector::norm(pin.position() - p_eye);
            pin.enable(away < REGION_NAMES_WITHIN)
                .scale(std::max(1.0f, away * PIN_SEEN));
            show(pin,
                 p_names && (away < PART_NAMES_WITHIN) &&
                     (topic.pages[k].title != topic.name),
                 compages::renderer::Label{
                     .text = shortTitle(topic.pages[k].title),
                     .color = { 0.88f, 0.92f, 0.95f },
                     .pixels = 12.0f,
                     .hide_closer_than = 0.0f });
        }
    }
}

} // namespace examples::archipelago
