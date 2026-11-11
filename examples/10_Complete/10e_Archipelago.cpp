// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/10e_Archipelago.hpp"

#include "Common/DataPath.hpp"
#include "Common/Gui.hpp"
#include "Compages/Fields/HeightField.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <format>

//! \brief The sizes carry their unit (16.0_km, 2.4_km), turned into the
//! metres of the World by compages::toMetres().
using units::literals::operator""_km;
using units::literals::operator""_m;

namespace examples
{

using archipelago::MAP;
using archipelago::Page;
using archipelago::Place;
using archipelago::Theme;
using archipelago::Topic;
using compages::toMetres;
using compages::Vector2f;
using compages::Vector3f;

//! \brief The layout shown by default: the 118 public bookmarks of
//! IslandedBrowser (github.com/Lecrapouille/IslandedBrowser), in
//! Compages-data. COMPAGES_ARCHIPELAGO gives another one, written by
//! `IslandedBrowser -d DIR --layout` (DIR/layout.json).
static constexpr char const* KNOWLEDGE = "archipelago/bookmarks-layout.json";

static constexpr float VISIT_DISTANCE = toMetres(2.4_km);
static constexpr float OVERVIEW_DISTANCE = toMetres(17.0_km);
//! \brief Two clicks on the same place within this many seconds: a double
//! click.
static constexpr float DOUBLE_CLICK = 0.4f;

//! \brief The fog, and the background (sRGB). The fog goes through the tone
//! curve of the Scene, the background does not: SKY is what HAZE comes out
//! as on the screen (measured), so the horizon has no seam.
static const Vector3f HAZE{ 0.74f, 0.84f, 0.90f };
static const Vector3f SKY{ 0.81f, 0.855f, 0.88f };

// ----------------------------------------------------------------------------
Archipelago::~Archipelago()
{
    // The threads read the land: they end before it goes.
    m_loading.wait();
    m_fine.drop();
}

// ----------------------------------------------------------------------------
std::string Archipelago::description() const
{
    return "A map of knowledge after Chréage: the bookmarks of IslandedBrowser "
           "as islands of Voronoi cells around "
           "their graph, whose relief grows by uplift and river erosion. As "
           "the thesis maps contents along a "
           "strand: a theme is an island and its main river, a topic a "
           "tributary, a link a cell along it (a pin). "
           "Made in a thread behind a progress bar; near the camera, finer "
           "tiles and a sea whose vertices move "
           "with Gerstner waves. Left drag moves, right drag turns, the wheel "
           "zooms toward the mouse; over the picture, the interface of "
           "IslandedBrowser: a click selects, a double click opens or flies, "
           "/ searches, H helps";
}

// ----------------------------------------------------------------------------
void Archipelago::loadKnowledge()
{
    const char* const asked = std::getenv("COMPAGES_ARCHIPELAGO");
    const std::filesystem::path file =
        ((asked != nullptr) && (asked[0] != '\0')) ? asked
                                                   : dataPath(KNOWLEDGE);
    if (compages::Result<archipelago::Knowledge> knowledge =
            archipelago::loadKnowledge(file);
        knowledge)
    {
        m_knowledge = knowledge.take();
    }
    else
    {
        m_knowledge = archipelago::builtinKnowledge();
        m_message = knowledge.error();
    }
}

// ----------------------------------------------------------------------------
compages::Status Archipelago::setUp()
{
    const std::filesystem::path font = dataPath("fonts/DejaVuSans.ttf");
    if (font.empty())
    {
        return compages::failure(
            "fonts/DejaVuSans.ttf is missing from external/Compages-data/");
    }
    m_scene.font(font);
    m_scene.background(SKY.x, SKY.y, SKY.z).ambient(0.32f, 0.36f, 0.42f);
    m_scene.environment().fog_color = HAZE;
    // exp(-(density * d)^2): clear over the archipelago, haze at the horizon.
    m_scene.environment().fog_density = 1.0f / toMetres(70.0_km);
    // The shadow picture spans the 16 km of the map: 4 m a pixel.
    m_scene.environment().shadow_resolution = 4096u;
    // A low sun from the south-west: the relief shows, the sea glints.
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.6f)
        .position(-0.55f * MAP, 0.45f * MAP, 0.65f * MAP)
        .lookAt(0.0f, 0.0f, 0.0f);

    loadKnowledge();
    m_pins.create(m_scene, m_knowledge);
    m_highlight.create(m_scene);

    m_camera = m_scene.camera().position(
        0.0f, 0.75f * OVERVIEW_DISTANCE, 0.66f * OVERVIEW_DISTANCE);
    compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
    lens.near_plane = toMetres(2.0_m);
    lens.far_plane = toMetres(400.0_km);
    // A Rig: left drag moves the map, right drag turns, the wheel zooms
    // toward the mouse.
    m_camera.add<compages::world::Rig>(Vector3f::ZERO);
    compages::world::RigController& rig =
        m_camera.get<compages::world::Rig>().controller;
    rig.min_distance = toMetres(150.0_m);
    rig.max_distance = toMetres(60.0_km);
    rig.max_pitch = -0.12f;
    // The camera never enters a hill nor the sea: the ground as the screen
    // shows it (fine tiles near the camera, else the coarse map), at least
    // the level of the sea, plus room for the waves.
    rig.height_at = [this](float p_x, float p_z)
    {
        return m_ready ? std::max(0.0f, m_fine.groundAt(m_map.base, p_x, p_z))
                       : 0.0f;
    };
    rig.clearance = toMetres(15.0_m);
    COMPAGES_TRY(m_sea.create(m_scene));
    // The land is made in a thread: setUp() returns at once, the gallery
    // shows how far it has gone (progress()), draw() shows it when ready.
    startLoading();
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! In the loading thread: the land (40 % of the bar), its coarse patch.
// ----------------------------------------------------------------------------
Archipelago::Loaded Archipelago::makeMap(archipelago::Recipe const& p_recipe,
                                         Report& p_report)
{
    auto tell = [&p_report](float p_done, char const* p_step)
    {
        std::scoped_lock lock(p_report.mutex);
        p_report.done = p_done;
        p_report.step = p_step;
    };
    Loaded loaded;
    loaded.land =
        archipelago::makeLand(p_recipe,
                              [&tell](float p_done, char const* p_step)
                              { tell(0.4f * p_done, p_step); });
    loaded.base =
        archipelago::makePatch(loaded.land,
                               -0.5f * MAP,
                               -0.5f * MAP,
                               MAP,
                               archipelago::BASE_CELLS,
                               archipelago::BASE_PIXELS,
                               [&tell](float p_done, char const* p_step)
                               { tell(0.4f + (0.6f * p_done), p_step); });
    return loaded;
}

// ----------------------------------------------------------------------------
void Archipelago::startLoading()
{
    archipelago::Recipe recipe{ .seed = std::uint32_t(m_generation.seed),
                                .islands = {},
                                .regions = {},
                                .erosion_steps = m_generation.erosion_steps };
    for (Theme const& theme : m_knowledge.themes)
    {
        recipe.islands.push_back(theme.node);
    }
    for (Topic const& topic : m_knowledge.topics)
    {
        archipelago::Recipe::Group& group =
            recipe.regions.emplace_back(topic.theme, topic.node);
        for (Page const& page : topic.pages)
        {
            group.links.push_back(page.node);
        }
    }
    m_report = std::make_shared<Report>();
    m_loading.start([recipe = std::move(recipe), report = m_report]()
                    { return makeMap(recipe, *report); });
}

// ----------------------------------------------------------------------------
Archipelago::Progress Archipelago::progress() const
{
    if (!m_loading.busy())
    {
        return { 1.0f, {} };
    }
    std::scoped_lock lock(m_report->mutex);
    // 1 only once draw() has shown the land.
    return { std::min(m_report->done, 0.999f), m_report->step };
}

// ----------------------------------------------------------------------------
compages::Status Archipelago::finishLoading(Loaded p_loaded)
{
    // The fine tiles of the old land are made from it: they end first.
    m_fine.drop();
    m_map = std::move(p_loaded);
    COMPAGES_TRY(m_sea.show(m_scene, m_map.base));
    m_pins.place(m_map.land);
    m_highlight.reset();
    m_pointer.hovered = {};
    m_pointer.chosen = {};
    m_wildlife.place(
        m_scene, m_map.land, m_map.base, std::uint32_t(m_generation.seed));
    m_ready = true;
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
//! March from where the ray goes below the highest peak down to the sea, in
//! steps of half a coarse quad, then halve the last step.
// ----------------------------------------------------------------------------
Place Archipelago::placeUnder(Vector2f p_mouse) const
{
    compages::renderer::CameraFrame const& camera = m_scene.lastCamera();
    if ((camera.viewport_width == 0u) || (camera.viewport_height == 0u))
    {
        return {};
    }
    const compages::Ray ray = camera.screenRay(
        p_mouse.x, p_mouse.y, camera.viewport_width, camera.viewport_height);
    if (ray.direction.y >= -1.0e-4f)
    {
        return {}; // toward the sky
    }
    archipelago::Patch const& base = m_map.base;
    auto ground = [&base](float p_x, float p_z)
    { return std::max(0.0f, base.heightAt(p_x, p_z)); };
    const float quad = MAP / float(archipelago::BASE_CELLS);
    const float begin = std::max(
        0.0f, ((1.2f * archipelago::PEAK) - ray.origin.y) / ray.direction.y);
    const float end = -ray.origin.y / ray.direction.y;
    const float step = 0.5f * quad / std::max(std::abs(ray.direction.y), 0.05f);
    const float hit =
        compages::intersectHeight(
            ray, ground, begin, std::min(end, begin + (8000.0f * step)), step)
            .value_or(end);
    const Vector3f point = ray.pointAt(hit);
    if (const bool outside = (std::abs(point.x) >= 0.5f * MAP) ||
                             (std::abs(point.z) >= 0.5f * MAP);
        outside || (base.heightAt(point.x, point.z) <= 0.0f))
    {
        return {};
    }
    return m_map.land.placeOf(point.x, point.z);
}

// ----------------------------------------------------------------------------
void Archipelago::handleMouse(compages::world::Input const& p_input)
{
    // Ctrl is not one of the keys of the frame: Dear ImGui knows it.
    const bool control = ImGui::GetIO().KeyCtrl;
    if (p_input.mouse_left_pressed)
    {
        m_pointer.press = p_input.mouse;
    }
    if (const bool released =
            m_pointer.was_down && !p_input.mouse_left && p_input.mouse_over &&
            (compages::vector::norm(p_input.mouse - m_pointer.press) < 4.0f);
        released)
    {
        click(placeUnder(p_input.mouse), control);
    }
    if (const bool middle = p_input.mouse_middle &&
                            !m_pointer.middle_was_down && p_input.mouse_over;
        middle)
    {
        open(placeUnder(p_input.mouse));
    }
    m_pointer.was_down = p_input.mouse_left;
    m_pointer.middle_was_down = p_input.mouse_middle;
    m_pointer.hovered = (p_input.mouse_over && !p_input.mouse_left)
                            ? placeUnder(p_input.mouse)
                            : Place{};
}

// ----------------------------------------------------------------------------
void Archipelago::click(Place p_place, bool p_open)
{
    if (m_panels.helpShown())
    {
        return;
    }
    const bool twice = (p_place.region >= 0) &&
                       (p_place.region == m_pointer.clicked.region) &&
                       (p_place.part == m_pointer.clicked.part) &&
                       ((m_time - m_pointer.clicked_at) < DOUBLE_CLICK);
    m_pointer.clicked = twice ? Place{} : p_place;
    m_pointer.clicked_at = m_time;
    if (p_place.region < 0)
    {
        select(Place{}); // a click on the sea
    }
    else if (p_open || (twice && (p_place.part >= 0)))
    {
        open(p_place);
    }
    else if (twice)
    {
        zoomOn(p_place);
    }
    else
    {
        select(p_place);
    }
}

// ----------------------------------------------------------------------------
void Archipelago::select(Place p_place)
{
    m_pointer.chosen =
        Place{ .cell = -1, .region = p_place.region, .part = p_place.part };
}

// ----------------------------------------------------------------------------
void Archipelago::zoomOn(Place p_place)
{
    if (p_place.region < 0)
    {
        return;
    }
    archipelago::Region const& region =
        m_map.land.regions[std::size_t(p_place.region)];
    const Vector2f site = (p_place.part >= 0)
                              ? region.parts[std::size_t(p_place.part)].site
                              : region.site;
    m_camera.get<compages::world::Rig>().controller.flyTo(
        Vector3f(site.x, 0.0f, site.y),
        (p_place.part >= 0) ? VISIT_DISTANCE : 1.5f * VISIT_DISTANCE);
}

// ----------------------------------------------------------------------------
void Archipelago::zoomOnIsland(std::size_t p_island)
{
    archipelago::Island const& island = m_map.land.islands[p_island];
    m_camera.get<compages::world::Rig>().controller.flyTo(
        Vector3f(island.centre.x, 0.0f, island.centre.y),
        std::max(VISIT_DISTANCE, 3.0f * island.radius));
}

// ----------------------------------------------------------------------------
void Archipelago::home()
{
    m_camera.get<compages::world::Rig>().controller.flyTo(Vector3f::ZERO,
                                                          OVERVIEW_DISTANCE);
}

// ----------------------------------------------------------------------------
void Archipelago::open(Place p_place)
{
    if ((p_place.region < 0) ||
        (std::size_t(p_place.region) >= m_knowledge.topics.size()))
    {
        return;
    }
    std::vector<std::string> urls;
    std::vector<Page> const& pages =
        m_knowledge.topics[std::size_t(p_place.region)].pages;
    for (std::size_t k = 0u; k < pages.size(); ++k)
    {
        if (((p_place.part < 0) || (int(k) == p_place.part)) &&
            !pages[k].url.empty())
        {
            urls.push_back(pages[k].url);
        }
    }
    m_panels.toast(m_browser.open(urls));
}

// ----------------------------------------------------------------------------
void Archipelago::overlay(Overlay const& p_view)
{
    if (!m_ready)
    {
        return;
    }
    m_signs.draw(p_view,
                 m_scene.lastCamera(),
                 m_map.land,
                 m_knowledge,
                 m_pointer.hovered,
                 m_pointer.chosen,
                 m_view.names);
    m_panels.draw(
        p_view,
        m_knowledge,
        m_pointer.hovered,
        m_pointer.chosen,
        m_camera.get<compages::world::Rig>().controller.currentDistance(),
        m_view,
        *this);
}

// ----------------------------------------------------------------------------
void Archipelago::draw(compages::world::ViewFrame const& p_frame)
{
    m_time += p_frame.elapsed;
    m_browser.reap();
    // The land made in the thread, shown when it is ready.
    if (m_loading.ready())
    {
        if (const compages::Status shown = finishLoading(m_loading.take());
            !shown)
        {
            compages::gpu::reportError(shown.error());
        }
    }
    if (!m_ready)
    {
        m_scene.draw(p_frame);
        return;
    }
    compages::world::RigController const& rig =
        m_camera.get<compages::world::Rig>().controller;
    m_fine.update(m_scene,
                  m_map.land,
                  m_sea.map(),
                  rig.currentTarget(),
                  rig.currentDistance(),
                  m_view.details,
                  m_sea.tiles());
    handleMouse(p_frame.input);
    m_pins.update(m_camera.position());
    if (m_view.animals)
    {
        m_wildlife.animate(m_map.base,
                           p_frame.elapsed * m_view.time_scale,
                           m_time,
                           m_camera.position());
    }
    // The region under the mouse, draped on the ground; the outline of the
    // chosen one.
    m_highlight.show(m_scene,
                     m_map.land,
                     m_map.base,
                     m_fine,
                     m_pointer.hovered.region,
                     rig.currentDistance());
    if (m_view.borders && (m_pointer.chosen.region >= 0))
    {
        std::vector<Vector3f> const& border =
            m_map.land.regions[std::size_t(m_pointer.chosen.region)].border;
        for (std::size_t k = 0u; (k + 1u) < border.size(); k += 2u)
        {
            m_scene.debug().line(
                border[k], border[k + 1u], { 1.0f, 0.82f, 0.48f });
        }
    }
    m_sea.update(m_scene, rig, m_time);
    m_scene.draw(p_frame);
}

// ----------------------------------------------------------------------------
void Archipelago::controls()
{
    ImGui::TextWrapped("Contents: %s (%zu islands, %zu regions)",
                       m_knowledge.source.c_str(),
                       m_knowledge.themes.size(),
                       m_knowledge.topics.size());
    ImGui::InputInt("Seed", &m_generation.seed);
    ImGui::SliderInt("Erosion steps", &m_generation.erosion_steps, 0, 300);
    ImGui::SetItemTooltip(
        "Steps of 40 000 years: the rivers cut deeper valleys.");
    ImGui::BeginDisabled(m_loading.busy());
    if (ImGui::Button("Generate"))
    {
        startLoading();
    }
    ImGui::EndDisabled();
    if (!m_message.empty())
    {
        ImGui::TextWrapped("%s", m_message.c_str());
    }
    if (!m_ready)
    {
        return;
    }

    // Every region, by island, to fly to.
    const int chosen = m_pointer.chosen.region;
    if (const std::string current =
            (chosen < 0)
                ? std::string("The whole archipelago")
                : std::format(
                      "{} / {}",
                      m_knowledge
                          .themes[m_knowledge.topics[std::size_t(chosen)].theme]
                          .name,
                      m_knowledge.topics[std::size_t(chosen)].name);
        ImGui::BeginCombo("Fly to", current.c_str()))
    {
        if (ImGui::Selectable("The whole archipelago", chosen < 0))
        {
            select(Place{});
            home();
        }
        for (std::size_t r = 0u; r < m_knowledge.topics.size(); ++r)
        {
            Topic const& topic = m_knowledge.topics[r];
            const std::string name = std::format(
                "{} / {}", m_knowledge.themes[topic.theme].name, topic.name);
            if (ImGui::Selectable(name.c_str(), int(r) == chosen))
            {
                const Place region{ .cell = -1, .region = int(r), .part = -1 };
                select(region);
                zoomOn(region);
            }
        }
        ImGui::EndCombo();
    }
    compages::world::RigController& rig =
        m_camera.get<compages::world::Rig>().controller;
    ImGui::SliderFloat("Flight pace (s)", &rig.flight_pace, 0.2f, 2.0f, "%.2f");
    ImGui::Checkbox("Borders", &m_view.borders);
    ImGui::SameLine();
    ImGui::Checkbox("Names", &m_view.names);
    ImGui::SameLine();
    ImGui::Checkbox("Animals", &m_view.animals);
    ImGui::SameLine();
    ImGui::Checkbox("Fine tiles", &m_view.details);
    ImGui::SliderFloat(
        "Animals' time", &m_view.time_scale, 1.0f, 30.0f, "x %.0f");
    ImGui::Text("Fine tiles shown: %zu (8 m, 2 m a pixel)", m_fine.shown());
    ImGui::TextWrapped("Click a region: its details on the right of the "
                       "picture. H over the picture: the help.");
}

} // namespace examples
