// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "10_Complete/Archipelago/Panels.hpp"
#include "10_Complete/Archipelago/Browser.hpp"
#include "10_Complete/Archipelago/Labels.hpp"

#include "Common/Gui.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <filesystem>
#include <format>

namespace examples::archipelago
{

// The look of IslandedBrowser (src/Settings.hpp).
static const ImVec4 PANEL_COLOR{ 10.0f / 255.0f,
                                 18.0f / 255.0f,
                                 28.0f / 255.0f,
                                 245.0f / 255.0f };
static const ImVec4 PANEL_BORDER_COLOR{ 90.0f / 255.0f,
                                        120.0f / 255.0f,
                                        150.0f / 255.0f,
                                        160.0f / 255.0f };
static const ImVec4 TEXT_COLOR{ 235.0f / 255.0f,
                                235.0f / 255.0f,
                                235.0f / 255.0f,
                                1.0f };
static const ImVec4 DIM_TEXT_COLOR{ 150.0f / 255.0f,
                                    160.0f / 255.0f,
                                    175.0f / 255.0f,
                                    1.0f };
static const ImVec4 URL_COLOR{ 120.0f / 255.0f, 190.0f / 255.0f, 1.0f, 1.0f };
static const ImVec4 SELECTION_COLOR{ 1.0f,
                                     220.0f / 255.0f,
                                     80.0f / 255.0f,
                                     1.0f };
//! \brief Width of the panel of the choice, height of the status bar, the
//! seconds a notification stays.
static constexpr float PANEL_WIDTH = 380.0f;
static constexpr float STATUSBAR_HEIGHT = 26.0f;
static constexpr double TOAST_DURATION = 3.0;

//! \brief The windows over the picture: no title, no move, never saved.
static constexpr ImGuiWindowFlags BOX =
    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoDocking |
    ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;

//! \brief The colour of an island, for the stripes.
static ImVec4 hueOf(Knowledge const& p_knowledge, std::size_t p_theme)
{
    std::array<float, 3> const& hue = p_knowledge.themes[p_theme].hue;
    return { hue[0], hue[1], hue[2], 1.0f };
}

//! \brief A stripe of a colour along the left (or the top) of the window.
static void stripe(ImVec4 p_accent, bool p_top)
{
    const ImVec2 at = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const ImVec2 end = p_top ? ImVec2(at.x + size.x, at.y + 3.0f)
                             : ImVec2(at.x + 3.0f, at.y + size.y);
    ImGui::GetWindowDrawList()->AddRectFilled(
        at, end, ImGui::GetColorU32(p_accent));
}

//! \brief Begin a box over the picture, with a stripe of its colour on its
//! left (vertical) or on its top.
static bool beginBox(char const* p_name,
                     ImVec2 p_at,
                     ImVec2 p_size,
                     ImVec4 p_accent,
                     bool p_inputs,
                     bool p_top)
{
    ImGui::SetNextWindowPos(p_at);
    if (p_size.x > 0.0f)
    {
        ImGui::SetNextWindowSize(p_size);
    }
    ImGui::PushStyleColor(ImGuiCol_WindowBg, PANEL_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Border, PANEL_BORDER_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
    ImGuiWindowFlags flags =
        BOX | ((p_size.x > 0.0f) ? 0 : ImGuiWindowFlags_AlwaysAutoResize);
    if (!p_inputs)
    {
        flags |= ImGuiWindowFlags_NoInputs;
    }
    const bool open = ImGui::Begin(p_name, nullptr, flags);
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    stripe(p_accent, p_top);
    return open;
}

//! \brief A line of text in a colour, wrapped at the width of the window.
static void line(ImVec4 p_colour, std::string const& p_text)
{
    ImGui::PushStyleColor(ImGuiCol_Text, p_colour);
    ImGui::TextWrapped("%s", p_text.c_str());
    ImGui::PopStyleColor();
}

//! \brief A title, larger.
static void title(std::string const& p_text)
{
    ImGui::SetWindowFontScale(1.25f);
    ImGui::TextWrapped("%s", p_text.c_str());
    ImGui::SetWindowFontScale(1.0f);
}

//! \brief Lower case, for a search that ignores the case (ASCII letters).
static std::string lower(std::string p_text)
{
    std::ranges::transform(p_text,
                           p_text.begin(),
                           [](unsigned char p_c)
                           { return char(std::tolower(p_c)); });
    return p_text;
}

//! \brief Was this character typed this frame (layout-dependent keys: / ?)?
static bool typed(ImWchar p_character)
{
    ImGuiIO const& io = ImGui::GetIO();
    return std::ranges::find(io.InputQueueCharacters, p_character) !=
           io.InputQueueCharacters.end();
}

// ----------------------------------------------------------------------------
void Panels::toast(std::string p_message)
{
    m_toast = std::move(p_message);
    m_toast_time = ImGui::GetTime();
}

// ----------------------------------------------------------------------------
void Panels::updateMatches(Knowledge const& p_knowledge)
{
    m_matches.clear();
    m_match = 0u;
    const std::string query = lower(m_query);
    if (query.empty())
    {
        return;
    }
    for (std::size_t r = 0u; r < p_knowledge.topics.size(); ++r)
    {
        Topic const& topic = p_knowledge.topics[r];
        if (lower(topic.name).find(query) != std::string::npos)
        {
            m_matches.push_back(
                Place{ .cell = -1, .region = int(r), .part = -1 });
        }
        for (std::size_t k = 0u; k < topic.pages.size(); ++k)
        {
            Page const& page = topic.pages[k];
            if ((lower(page.title).find(query) != std::string::npos) ||
                (lower(page.url).find(query) != std::string::npos))
            {
                m_matches.push_back(
                    Place{ .cell = -1, .region = int(r), .part = int(k) });
            }
        }
    }
    m_match = m_matches.size(); // none chosen yet
}

// ----------------------------------------------------------------------------
void Panels::nextMatch(int p_direction, Navigator& p_navigator)
{
    const std::size_t count = m_matches.size();
    if (count == 0u)
    {
        return;
    }
    if (m_match >= count)
    {
        m_match = (p_direction > 0) ? 0u : (count - 1u);
    }
    else
    {
        m_match = (m_match + ((p_direction > 0) ? 1u : (count - 1u))) % count;
    }
    p_navigator.select(m_matches[m_match]);
    p_navigator.zoomOn(m_matches[m_match]);
}

// ----------------------------------------------------------------------------
//! The shortcuts of IslandedBrowser, but F (the camera zooms out with it):
//! Home fits the whole map, and B shows the borders (L, its links).
// ----------------------------------------------------------------------------
void Panels::keys(Frame const& p_frame, View& p_options, Navigator& p_navigator)
{
    if (!p_frame.view.keys && !m_focused)
    {
        return;
    }
    ImGuiIO const& io = ImGui::GetIO();
    const Place chosen = p_frame.chosen;
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
    {
        if (m_help)
        {
            m_help = false;
        }
        else if (chosen.region >= 0)
        {
            p_navigator.select(Place{});
        }
    }
    if (ImGui::IsKeyPressed(ImGuiKey_H, false) ||
        ImGui::IsKeyPressed(ImGuiKey_F1, false) || typed('?'))
    {
        m_help = !m_help;
        m_focus_help = m_help;
    }
    if (typed('/') || (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F, false)))
    {
        m_searching = true;
        m_focus_search = true;
        m_help = false;
        updateMatches(p_frame.knowledge);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_O, false))
    {
        p_navigator.open(chosen);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Z, false) && (chosen.region >= 0))
    {
        p_navigator.zoomOn(chosen);
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Home, false))
    {
        p_navigator.home();
    }
    if (ImGui::IsKeyPressed(ImGuiKey_N, false))
    {
        p_options.names = !p_options.names;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_B, false))
    {
        p_options.borders = !p_options.borders;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        p_options.animals = !p_options.animals;
        toast(p_options.animals ? "Animals moving" : "Animals paused");
    }
}

// ----------------------------------------------------------------------------
//! While the search box has the keys: Enter or Down for the next match,
//! Shift + Enter or Up for the previous one, Ctrl + Enter opens it, Esc
//! closes the box.
// ----------------------------------------------------------------------------
void Panels::searchKeys(Frame const& p_frame, Navigator& p_navigator)
{
    ImGuiIO const& io = ImGui::GetIO();
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false))
    {
        m_searching = false;
        m_query.clear();
        m_matches.clear();
        return;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_Enter, false) ||
        ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))
    {
        if (io.KeyCtrl)
        {
            p_navigator.open(p_frame.chosen);
        }
        else
        {
            nextMatch(io.KeyShift ? -1 : 1, p_navigator);
        }
        m_focus_search = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) ||
        ImGui::IsKeyPressed(ImGuiKey_Tab))
    {
        nextMatch(1, p_navigator);
        m_focus_search = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
    {
        nextMatch(-1, p_navigator);
    }
}

// ----------------------------------------------------------------------------
//! Next to the pointer, kept inside the picture: the title, the URL of a
//! link or how many links a region has, where it is, what a click does.
// ----------------------------------------------------------------------------
void Panels::tooltip(Frame const& p_frame) const
{
    const Place hovered = p_frame.hovered;
    if ((hovered.region < 0) || m_help)
    {
        return;
    }
    Topic const& topic = p_frame.knowledge.topics[std::size_t(hovered.region)];
    const std::string island = p_frame.knowledge.themes[topic.theme].name;
    const bool link = (hovered.part >= 0);
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const float width = 380.0f;
    ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f),
                                        ImVec2(width, 400.0f));
    // Below right of the pointer; on its other side near an edge.
    Example::Overlay const& view = p_frame.view;
    ImVec2 at(mouse.x + 18.0f, mouse.y + 20.0f);
    if (at.x + width > view.x + view.width)
    {
        at.x = mouse.x - width - 12.0f;
    }
    if (at.y + 120.0f > view.y + view.height - STATUSBAR_HEIGHT)
    {
        at.y = mouse.y - 132.0f;
    }
    at = ImVec2(std::max(view.x + 4.0f, at.x), std::max(view.y + 4.0f, at.y));
    // A tooltip window of Dear ImGui: always over the other windows.
    ImGui::SetNextWindowPos(at);
    ImGui::PushStyleColor(ImGuiCol_PopupBg, PANEL_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Border, PANEL_BORDER_COLOR);
    ImGui::PushStyleColor(ImGuiCol_Text, TEXT_COLOR);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 4.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 10.0f));
    const bool open = ImGui::BeginTooltip();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(3);
    if (open)
    {
        stripe(hueOf(p_frame.knowledge, topic.theme), false);
        ImGui::PushTextWrapPos(width - 12.0f);
        if (link)
        {
            Page const& page = topic.pages[std::size_t(hovered.part)];
            title(page.title);
            line(URL_COLOR, page.url);
            line(DIM_TEXT_COLOR, std::format("{}  >  {}", island, topic.name));
        }
        else
        {
            title(topic.name);
            line(hueOf(p_frame.knowledge, topic.theme),
                 std::format("Region · {} link{}",
                             topic.pages.size(),
                             (topic.pages.size() > 1u) ? "s" : ""));
            line(DIM_TEXT_COLOR, island);
        }
        const bool chosen = (hovered.region == p_frame.chosen.region) &&
                            (hovered.part == p_frame.chosen.part);
        if (link)
        {
            line(DIM_TEXT_COLOR,
                 chosen ? "Double-click or Enter: open"
                        : "Click: details    Double-click: open");
        }
        else
        {
            line(DIM_TEXT_COLOR,
                 chosen ? "Double-click: zoom    Enter: open its links"
                        : "Click: details    Double-click: zoom    "
                          "Middle-click: open");
        }
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// ----------------------------------------------------------------------------
//! On the right: what is chosen, where it is (clickable), its URL or its
//! links (clickable: a click goes there, Ctrl + click opens), what can be
//! done.
// ----------------------------------------------------------------------------
void Panels::details(Frame const& p_frame, Navigator& p_navigator)
{
    const Place chosen = p_frame.chosen;
    if (chosen.region < 0)
    {
        return;
    }
    Knowledge const& knowledge = p_frame.knowledge;
    Topic const& topic = knowledge.topics[std::size_t(chosen.region)];
    const ImVec4 accent = hueOf(knowledge, topic.theme);
    Example::Overlay const& view = p_frame.view;
    const float width = std::min(PANEL_WIDTH, view.width * 0.45f);
    const ImVec2 at(view.x + view.width - width - 12.0f, view.y + 12.0f);
    const ImVec2 size(width, view.height - STATUSBAR_HEIGHT - 24.0f);
    if (beginBox("##details", at, size, accent, true, true))
    {
        m_focused = m_focused || ImGui::IsWindowFocused();
        const bool link = (chosen.part >= 0);
        line(accent, link ? "LINK" : "REGION");
        title(link ? topic.pages[std::size_t(chosen.part)].title : topic.name);
        // Where it is: the island, the region of a link.
        if (ImGui::Selectable(
                std::format("> {}", knowledge.themes[topic.theme].name)
                    .c_str()))
        {
            p_navigator.zoomOnIsland(topic.theme);
        }
        if (link && ImGui::Selectable(
                        std::format("    > {}##region", topic.name).c_str()))
        {
            p_navigator.select(
                Place{ .cell = -1, .region = chosen.region, .part = -1 });
        }
        ImGui::Spacing();
        if (link)
        {
            line(URL_COLOR, topic.pages[std::size_t(chosen.part)].url);
        }
        line(TEXT_COLOR,
             std::format("{} link{}",
                         topic.pages.size(),
                         (topic.pages.size() > 1u) ? "s" : ""));
        ImGui::SeparatorText("Content");
        const float hints = 5.0f * ImGui::GetTextLineHeightWithSpacing();
        if (ImGui::BeginChild("##links", ImVec2(0.0f, -hints)))
        {
            for (std::size_t k = 0u; k < topic.pages.size(); ++k)
            {
                ImGui::PushID(int(k));
                const Place place{ .cell = -1,
                                   .region = chosen.region,
                                   .part = int(k) };
                if (ImGui::Selectable(
                        std::format("- {}", shortTitle(topic.pages[k].title))
                            .c_str(),
                        chosen.part == int(k)))
                {
                    if (ImGui::GetIO().KeyCtrl)
                    {
                        p_navigator.open(place);
                    }
                    else
                    {
                        p_navigator.select(place);
                        p_navigator.zoomOn(place);
                    }
                }
                ImGui::SetItemTooltip("%s", topic.pages[k].url.c_str());
                ImGui::PopID();
            }
        }
        ImGui::EndChild();
        // What can be done.
        const std::size_t tabs =
            std::min(topic.pages.size(), Browser::MAX_TABS);
        line(DIM_TEXT_COLOR,
             link ? std::string("Enter or double-click: open in the browser")
                  : std::format("Enter: open {}{} links in the browser",
                                tabs,
                                (topic.pages.size() > Browser::MAX_TABS)
                                    ? " first"
                                    : ""));
        line(DIM_TEXT_COLOR,
             "Z: zoom on it    Click an entry to go to it    Esc: close");
    }
    ImGui::End();
}

// ----------------------------------------------------------------------------
//! At the top, in the middle: the query, and which match of how many.
// ----------------------------------------------------------------------------
void Panels::search(Frame const& p_frame, Navigator& p_navigator)
{
    if (!m_searching)
    {
        return;
    }
    Example::Overlay const& view = p_frame.view;
    const float width = std::min(520.0f, view.width - 24.0f);
    if (beginBox("##search",
                 ImVec2(view.x + ((view.width - width) * 0.5f), view.y + 12.0f),
                 ImVec2(width, 0.0f),
                 URL_COLOR,
                 true,
                 true))
    {
        m_focused = m_focused || ImGui::IsWindowFocused();
        line(DIM_TEXT_COLOR, "Search:");
        ImGui::SameLine();
        std::string count = "titles and URLs";
        if (!m_query.empty() && m_matches.empty())
        {
            count = "no match";
        }
        else if (m_match < m_matches.size())
        {
            count = std::format("{} / {}", m_match + 1u, m_matches.size());
        }
        else if (!m_matches.empty())
        {
            count = std::format("{} match{}",
                                m_matches.size(),
                                (m_matches.size() > 1u) ? "es" : "");
        }
        ImGui::SetNextItemWidth(width - 120.0f -
                                ImGui::CalcTextSize(count.c_str()).x);
        if (m_focus_search)
        {
            ImGui::SetKeyboardFocusHere();
            m_focus_search = false;
        }
        char buffer[256] = {};
        m_query.copy(buffer, sizeof(buffer) - 1u);
        if (ImGui::InputText("##query", buffer, sizeof(buffer)))
        {
            m_query = buffer;
            updateMatches(p_frame.knowledge);
        }
        if (ImGui::IsItemActive() || ImGui::IsItemDeactivated())
        {
            searchKeys(p_frame, p_navigator);
        }
        ImGui::SameLine();
        line((!m_query.empty() && m_matches.empty())
                 ? ImVec4(1.0f, 0.55f, 0.47f, 1.0f)
                 : DIM_TEXT_COLOR,
             count);
    }
    ImGui::End();
}

// ----------------------------------------------------------------------------
//! At the bottom: what the map holds, the camera, and the keys of the
//! moment.
// ----------------------------------------------------------------------------
void Panels::statusBar(Frame const& p_frame, float p_camera) const
{
    Example::Overlay const& view = p_frame.view;
    ImGui::SetNextWindowBgAlpha(0.88f);
    if (beginBox("##status",
                 ImVec2(view.x, view.y + view.height - STATUSBAR_HEIGHT),
                 ImVec2(view.width, STATUSBAR_HEIGHT),
                 PANEL_COLOR,
                 false,
                 true))
    {
        std::size_t links = 0u;
        for (Topic const& topic : p_frame.knowledge.topics)
        {
            links += topic.pages.size();
        }
        std::string hint = "Click: select    Double-click: open    Drag: move  "
                           "  Wheel: zoom    /: search    H: help";
        if (m_help)
        {
            hint = "H or Esc: close the help";
        }
        else if (m_searching)
        {
            hint = "Type to search    Enter / Down: next    Up: previous    "
                   "Ctrl + "
                   "Enter: open    Esc: close";
        }
        else if (p_frame.chosen.region >= 0)
        {
            hint = "Enter: open    Z: zoom    Esc: unselect    /: search    H: "
                   "help";
        }
        const std::string info = std::format(
            "{}   ·   {} links   ·   {} islands   ·   camera {:.1f} km",
            std::filesystem::path(p_frame.knowledge.source).filename().string(),
            links,
            p_frame.knowledge.themes.size(),
            p_camera / 1000.0f);
        // The keys on the right; what the map holds on the left, when there
        // is room for both.
        const float hint_width = ImGui::CalcTextSize(hint.c_str()).x;
        if (ImGui::CalcTextSize(info.c_str()).x + hint_width + 48.0f <
            view.width)
        {
            ImGui::SetCursorPos(ImVec2(12.0f, 4.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, DIM_TEXT_COLOR);
            ImGui::TextUnformatted(info.c_str());
            ImGui::PopStyleColor();
        }
        ImGui::SetCursorPos(
            ImVec2(std::max(12.0f, view.width - hint_width - 12.0f), 4.0f));
        ImGui::TextUnformatted(hint.c_str());
    }
    ImGui::End();
}

// ----------------------------------------------------------------------------
//! Above the status bar, in the middle; it fades in, stays, fades out.
// ----------------------------------------------------------------------------
void Panels::notification(Frame const& p_frame)
{
    const double age = ImGui::GetTime() - m_toast_time;
    if (m_toast.empty() || (age >= TOAST_DURATION + 0.4))
    {
        m_toast.clear();
        return;
    }
    const auto alpha =
        float(std::min(std::min(age / 0.15, 1.0),
                       std::max(0.0, 1.0 - ((age - TOAST_DURATION) / 0.4))));
    Example::Overlay const& view = p_frame.view;
    const float width = ImGui::CalcTextSize(m_toast.c_str()).x + 32.0f;
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, alpha);
    if (beginBox("##toast",
                 ImVec2(view.x + ((view.width - width) * 0.5f),
                        view.y + view.height - STATUSBAR_HEIGHT - 52.0f),
                 ImVec2(width, 0.0f),
                 URL_COLOR,
                 false,
                 false))
    {
        ImGui::TextUnformatted(m_toast.c_str());
    }
    ImGui::End();
    ImGui::PopStyleVar();
}

// ----------------------------------------------------------------------------
//! In the middle of the darkened picture: the mouse, the keys, the map.
// ----------------------------------------------------------------------------
void Panels::help(Frame const& p_frame)
{
    if (!m_help)
    {
        return;
    }
    Example::Overlay const& view = p_frame.view;
    ImGui::GetForegroundDrawList()->AddRectFilled(
        ImVec2(view.x, view.y),
        ImVec2(view.x + view.width, view.y + view.height),
        IM_COL32(0, 0, 0, 120));
    struct Entry
    {
        char const* key;
        std::string action;
    };
    const std::vector<std::pair<char const*, std::vector<Entry>>> sections = {
        { "Mouse",
          { { "Hover", "the name, the URL; the region is draped" },
            { "Left click", "select: details in the right panel" },
            { "Double click", "open a link, zoom on a region" },
            { "Middle or Ctrl + click",
              std::format("open in the browser (a region: its {} first links)",
                          Browser::MAX_TABS) },
            { "Left drag  /  right drag", "move the map  /  turn" },
            { "Wheel", "zoom toward the pointer" },
            { "Click on the sea", "unselect" } } },
        { "Keyboard",
          { { "/  or  Ctrl + F", "search the titles and URLs" },
            { "Enter  or  O", "open the selection" },
            { "Z  /  Home", "zoom on the selection  /  the whole map" },
            { "W A S D,  Q E,  R F", "move, turn, zoom" },
            { "N  /  B  /  Space",
              "the names  /  the borders  /  the animals" },
            { "Esc", "close, then unselect" } } },
        { "Map",
          { { "Island", "a theme: a group of similar bookmarks" },
            { "Region", "a community, drained by its river" },
            { "Pin", "a bookmark, along the river of its region" } } }
    };
    const float width = std::min(view.width - 24.0f, 700.0f);
    const ImVec2 at(view.x + ((view.width - width) * 0.5f), view.y + 24.0f);
    if (m_focus_help)
    {
        // Over the panel of the choice.
        ImGui::SetNextWindowFocus();
        m_focus_help = false;
    }
    if (beginBox("##help", at, ImVec2(width, 0.0f), URL_COLOR, true, true))
    {
        m_focused = m_focused || ImGui::IsWindowFocused();
        title("Archipelago");
        for (auto const& [name, entries] : sections)
        {
            ImGui::Spacing();
            line(URL_COLOR, name);
            for (Entry const& entry : entries)
            {
                ImGui::PushStyleColor(ImGuiCol_Text, SELECTION_COLOR);
                ImGui::TextUnformatted(entry.key);
                ImGui::PopStyleColor();
                ImGui::SameLine(200.0f);
                ImGui::TextUnformatted(entry.action.c_str());
            }
        }
        ImGui::Spacing();
        line(DIM_TEXT_COLOR, "Press H, Esc or click to close");
        if (ImGui::IsWindowHovered() &&
            ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_help = false;
        }
    }
    ImGui::End();
}

// ----------------------------------------------------------------------------
void Panels::draw(Example::Overlay const& p_view,
                  Knowledge const& p_knowledge,
                  Place p_hovered,
                  Place p_chosen,
                  float p_camera,
                  View& p_options,
                  Navigator& p_navigator)
{
    const Frame frame{ .view = p_view,
                       .knowledge = p_knowledge,
                       .hovered = p_hovered,
                       .chosen = p_chosen };
    if (!m_searching || !ImGui::GetIO().WantTextInput)
    {
        keys(frame, p_options, p_navigator);
    }
    m_focused = false;
    tooltip(frame);
    details(frame, p_navigator);
    search(frame, p_navigator);
    statusBar(frame, p_camera);
    notification(frame);
    help(frame);
}

} // namespace examples::archipelago
