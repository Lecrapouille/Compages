// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Signs.hpp"

#include "Common/Gui.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>
#include <numeric>

namespace examples::archipelago
{

using compages::Vector2f;
using compages::Vector3f;
using units::literals::operator""_km;
using units::literals::operator""_m;

//! \brief The islands are named from afar, the regions closer.
static constexpr float ISLAND_NAMES_BEYOND = compages::toMetres(3.5_km);
static constexpr float REGION_NAMES_WITHIN = compages::toMetres(9.0_km);
//! \brief The head of a pin is this high over the ground.
static constexpr float PIN_HEAD = compages::toMetres(13.0_m);
//! \brief At most this many names round a region; the others are counted.
static constexpr std::size_t MOST_LINKS = 24u;
//! \brief The width of the tab of a sign, the space around its text.
static constexpr float TAB = 14.0f;
static constexpr float PADDING = 8.0f;
static constexpr float TURN = 2.0f * std::numbers::pi_v<float>;

//! \brief A colour from 0-1 components.
static ImU32 colour(std::array<float, 3> const& p_rgb, float p_alpha)
{
    return ImGui::GetColorU32(ImVec4(p_rgb[0], p_rgb[1], p_rgb[2], p_alpha));
}

//! \brief A dotted line, dots of 4 points every 8.
static void
dotted(ImDrawList* p_draw, ImVec2 p_from, ImVec2 p_to, ImU32 p_colour)
{
    const float dx = p_to.x - p_from.x;
    const float dy = p_to.y - p_from.y;
    const float length = std::hypot(dx, dy);
    for (float s = 0.0f; s < length; s += 8.0f)
    {
        const float e = std::min(s + 4.0f, length);
        p_draw->AddLine(
            ImVec2(p_from.x + (dx * s / length), p_from.y + (dy * s / length)),
            ImVec2(p_from.x + (dx * e / length), p_from.y + (dy * e / length)),
            p_colour,
            1.5f);
    }
}

//! \brief A rounded sign: its background, its tab on the left, its text.
static void drawSign(ImDrawList* p_draw,
                     ImVec2 p_min,
                     ImVec2 p_max,
                     char const* p_text,
                     float p_size,
                     ImU32 p_back,
                     ImU32 p_tab,
                     ImU32 p_text_colour)
{
    const float rounding = 0.5f * (p_max.y - p_min.y);
    p_draw->AddRectFilled(p_min, p_max, p_back, rounding);
    p_draw->AddRectFilled(p_min,
                          ImVec2(p_min.x + TAB, p_max.y),
                          p_tab,
                          rounding,
                          ImDrawFlags_RoundCornersLeft);
    p_draw->AddText(ImGui::GetFont(),
                    p_size,
                    ImVec2(p_min.x + TAB + PADDING,
                           p_min.y + 0.5f * (p_max.y - p_min.y - p_size)),
                    p_text_colour,
                    p_text);
}

// ----------------------------------------------------------------------------
std::optional<std::pair<float, float>>
Signs::project(Vector3f const& p_point) const
{
    const compages::Vector4f clip =
        m_camera->view_projection * compages::Vector4f(p_point, 1.0f);
    if (clip.w <= 0.0f)
    {
        return std::nullopt; // behind the camera
    }
    const float x = clip.x / clip.w;
    const float y = clip.y / clip.w;
    if ((std::abs(x) > 1.2f) || (std::abs(y) > 1.2f))
    {
        return std::nullopt;
    }
    return std::pair{ m_view.x + (0.5f * (x + 1.0f) * m_view.width),
                      m_view.y + (0.5f * (1.0f - y) * m_view.height) };
}

// ----------------------------------------------------------------------------
bool Signs::reserve(Box const& p_box, bool p_force)
{
    const bool free = std::ranges::none_of(m_taken,
                                           [&p_box](Box const& p_b)
                                           {
                                               return (p_box.x0 < p_b.x1) &&
                                                      (p_b.x0 < p_box.x1) &&
                                                      (p_box.y0 < p_b.y1) &&
                                                      (p_b.y0 < p_box.y1);
                                           });
    if (free || p_force)
    {
        m_taken.push_back(p_box);
    }
    return free;
}

// ----------------------------------------------------------------------------
//! The stem goes up from the point; if the sign would cover another one, it
//! goes higher (twice), else the sign is left out.
// ----------------------------------------------------------------------------
void Signs::sign(Vector3f const& p_point,
                 std::string const& p_text,
                 Style const& p_style)
{
    const auto at = project(p_point);
    if (!at)
    {
        return;
    }
    const auto [x, y] = *at;
    const ImVec2 text = ImGui::GetFont()->CalcTextSizeA(
        p_style.size, 1.0e6f, 0.0f, p_text.c_str());
    const float width = TAB + (2.0f * PADDING) + text.x;
    const float height = p_style.size + 10.0f;
    for (int raise = 0; raise < 3; ++raise)
    {
        const float bottom =
            y - p_style.stem - (float(raise) * (height + 4.0f));
        const Box box{ x - 8.0f, bottom - height, x - 8.0f + width, bottom };
        if (!reserve(box, false))
        {
            continue;
        }
        m_draw->AddLine(ImVec2(x, y), ImVec2(x, bottom), p_style.tab, 1.5f);
        m_draw->AddCircle(ImVec2(x, y), 3.0f, p_style.tab, 12, 1.5f);
        drawSign(m_draw,
                 ImVec2(box.x0, box.y0),
                 ImVec2(box.x1, box.y1),
                 p_text.c_str(),
                 p_style.size,
                 p_style.back,
                 p_style.tab,
                 p_style.text);
        return;
    }
}

// ----------------------------------------------------------------------------
//! The pins on the screen, their middle; the circle wide enough for the pins
//! and for the names (stacked on its two sides); each pin goes to the slot
//! of the circle nearest its own direction, in turn round the circle.
// ----------------------------------------------------------------------------
void Signs::circle(std::size_t p_region, int p_marked)
{
    Region const& region = m_land->regions[p_region];
    Topic const& topic = m_knowledge->topics[p_region];
    struct Pin
    {
        std::size_t link;
        ImVec2 at;
        float angle;
    };
    std::vector<Pin> pins;
    for (std::size_t k = 0u;
         (k < region.parts.size()) && (pins.size() < MOST_LINKS);
         ++k)
    {
        const Vector2f site = region.parts[k].site;
        const Vector3f head(site.x,
                            std::max(0.0f, m_land->heightAt(site.x, site.y)) +
                                PIN_HEAD,
                            site.y);
        if (const auto at = project(head))
        {
            pins.push_back(Pin{ k, ImVec2(at->first, at->second), 0.0f });
        }
    }
    if (pins.empty())
    {
        return;
    }
    ImVec2 middle(0.0f, 0.0f);
    for (Pin const& pin : pins)
    {
        middle = ImVec2(middle.x + (pin.at.x / float(pins.size())),
                        middle.y + (pin.at.y / float(pins.size())));
    }
    const float size = ImGui::GetFontSize() * 0.95f;
    const float height = size + 8.0f;
    float radius = 60.0f;
    for (Pin& pin : pins)
    {
        pin.angle = std::atan2(pin.at.y - middle.y, pin.at.x - middle.x);
        radius = std::max(radius,
                          std::hypot(pin.at.x - middle.x, pin.at.y - middle.y) +
                              40.0f);
    }
    const auto count = float(pins.size());
    radius = std::max(radius, count * (height + 4.0f) / 3.5f);
    std::ranges::sort(pins, {}, &Pin::angle);
    // The turn of the slots that fits the directions best (their circular
    // mean difference).
    float sine = 0.0f;
    float cosine = 0.0f;
    for (std::size_t i = 0u; i < pins.size(); ++i)
    {
        const float d = pins[i].angle - (TURN * float(i) / count);
        sine += std::sin(d);
        cosine += std::cos(d);
    }
    const float offset = std::atan2(sine, cosine);
    std::array<float, 3> const& hue = m_knowledge->themes[topic.theme].hue;
    m_draw->AddCircle(middle, radius, colour(hue, 0.35f), 96, 1.5f);
    for (std::size_t i = 0u; i < pins.size(); ++i)
    {
        Pin const& pin = pins[i];
        const float angle = offset + (TURN * float(i) / count);
        const ImVec2 slot(middle.x + (radius * std::cos(angle)),
                          middle.y + (radius * std::sin(angle)));
        const std::string name = shortTitle(topic.pages[pin.link].title);
        const float width =
            TAB + (2.0f * PADDING) +
            ImGui::GetFont()->CalcTextSizeA(size, 1.0e6f, 0.0f, name.c_str()).x;
        // Away from the middle: on the right of the slot or on its left.
        const float x0 = (std::cos(angle) >= 0.0f) ? (slot.x + 4.0f)
                                                   : (slot.x - 4.0f - width);
        const Box box{
            x0, slot.y - (0.5f * height), x0 + width, slot.y + (0.5f * height)
        };
        (void)reserve(box, true); // the names round a region always show
        dotted(m_draw, pin.at, slot, colour({ 1.0f, 0.6f, 0.15f }, 0.9f));
        m_draw->AddCircleFilled(slot, 2.5f, colour(hue, 1.0f));
        const bool marked = (int(pin.link) == p_marked);
        drawSign(m_draw,
                 ImVec2(box.x0, box.y0),
                 ImVec2(box.x1, box.y1),
                 name.c_str(),
                 size,
                 marked ? IM_COL32(255, 236, 170, 240)
                        : IM_COL32(232, 234, 236, 225),
                 colour(hue, 1.0f),
                 IM_COL32(60, 140, 210, 255));
    }
    if (topic.pages.size() > pins.size())
    {
        const std::string more =
            std::format("+ {} more", topic.pages.size() - pins.size());
        m_draw->AddText(ImVec2(middle.x - 30.0f, middle.y + radius + 8.0f),
                        IM_COL32(255, 255, 255, 220),
                        more.c_str());
    }
}

// ----------------------------------------------------------------------------
void Signs::draw(Example::Overlay const& p_view,
                 compages::renderer::CameraFrame const& p_camera,
                 Land const& p_land,
                 Knowledge const& p_knowledge,
                 Place p_hovered,
                 Place p_chosen,
                 bool p_names)
{
    if ((p_camera.viewport_width == 0u) ||
        (p_land.regions.size() != p_knowledge.topics.size()))
    {
        return;
    }
    m_view = p_view;
    m_camera = &p_camera;
    m_land = &p_land;
    m_knowledge = &p_knowledge;
    m_taken.clear();
    // On the picture, under the panels (Example::overlay).
    m_draw = ImGui::GetWindowDrawList();
    m_draw->PushClipRect(
        ImVec2(p_view.x, p_view.y),
        ImVec2(p_view.x + p_view.width, p_view.y + p_view.height));
    const Vector3f eye = p_camera.position;
    const float font = ImGui::GetFontSize();

    // The islands, from afar.
    for (std::size_t k = 0u; p_names && (k < p_land.islands.size()); ++k)
    {
        const Vector3f peak = p_land.islands[k].peak;
        if (compages::vector::norm(peak - eye) > ISLAND_NAMES_BEYOND)
        {
            sign(peak,
                 p_knowledge.themes[k].name,
                 Style{ font * 1.25f,
                        IM_COL32(245, 247, 250, 235),
                        IM_COL32(60, 140, 210, 255),
                        colour(p_knowledge.themes[k].hue, 1.0f),
                        70.0f });
        }
    }
    // The links round the region under the mouse and round the chosen one.
    if (p_hovered.region >= 0)
    {
        circle(std::size_t(p_hovered.region), p_hovered.part);
    }
    if ((p_chosen.region >= 0) && (p_chosen.region != p_hovered.region))
    {
        circle(std::size_t(p_chosen.region), p_chosen.part);
    }
    // The regions, the nearest first.
    std::vector<std::size_t> regions(p_land.regions.size());
    std::iota(regions.begin(), regions.end(), std::size_t(0));
    auto groundOf = [&p_land](std::size_t p_r)
    {
        const Vector2f site = p_land.regions[p_r].site;
        return Vector3f(
            site.x, std::max(0.0f, p_land.heightAt(site.x, site.y)), site.y);
    };
    std::vector<float> distances(regions.size());
    for (std::size_t r = 0u; r < regions.size(); ++r)
    {
        distances[r] = compages::vector::norm(groundOf(r) - eye);
    }
    std::ranges::sort(
        regions, {}, [&distances](std::size_t p_r) { return distances[p_r]; });
    for (const std::size_t r : regions)
    {
        if (!p_names || (distances[r] > REGION_NAMES_WITHIN))
        {
            break;
        }
        Topic const& topic = p_knowledge.topics[r];
        sign(groundOf(r),
             topic.name,
             Style{ font,
                    IM_COL32(232, 234, 236, 225),
                    IM_COL32(60, 140, 210, 255),
                    colour(p_knowledge.themes[topic.theme].hue, 1.0f),
                    36.0f });
    }
    m_draw->PopClipRect();
}

} // namespace examples::archipelago
