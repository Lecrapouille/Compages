//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

// ****************************************************************************
//! \file
//! \brief The panels of the gallery, drawn with Dear ImGui.
//!
//! Nothing here touches the device: the panels read what Gallery.cpp keeps and
//! flip its switches. The one exception is the Viewport panel, which sizes the
//! texture the example is drawn into before showing it.
// ****************************************************************************

#include "Common/Gallery.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <string>

namespace examples
{

//! \brief Names of the panels, which are also the keys of their saved layout.
constexpr char const* VIEWPORT = "Viewport";
constexpr char const* EXAMPLES = "Examples";
constexpr char const* INSPECTOR = "Inspector";
constexpr char const* TRY_IT = "Try it";
constexpr char const* SOURCE = "Source";
constexpr char const* DEBUG = "Debug";
constexpr char const* CONSOLE = "Console";

const ImVec4 RED(1.0f, 0.42f, 0.42f, 1.0f);
const ImVec4 ORANGE(1.0f, 0.70f, 0.30f, 1.0f);
const ImVec4 GREEN(0.45f, 0.85f, 0.45f, 1.0f);
const ImVec4 GREY(0.45f, 0.48f, 0.52f, 1.0f);
const ImVec4 COMMENT(0.50f, 0.72f, 0.50f, 1.0f);

//! \brief What the Console panel shows, kept between frames.
struct ConsoleView
{
    bool info = true;
    bool warnings = true;
    bool errors = true;
    bool this_example_only = false;
    ImGuiTextFilter filter;
};

static ConsoleView& consoleView()
{
    static ConsoleView view;
    return view;
}

static ImGuiTextFilter& examplesFilter()
{
    static ImGuiTextFilter filter;
    return filter;
}

//! \brief The number of this thing the current example added.
static std::size_t owed(std::size_t p_now, std::size_t p_before)
{
    return (p_now > p_before) ? (p_now - p_before) : 0u;
}

//! \brief A text input bound to a filter, with a hint while it is empty.
static void drawFilter(char const* p_id, char const* p_hint, ImGuiTextFilter& p_filter)
{
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputTextWithHint(p_id, p_hint, p_filter.InputBuf,
                                 IM_ARRAYSIZE(p_filter.InputBuf)))
    {
        p_filter.Build();
    }
}

//! \brief A coloured dot, the size of a line of text.
static void dot(ImVec4 const& p_color)
{
    const float size = ImGui::GetTextLineHeight();
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(
        ImVec2(at.x + size * 0.5f, at.y + size * 0.5f + 1.0f), size * 0.22f,
        ImGui::GetColorU32(p_color));
    ImGui::Dummy(ImVec2(size, size));
    ImGui::SameLine(0.0f, 2.0f);
}

//! \brief Is this line of C++ a comment, once its indentation is skipped?
static bool isComment(std::string const& p_line)
{
    const std::size_t first = p_line.find_first_not_of(" \t");
    return (first != std::string::npos) &&
           ((p_line.compare(first, 2u, "//") == 0) ||
            (p_line.compare(first, 2u, "/*") == 0) ||
            (p_line.compare(first, 1u, "*") == 0));
}

//------------------------------------------------------------------------------
void Gallery::drawUi()
{
    drawMenuBar();
    drawStatusBar();
    drawDockSpace();
    drawExamplesPanel();
    drawViewport();
    drawInspectorPanel();
    drawTryItPanel();
    drawSourcePanel();
    drawDebugPanel();
    drawConsolePanel();
}

//------------------------------------------------------------------------------
void Gallery::drawMenuBar()
{
    if (!ImGui::BeginMainMenuBar())
    {
        return;
    }
    if (ImGui::BeginMenu("File"))
    {
        if (ImGui::MenuItem("Screenshot of the example", "F12"))
        {
            m_screenshot_asked = true;
        }
        if (ImGui::MenuItem("Screenshot of the window", "Shift+F12"))
        {
            m_window_screenshot_asked = true;
        }
        ImGui::Separator();
        if (ImGui::MenuItem("Quit", "Esc"))
        {
            m_window.close();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("View"))
    {
        if (ImGui::MenuItem("Hide the panels", "F1"))
        {
            m_show_overlay = false;
        }
        if (ImGui::MenuItem("Reset the layout"))
        {
            m_reset_layout = true;
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Examples"))
    {
        if (ImGui::MenuItem("Previous", "PgUp"))
        {
            show((m_index + m_entries.size() - 1u) % m_entries.size());
        }
        if (ImGui::MenuItem("Next", "PgDn"))
        {
            show((m_index + 1u) % m_entries.size());
        }
        if (ImGui::MenuItem("Restart", "F5"))
        {
            show(m_index);
        }
        ImGui::Separator();
        std::string chapter;
        bool open = false;
        for (std::size_t i = 0u; i < m_entries.size(); ++i)
        {
            if (m_entries[i].chapter != chapter)
            {
                if (open)
                {
                    ImGui::EndMenu();
                }
                chapter = m_entries[i].chapter;
                open = ImGui::BeginMenu(chapter.c_str());
            }
            if (open && ImGui::MenuItem(m_entries[i].title.c_str(), nullptr, i == m_index))
            {
                show(i);
            }
        }
        if (open)
        {
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Debug"))
    {
        ImGui::MenuItem("Pause", "F6", &m_debug.paused);
        if (ImGui::MenuItem("One frame", "F7"))
        {
            m_debug.paused = true;
            m_debug.step = true;
        }
        bool wireframe = compages::gpu::wireframeShown();
        if (ImGui::MenuItem("Wireframe", "F2", &wireframe))
        {
            compages::gpu::showWireframe(wireframe);
        }
        bool vsync = m_window.vsync();
        if (ImGui::MenuItem("Wait for the screen", nullptr, &vsync))
        {
            m_window.vsync(vsync);
        }
        ImGui::Separator();
        ImGui::MenuItem("Stop at the first error", nullptr, &m_debug.stop_on_error);
        bool breaks = compages::gpu::breakOnError();
        if (ImGui::MenuItem("Break into the debugger on error", nullptr, &breaks))
        {
            compages::gpu::setBreakOnError(breaks);
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Help"))
    {
        ImGui::TextDisabled("PgUp / PgDn   previous / next example");
        ImGui::TextDisabled("Left / Right  same, when the viewport has focus");
        ImGui::TextDisabled("F1            show / hide the panels");
        ImGui::TextDisabled("F2            wireframe");
        ImGui::TextDisabled("F5            restart the example");
        ImGui::TextDisabled("F6 / F7       pause / one frame");
        ImGui::TextDisabled("F12           screenshot into screenshots/");
        ImGui::TextDisabled("Shift+F12     same, panels included");
        ImGui::TextDisabled("Esc           quit");
        if (compages::gpu::initialized() && ImGui::BeginMenu("Driver"))
        {
            compages::gpu::DeviceInfo const& device = compages::gpu::device();
            ImGui::TextUnformatted(device.renderer.c_str());
            ImGui::TextDisabled("%s", device.vendor.c_str());
            ImGui::Text("OpenGL %s", device.version.c_str());
            ImGui::Text("GLSL %s", device.shading_language_version.c_str());
            ImGui::Text("Debug output %s", device.debug_output ? "on" : "off");
            ImGui::Text("Texture size up to %d", device.max_texture_size);
            ImGui::Text("Texture units %d  Vertex attributes %d", device.max_texture_units,
                        device.max_vertex_attributes);
            ImGui::EndMenu();
        }
        ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
}

//------------------------------------------------------------------------------
void Gallery::drawStatusBar()
{
    // A side bar of the main viewport rather than a window: the dock space is
    // laid out in what it leaves, so the bar never covers a panel.
    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_MenuBar;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    if (ImGui::BeginViewportSideBar("##GalleryStatusBar", viewport, ImGuiDir_Down,
                                    ImGui::GetFrameHeight(), flags))
    {
        if (ImGui::BeginMenuBar())
        {
            if (m_index < m_entries.size())
            {
                ImGui::TextUnformatted(m_entries[m_index].title.c_str());
            }
            ImGui::Separator();
            ImGui::Text("%.0f fps  %.2f ms", static_cast<double>(m_window.framesPerSecond()),
                        static_cast<double>(m_window.elapsed()) * 1000.0);
            ImGui::Separator();
            ImGui::Text("%zu draws", m_frame_statistics.draw_calls);
            if (m_debug.paused)
            {
                ImGui::Separator();
                ImGui::TextColored(ORANGE, "PAUSED");
            }
            if (compages::gpu::wireframeShown())
            {
                ImGui::Separator();
                ImGui::TextColored(ORANGE, "WIREFRAME");
            }
            const std::size_t errors = m_console.count(compages::gpu::LogLevel::Error);
            if (errors != 0u)
            {
                ImGui::Separator();
                ImGui::TextColored(RED, "%zu errors", errors);
            }
            ImGui::Separator();
            ImGui::TextDisabled("F1 panels  PgUp/PgDn examples  F5 restart  "
                                "F6 pause  F7 step  F12 screenshot");
            ImGui::EndMenuBar();
        }
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawDockSpace()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    const ImGuiID dock = ImGui::GetID("GalleryDockSpace");

    // The layout of a first run, and of "Reset the layout". Any other run
    // finds the one the user arranged in gallery.ini, unless it was saved
    // before the "Try it" panel existed.
    if ((ImGui::DockBuilderGetNode(dock) != nullptr) &&
        (ImGui::FindWindowSettingsByID(ImHashStr(TRY_IT)) == nullptr))
    {
        m_reset_layout = true;
    }
    if (m_reset_layout || (ImGui::DockBuilderGetNode(dock) == nullptr))
    {
        m_reset_layout = false;
        ImGui::DockBuilderRemoveNode(dock);
        ImGui::DockBuilderAddNode(dock, ImGuiDockNodeFlags_DockSpace);
        ImGui::DockBuilderSetNodeSize(dock, viewport->WorkSize);

        ImGuiID center = dock;
        const ImGuiID left =
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.18f, nullptr, &center);
        const ImGuiID right =
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.30f, nullptr, &center);
        const ImGuiID bottom =
            ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.22f, nullptr, &center);
        ImGuiID above = right;
        const ImGuiID below =
            ImGui::DockBuilderSplitNode(above, ImGuiDir_Down, 0.40f, nullptr, &above);

        ImGui::DockBuilderDockWindow(EXAMPLES, left);
        ImGui::DockBuilderDockWindow(INSPECTOR, above);
        ImGui::DockBuilderDockWindow(SOURCE, above);
        ImGui::DockBuilderDockWindow(DEBUG, above);
        ImGui::DockBuilderDockWindow(TRY_IT, below);
        ImGui::DockBuilderDockWindow(CONSOLE, bottom);
        ImGui::DockBuilderDockWindow(VIEWPORT, center);
        ImGui::DockBuilderFinish(dock);
    }
    ImGui::DockSpaceOverViewport(dock, viewport);
}

//------------------------------------------------------------------------------
void Gallery::drawViewport()
{
    // No tab while it is alone in its place, no padding: the picture fills
    // the panel.
    ImGuiWindowClass unadorned;
    unadorned.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_AutoHideTabBar;
    ImGui::SetNextWindowClass(&unadorned);
    if (m_focus_view)
    {
        m_focus_view = false;
        ImGui::SetNextWindowFocus();
    }
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    const bool visible = ImGui::Begin(VIEWPORT, nullptr,
                                      ImGuiWindowFlags_NoScrollbar |
                                          ImGuiWindowFlags_NoScrollWithMouse |
                                          ImGuiWindowFlags_NoCollapse);
    ImGui::PopStyleVar();

    m_view_hovered = false;
    m_view_focused = false;
    if (!visible)
    {
        m_view_dragged = false;
        ImGui::End();
        return;
    }

    // The texture is as large as the panel, in pixels of the screen rather
    // than in the points ImGui lays out with.
    ImGuiIO const& io = ImGui::GetIO();
    const ImVec2 size(std::max(ImGui::GetContentRegionAvail().x, 1.0f),
                      std::max(ImGui::GetContentRegionAvail().y, 1.0f));
    const ImVec2 scale = io.DisplayFramebufferScale;
    resizeView(static_cast<std::uint32_t>(size.x * scale.x),
               static_cast<std::uint32_t>(size.y * scale.y));

    // An invisible button over the picture takes the clicks, so that a drag in
    // the example does not move the panel.
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##view", size,
                           ImGuiButtonFlags_MouseButtonLeft |
                               ImGuiButtonFlags_MouseButtonRight |
                               ImGuiButtonFlags_MouseButtonMiddle);
    m_view_hovered = ImGui::IsItemHovered();
    m_view_dragged = ImGui::IsItemActive();
    m_view_focused = ImGui::IsWindowFocused();
    if ((m_current != nullptr) && m_current->capturesMouse() && !m_mouse_captured &&
        ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
    {
        captureMouse(true);
    }

    const ImVec2 end(at.x + size.x, at.y + size.y);
    ImDrawList* draw = ImGui::GetWindowDrawList();
    if (m_view_width != 0u)
    {
        // Upside down: the device puts the first row at the bottom.
        draw->AddImage(ImTextureRef(static_cast<ImTextureID>(m_view_color.nativeId())),
                       at, end, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));
    }
    m_view_mouse = Vector2f((io.MousePos.x - at.x) * scale.x,
                            static_cast<float>(m_view_height) -
                                ((io.MousePos.y - at.y) * scale.y));

    // Why nothing is drawn, where one looks for the drawing.
    if ((m_current == nullptr) && !m_failure.empty())
    {
        ImGui::SetCursorScreenPos(ImVec2(at.x + 16.0f, at.y + 16.0f));
        ImGui::PushTextWrapPos(end.x - 16.0f);
        ImGui::TextColored(RED, "Not running: %s", m_failure.c_str());
        ImGui::TextDisabled("F5 tries again once the code is fixed.");
        ImGui::PopTextWrapPos();
    }
    else if (m_debug.paused)
    {
        ImGui::SetCursorScreenPos(ImVec2(at.x + 10.0f, at.y + 8.0f));
        ImGui::TextColored(ORANGE, "PAUSED  (F6 resumes, F7 one frame)");
    }
    drawHud(at.x, at.y, size.x, size.y);
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawHud(float p_x, float p_y, float p_width, float p_height)
{
    if (m_current == nullptr)
    {
        return;
    }
    const std::string text = m_current->hud();
    if (text.empty())
    {
        return;
    }

    // On top of everything, so that it is seen whether the panels are shown
    // or not. A shadow under each line keeps it readable on a bright picture.
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    ImFont* font = ImGui::GetFont();
    const float small = ImGui::GetFontSize() * 1.25f;
    const float large = ImGui::GetFontSize() * 3.0f;
    const ImU32 shadow = IM_COL32(0, 0, 0, 200);
    const ImU32 white = IM_COL32(255, 245, 225, 255);
    float small_y = p_y + 10.0f;
    float large_y = p_y + (p_height * 0.35f);

    std::size_t begin = 0u;
    while (begin <= text.size())
    {
        std::size_t end = text.find('\n', begin);
        if (end == std::string::npos)
        {
            end = text.size();
        }
        const std::string line = text.substr(begin, end - begin);
        begin = end + 1u;

        const bool big = !line.empty() && (line.front() == '!');
        char const* words = line.c_str() + (big ? 1 : 0);
        const float height = big ? large : small;
        ImVec2 place(p_x + 12.0f, small_y);
        if (big)
        {
            const ImVec2 extent = font->CalcTextSizeA(height, p_width, 0.0f, words);
            place = ImVec2(p_x + ((p_width - extent.x) * 0.5f), large_y);
            large_y += height * 1.1f;
        }
        else
        {
            small_y += height * 1.15f;
        }
        draw->AddText(font, height, ImVec2(place.x + 2.0f, place.y + 2.0f), shadow, words);
        draw->AddText(font, height, place, white, words);
    }
}

//------------------------------------------------------------------------------
void Gallery::drawExamplesPanel()
{
    if (!ImGui::Begin(EXAMPLES))
    {
        ImGui::End();
        return;
    }

    ImGuiTextFilter& filter = examplesFilter();
    drawFilter("##examples", "Filter...", filter);

    std::string chapter;
    bool open = false;
    for (std::size_t i = 0u; i < m_entries.size(); ++i)
    {
        Entry const& entry = m_entries[i];
        if (!filter.PassFilter(entry.name.c_str()))
        {
            continue;
        }
        if (entry.chapter != chapter)
        {
            if (open)
            {
                ImGui::TreePop();
            }
            chapter = entry.chapter;
            open = ImGui::TreeNodeEx(chapter.c_str(), ImGuiTreeNodeFlags_DefaultOpen |
                                                          ImGuiTreeNodeFlags_SpanAvailWidth);
        }
        if (!open)
        {
            continue;
        }

        ImGui::PushID(static_cast<int>(i));
        switch (entry.outcome)
        {
            case Outcome::NotRun: dot(GREY); break;
            case Outcome::Fine: dot(GREEN); break;
            case Outcome::Failed: dot(RED); break;
            case Outcome::Leaked: dot(ORANGE); break;
        }
        if (ImGui::Selectable(entry.title.c_str(), i == m_index))
        {
            show(i);
        }
        if (!entry.problem.empty() && ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("%s", entry.problem.c_str());
        }
        ImGui::PopID();
    }
    if (open)
    {
        ImGui::TreePop();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawInspectorPanel()
{
    if (!ImGui::Begin(INSPECTOR))
    {
        ImGui::End();
        return;
    }
    if (m_index < m_entries.size())
    {
        Entry const& entry = m_entries[m_index];
        ImGui::TextUnformatted(entry.title.c_str());
        ImGui::TextDisabled("%s  -  examples/%s", entry.chapter.c_str(), entry.source.c_str());
        ImGui::Separator();
    }

    if (m_current != nullptr)
    {
        ImGui::TextWrapped("%s", m_current->description().c_str());
    }
    if (!m_failure.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, RED);
        ImGui::TextWrapped("Not running: %s", m_failure.c_str());
        ImGui::PopStyleColor();
    }
    if (!m_leak.empty())
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ORANGE);
        ImGui::TextWrapped("The previous example left %s behind.", m_leak.c_str());
        ImGui::PopStyleColor();
    }

    ImGui::SeparatorText("Frame");
    ImGui::Text("%.1f fps  %.2f ms  time %.2f s",
                static_cast<double>(m_window.framesPerSecond()),
                static_cast<double>(m_window.elapsed()) * 1000.0,
                static_cast<double>(m_example_time));
    ImGui::Text("Viewport %u x %u", m_view_width, m_view_height);
    ImGui::Text("Draws %zu  Passes %zu  Dispatches %zu", m_frame_statistics.draw_calls,
                m_frame_statistics.passes, m_frame_statistics.dispatches);
    ImGui::Text("Vertices %zu  Instances %zu", m_frame_statistics.vertices,
                m_frame_statistics.instances);

    ImGui::SeparatorText("Device, for this example");
    const compages::gpu::ResourceStatistics now = compages::gpu::resourceStatistics();
    ImGui::Text("Buffers %zu  Textures %zu  Framebuffers %zu",
                owed(now.buffers, m_before.buffers), owed(now.textures, m_before.textures),
                owed(now.framebuffers, m_before.framebuffers));
    ImGui::Text("Programs %zu  Pipelines %zu  Readers %zu",
                owed(now.programs, m_before.programs), owed(now.pipelines, m_before.pipelines),
                owed(now.vertex_readers, m_before.vertex_readers));
    ImGui::Text("Buffer memory %zu KiB  Texture memory %zu KiB",
                owed(now.buffer_bytes, m_before.buffer_bytes) / 1024u,
                owed(now.texture_bytes, m_before.texture_bytes) / 1024u);
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawTryItPanel()
{
    if (!ImGui::Begin(TRY_IT))
    {
        ImGui::End();
        return;
    }
    if (m_current != nullptr)
    {
        // Nothing drawn by the example leaves the cursor where it was.
        const float before = ImGui::GetCursorPosY();
        ImGui::PushID("controls");
        m_current->controls();
        ImGui::PopID();
        if (ImGui::GetCursorPosY() == before)
        {
            ImGui::TextDisabled("Nothing to change in this example.");
        }
    }
    if (m_mouse_captured)
    {
        ImGui::Separator();
        ImGui::TextColored(ORANGE, "The mouse is held by the example: Escape gives it back.");
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawSourcePanel()
{
    if (!ImGui::Begin(SOURCE))
    {
        ImGui::End();
        return;
    }
    if (m_index >= m_entries.size())
    {
        ImGui::End();
        return;
    }

    // The .cpp the manifest names and the header next to it.
    std::string const& cpp = m_entries[m_index].source;
    const std::string hpp = cpp.substr(0u, cpp.rfind('.')) + ".hpp";
    if (ImGui::BeginTabBar("##files"))
    {
        for (std::string const* file : { &cpp, &hpp })
        {
            const std::string label = file->substr(file->rfind('.'));
            if (!ImGui::BeginTabItem(label.c_str()))
            {
                continue;
            }
            SourceFile const& source = sourceFile(*file);
            std::vector<std::string> const& lines = source.lines;
            ImGui::TextDisabled("examples/%s", file->c_str());
            ImGui::BeginChild("##code", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                              ImGuiWindowFlags_HorizontalScrollbar);
            ImGuiListClipper clipper;
            clipper.Begin(static_cast<int>(lines.size()));
            while (clipper.Step())
            {
                for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
                {
                    std::string const& line = lines[static_cast<std::size_t>(i)];
                    ImGui::TextDisabled("%4zu", source.first + static_cast<std::size_t>(i));
                    ImGui::SameLine();
                    if (isComment(line))
                    {
                        ImGui::TextColored(COMMENT, "%s", line.c_str());
                    }
                    else
                    {
                        ImGui::TextUnformatted(line.c_str());
                    }
                }
            }
            ImGui::EndChild();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawDebugPanel()
{
    if (!ImGui::Begin(DEBUG))
    {
        ImGui::End();
        return;
    }

    ImGui::SeparatorText("Time");
    ImGui::Checkbox("Pause (F6)", &m_debug.paused);
    ImGui::SameLine();
    if (ImGui::Button("One frame (F7)"))
    {
        m_debug.paused = true;
        m_debug.step = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Restart (F5)"))
    {
        show(m_index);
    }
    ImGui::SliderFloat("Speed", &m_debug.speed, 0.1f, 4.0f, "%.2fx",
                       ImGuiSliderFlags_Logarithmic);

    ImGui::SeparatorText("Drawing");
    bool wireframe = compages::gpu::wireframeShown();
    if (ImGui::Checkbox("Wireframe (F2)", &wireframe))
    {
        compages::gpu::showWireframe(wireframe);
    }
    ImGui::SetItemTooltip("Every triangle drawn as its edges, whatever the example asks for.");
    bool vsync = m_window.vsync();
    if (ImGui::Checkbox("Wait for the screen", &vsync))
    {
        m_window.vsync(vsync);
    }
    ImGui::SetItemTooltip("Off, frames are drawn as fast as they can be: what they cost shows.");
    if (ImGui::Button("Save a screenshot (F12)"))
    {
        m_screenshot_asked = true;
    }

    ImGui::SeparatorText("Errors");
    ImGui::Checkbox("Stop at the first error", &m_debug.stop_on_error);
    ImGui::SetItemTooltip("Off, each error goes to the Console and the example draws on.");
    bool breaks = compages::gpu::breakOnError();
    if (ImGui::Checkbox("Break into the debugger on error", &breaks))
    {
        compages::gpu::setBreakOnError(breaks);
    }
    ImGui::SetItemTooltip("Asserts where the error is reported, with the guilty call on the stack.");
    bool hints = compages::gpu::driverHintsReported();
    if (ImGui::Checkbox("Driver performance hints", &hints))
    {
        compages::gpu::reportDriverHints(hints);
    }
    ImGui::SetItemTooltip("What the driver says about speed rather than mistakes, such as a "
                          "shader recompiled for the state or a buffer moved to host memory. "
                          "Shown as Info in the Console.");
    ImGui::End();
}

//------------------------------------------------------------------------------
void Gallery::drawConsolePanel()
{
    if (!ImGui::Begin(CONSOLE))
    {
        ImGui::End();
        return;
    }

    ConsoleView& view = consoleView();
    const std::string info = "Info (" + std::to_string(m_console.count(compages::gpu::LogLevel::Info)) + ")";
    const std::string warnings =
        "Warnings (" + std::to_string(m_console.count(compages::gpu::LogLevel::Warning)) + ")";
    const std::string errors =
        "Errors (" + std::to_string(m_console.count(compages::gpu::LogLevel::Error)) + ")";
    ImGui::Checkbox(info.c_str(), &view.info);
    ImGui::SameLine();
    ImGui::Checkbox(warnings.c_str(), &view.warnings);
    ImGui::SameLine();
    ImGui::Checkbox(errors.c_str(), &view.errors);
    ImGui::SameLine();
    ImGui::Checkbox("This example only", &view.this_example_only);
    ImGui::SameLine();
    if (ImGui::Button("Clear"))
    {
        m_console.clear();
    }
    drawFilter("##console", "Filter...", view.filter);

    const std::string current =
        (m_index < m_entries.size()) ? m_entries[m_index].title : std::string();
    ImGui::BeginChild("##messages", ImVec2(0.0f, 0.0f), ImGuiChildFlags_None,
                      ImGuiWindowFlags_HorizontalScrollbar);
    for (Console::Message const& message : m_console.messages())
    {
        const bool shown = (message.level == compages::gpu::LogLevel::Info)      ? view.info
                           : (message.level == compages::gpu::LogLevel::Warning) ? view.warnings
                                                                       : view.errors;
        if (!shown || (view.this_example_only && (message.example != current)) ||
            !view.filter.PassFilter(message.text.c_str()))
        {
            continue;
        }
        const ImVec4 color = (message.level == compages::gpu::LogLevel::Error)     ? RED
                             : (message.level == compages::gpu::LogLevel::Warning) ? ORANGE
                                                                         : GREY;
        if (message.count > 1u)
        {
            ImGui::TextDisabled("x%zu", message.count);
            ImGui::SameLine();
        }
        if (!message.example.empty())
        {
            ImGui::TextDisabled("%s", message.example.c_str());
            ImGui::SameLine();
        }
        ImGui::TextColored(color, "%s", message.text.c_str());
    }
    // Follow the new messages, unless the user scrolled up to read old ones.
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        ImGui::SetScrollHereY(1.0f);
    }
    ImGui::EndChild();
    ImGui::End();
}

} // namespace examples
