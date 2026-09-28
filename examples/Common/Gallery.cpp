// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Common/Gallery.hpp"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>

#include "Common/ImGuiTheme.hpp"
#include "Common/Screenshot.hpp"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

#if defined(__linux__)
#    include <unistd.h>
#endif

namespace examples
{

//! \brief The gallery the library logs to. There is one per process, as there
//! is one device, and the logger of the library is a plain function.
static Gallery* g_gallery = nullptr;

//! \brief The colour behind everything, and what an example starts from.
const Vector4f BACKGROUND{ 0.1f, 0.1f, 0.12f, 1.0f };

// ----------------------------------------------------------------------------
//! \brief What is left over between two counts, as a sentence, or nothing at
//! all when the two agree.
// ----------------------------------------------------------------------------
static std::string whatStayedBehind(compages::gpu::ResourceStatistics const& p_before,
                                    compages::gpu::ResourceStatistics const& p_after)
{
    std::string left;
    auto note = [&left](char const* p_what,
                        std::size_t p_before_count,
                        std::size_t p_after_count)
    {
        if (p_after_count > p_before_count)
        {
            if (!left.empty())
            {
                left += ", ";
            }
            left +=
                std::to_string(p_after_count - p_before_count) + " " + p_what;
        }
    };

    note("buffers", p_before.buffers, p_after.buffers);
    note("textures", p_before.textures, p_after.textures);
    note("shaders", p_before.shaders, p_after.shaders);
    note("programs", p_before.programs, p_after.programs);
    note("pipelines", p_before.pipelines, p_after.pipelines);
    note("framebuffers", p_before.framebuffers, p_after.framebuffers);
    note("vertex readers", p_before.vertex_readers, p_after.vertex_readers);
    return left;
}

// ----------------------------------------------------------------------------
//! \brief Add to p_counts what changed between p_before and p_after, so that
//! what the gallery itself allocates is not put on the example's account.
// ----------------------------------------------------------------------------
static void carry(compages::gpu::ResourceStatistics& p_counts,
                  compages::gpu::ResourceStatistics const& p_before,
                  compages::gpu::ResourceStatistics const& p_after)
{
    // Unsigned arithmetic wraps, and wraps back: the result is right whichever
    // of the two is larger.
    p_counts.buffers += p_after.buffers - p_before.buffers;
    p_counts.textures += p_after.textures - p_before.textures;
    p_counts.shaders += p_after.shaders - p_before.shaders;
    p_counts.programs += p_after.programs - p_before.programs;
    p_counts.pipelines += p_after.pipelines - p_before.pipelines;
    p_counts.framebuffers += p_after.framebuffers - p_before.framebuffers;
    p_counts.vertex_readers += p_after.vertex_readers - p_before.vertex_readers;
    p_counts.buffer_bytes += p_after.buffer_bytes - p_before.buffer_bytes;
    p_counts.texture_bytes += p_after.texture_bytes - p_before.texture_bytes;
}

// ----------------------------------------------------------------------------
//! \brief "00_GettingStarted/01b_Triangle.cpp" is in chapter "Getting Started",
//! "10_ScientificAndCompute/..." in "Scientific & Compute".
// ----------------------------------------------------------------------------
static std::string chapterOf(std::string const& p_source)
{
    std::string folder = p_source.substr(0u, p_source.find('/'));
    const std::size_t underscore = folder.find('_');
    if (underscore != std::string::npos)
    {
        folder.erase(0u, underscore + 1u);
    }

    std::string words;
    for (std::size_t i = 0u; i < folder.size(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(folder[i]);
        if ((i != 0u) && (std::isupper(c) != 0))
        {
            words += ' ';
        }
        words += folder[i];
    }
    const std::size_t conjunction = words.find(" And ");
    if (conjunction != std::string::npos)
    {
        words.replace(conjunction, 5u, " & ");
    }
    return words.empty() ? std::string("Other") : words;
}

// ----------------------------------------------------------------------------
//! \brief The folder the gallery was started from, found from the program
//! itself so that it does not depend on the working directory.
// ----------------------------------------------------------------------------
static std::filesystem::path programFolder()
{
#if defined(__linux__)
    char buffer[4096];
    const ssize_t length =
        ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1u);
    if (length > 0)
    {
        buffer[length] = '\0';
        return std::filesystem::path(buffer).parent_path();
    }
#endif
    return std::filesystem::current_path();
}

// ----------------------------------------------------------------------------
//! \brief Where a file of examples/ is, looked for around the working directory
//! and around the program. Empty when it cannot be found.
// ----------------------------------------------------------------------------
static std::filesystem::path findExampleFile(std::string const& p_relative)
{
    std::error_code ignored;
    for (char const* root : { "examples/", "../examples/", "../../examples/" })
    {
        const std::filesystem::path candidate =
            std::filesystem::path(root) / p_relative;
        if (std::filesystem::is_regular_file(candidate, ignored))
        {
            return candidate;
        }
    }
    std::filesystem::path dir = programFolder();
    for (int depth = 0; depth < 6; ++depth)
    {
        const std::filesystem::path candidate = dir / "examples" / p_relative;
        if (std::filesystem::is_regular_file(candidate, ignored))
        {
            return candidate;
        }
        if (!dir.has_parent_path() || (dir == dir.parent_path()))
        {
            break;
        }
        dir = dir.parent_path();
    }
    return {};
}

//! \brief Where Dear ImGui keeps the layout of the panels, next to the program.
static std::string const& layoutFile()
{
    static const std::string path = (programFolder() / "gallery.ini").string();
    return path;
}

void Gallery::add(std::string p_name, std::string p_source, Factory p_factory)
{
    Entry entry;
    entry.chapter = chapterOf(p_source);
    const std::size_t underscore = p_name.find('_');
    entry.title = (underscore == std::string::npos)
                      ? p_name
                      : p_name.substr(underscore + 1u);
    entry.name = std::move(p_name);
    entry.source = std::move(p_source);
    entry.factory = std::move(p_factory);
    m_entries.emplace_back(std::move(entry));
}

void Gallery::checkManifest(std::string const& p_actual, char const* p_expected)
{
    const std::string expected = p_expected;
    if (p_actual != expected)
    {
        m_manifest_problems.emplace_back("manifest expects '" + expected +
                                         "' but the example reports '" +
                                         p_actual + "'");
    }
    for (Entry const& entry : m_entries)
    {
        if (entry.name == expected)
        {
            m_manifest_problems.emplace_back(
                "manifest duplicates example name '" + expected + "'");
            break;
        }
    }
    if (!m_entries.empty() && (m_entries.back().name >= expected))
    {
        m_manifest_problems.emplace_back("manifest name '" + expected +
                                         "' is out of order after '" +
                                         m_entries.back().name + "'");
    }
}

Gallery::~Gallery()
{
    // The example goes first: it holds device resources, and the context they
    // live on belongs to the window, which is destroyed after this body runs.
    m_current.reset();
    if (g_gallery == this)
    {
        compages::gpu::logger(nullptr);
        g_gallery = nullptr;
    }
}

void Gallery::onLog(compages::gpu::LogLevel p_level, std::string_view p_message)
{
    Gallery* self = g_gallery;
    const std::string example =
        ((self != nullptr) && (self->m_current != nullptr))
            ? self->m_entries[self->m_index].title
            : std::string();

    // Said once on the terminal as well, next to the name of the example when
    // the gallery walks the list: a driver repeating itself every frame is one
    // line there, and counted in the Console panel. What is only worth knowing
    // while an example runs, such as a performance hint, stays in the panel.
    const bool first =
        (self == nullptr) || self->m_console.add(p_level, p_message, example);
    if (first && ((p_level != compages::gpu::LogLevel::Info) || example.empty()))
    {
        char const* prefix = "[gpu] ";
        if (p_level == compages::gpu::LogLevel::Warning)
        {
            prefix = "[gpu] warning: ";
        }
        else if (p_level == compages::gpu::LogLevel::Error)
        {
            prefix = "[gpu] error: ";
        }
        std::cerr << prefix << p_message << std::endl;
    }
}

void Gallery::fail(std::string const& p_why)
{
    Entry& entry = m_entries[m_index];
    m_failure = p_why;
    entry.outcome = Outcome::Failed;
    entry.problem = p_why;
    m_problems.emplace_back(entry.name + " " + p_why);
    m_console.add(compages::gpu::LogLevel::Error, p_why, entry.title);
    std::cerr << entry.name << ": " << p_why << std::endl;
    // Dropped rather than kept half working: the reason stays on the screen,
    // and F5 tries again. What it held must still all come back.
    hide();
}

void Gallery::hide()
{
    if (m_current == nullptr)
    {
        return;
    }

    Entry& entry = m_entries[m_index];
    m_current.reset();

    m_leak = whatStayedBehind(m_before, compages::gpu::resourceStatistics());
    if (!m_leak.empty())
    {
        const std::string why =
            "left " + m_leak + " on the device after being closed";
        if (entry.outcome != Outcome::Failed)
        {
            entry.outcome = Outcome::Leaked;
            entry.problem = why;
        }
        m_problems.emplace_back(entry.name + " " + why);
        m_console.add(compages::gpu::LogLevel::Error, why, entry.title);
        std::cerr << "Leak: " << entry.name << " " << why << std::endl;
    }
}

namespace
{

//! \brief ImGui would otherwise pick an arrow cursor every frame over GLFW's
//! hidden pointer.
void syncImGuiMouseCapture(bool p_captured)
{
    ImGuiIO& io = ImGui::GetIO();
    if (p_captured)
    {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
    }
    else
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
    }
}

} // namespace

void Gallery::captureMouse(bool p_captured)
{
    if (p_captured == m_mouse_captured)
    {
        return;
    }
    m_mouse_captured = p_captured;
    m_window.captureMouse(p_captured);
    syncImGuiMouseCapture(p_captured);
}

void Gallery::show(std::size_t p_index)
{
    hide();
    captureMouse(false);

    m_failure.clear();
    m_frames = 0u;
    m_example_time = 0.0f;
    m_index = p_index;
    m_focus_view = true;
    if (m_index >= m_entries.size())
    {
        return;
    }

    // Read before the example is built, so that what the panels show is what
    // this example added.
    m_before = compages::gpu::resourceStatistics();
    Entry& entry = m_entries[m_index];
    m_current = entry.factory();
    if (m_current == nullptr)
    {
        return fail("could not be created at all");
    }
    if (m_announce)
    {
        std::cerr << "Showing " << entry.name << std::endl;
    }

    (void)compages::gpu::takeFrameError();
    compages::gpu::Status ready = m_current->setUp();
    if (ready && compages::gpu::hasFrameError())
    {
        ready = compages::gpu::failure(compages::gpu::takeFrameError());
    }
    if (!ready)
    {
        return fail("could not set up: " + ready.error());
    }
    entry.outcome = Outcome::Fine;
    entry.problem.clear();
    if (m_current->capturesMouse() && !m_show_overlay)
    {
        captureMouse(true);
    }
}

void Gallery::resizeView(std::uint32_t p_width, std::uint32_t p_height)
{
    if ((p_width == m_view_width) && (p_height == m_view_height))
    {
        return;
    }

    const compages::gpu::ResourceStatistics before = compages::gpu::resourceStatistics();
    m_view_width = 0u;
    m_view_height = 0u;
    compages::gpu::Status made = m_view_color.allocate({ .format = compages::gpu::PixelFormat::RGB8,
                                               .width = p_width,
                                               .height = p_height });
    if (made)
    {
        made = m_view_depth.allocate({ .format = compages::gpu::PixelFormat::Depth32F,
                                       .width = p_width,
                                       .height = p_height,
                                       .magnify = compages::gpu::Filter::Nearest,
                                       .minify = compages::gpu::Filter::Nearest });
    }
    if (made)
    {
        made = m_view.attach(m_view_color, m_view_depth);
    }
    carry(m_before, before, compages::gpu::resourceStatistics());

    if (!made)
    {
        m_console.add(compages::gpu::LogLevel::Error,
                      "the viewport cannot be made: " + made.error(),
                      {});
        return;
    }
    m_view_width = p_width;
    m_view_height = p_height;
}

Frame Gallery::makeFrame(std::uint32_t p_width, std::uint32_t p_height)
{
    Frame frame;
    frame.width = p_width;
    frame.height = p_height;

    // Time as the example sees it: stopped while paused, one sixtieth of a
    // second per step, scaled by the speed of the Debug panel.
    float elapsed = m_window.elapsed() * m_debug.speed;
    if (m_debug.paused)
    {
        elapsed = m_debug.step ? (1.0f / 60.0f) : 0.0f;
    }
    m_debug.step = false;
    m_example_time += elapsed;
    frame.elapsed = elapsed;
    frame.total = m_example_time;

    // With the panels shown, the example gets the mouse while it is over the
    // viewport, and the keyboard while the viewport has focus or the mouse is
    // over it; without them, it gets everything.
    ImGuiIO const& io = ImGui::GetIO();
    const bool mouse =
        m_mouse_captured || (m_show_overlay ? (m_view_hovered || m_view_dragged)
                                            : !io.WantCaptureMouse);
    const bool keyboard =
        m_mouse_captured ||
        (!io.WantTextInput && (!m_show_overlay || m_view_focused ||
                               m_view_hovered || !io.WantCaptureKeyboard));

    bool click = m_window.mouseLeftPressed();

    frame.input.mouse = m_show_overlay ? m_view_mouse : m_window.mouse();
    frame.input.mouse_over = mouse;
    frame.input.mouse_captured = m_mouse_captured;
    if (mouse)
    {
        frame.input.mouse_delta = m_window.mouseDelta();
        frame.input.scroll = m_window.scroll();
        frame.input.mouse_left = m_window.mouseLeft();
        frame.input.mouse_right = m_window.mouseRight();
        frame.input.mouse_left_pressed = click;
    }
    if (keyboard)
    {
        compages::world::KeyMap const map = (m_current != nullptr)
                                      ? m_current->keyMap()
                                      : compages::world::KeyMap::defaults();
        map.apply(frame.input,
                  [this](int p_glfw_key)
                  { return m_window.keyDown(p_glfw_key); });
    }
    return frame;
}

void Gallery::runExample()
{
    // Counted even when nothing runs, so that a run walking the list does not
    // stall on an example that failed to set itself up.
    ++m_frames;
    if (m_current == nullptr)
    {
        return;
    }

    const std::uint32_t width =
        m_show_overlay ? m_view_width : m_window.width();
    const std::uint32_t height =
        m_show_overlay ? m_view_height : m_window.height();
    if ((width == 0u) || (height == 0u))
    {
        return;
    }
    const Frame frame = makeFrame(width, height);

    {
        // With the panels, the example draws into the texture the Viewport
        // panel shows. It does not know: a pass naming no target draws into the
        // pass around it, and this is the one around it.
        std::optional<compages::gpu::RenderPass> view;
        if (m_show_overlay)
        {
            view.emplace(m_view, compages::gpu::PassDesc{ .color = BACKGROUND });
        }
        compages::gpu::resetFrameStatistics();
        m_current->draw(frame);
        m_frame_statistics = compages::gpu::frameStatistics();
    }

    if (compages::gpu::hasFrameError())
    {
        const std::size_t errors = compages::gpu::frameErrorCount();
        std::string why = compages::gpu::takeFrameError();
        if (errors > 1u)
        {
            why += " (and " + std::to_string(errors - 1u) +
                   " more errors this frame)";
        }
        if (m_debug.stop_on_error)
        {
            fail("stopped while drawing: " + why);
        }
        else
        {
            m_console.add(compages::gpu::LogLevel::Error, why, m_entries[m_index].title);
        }
    }
}

void Gallery::handleShortcuts()
{
    ImGuiIO const& io = ImGui::GetIO();
    if (io.WantTextInput)
    {
        return;
    }

    // Escape releases a captured pointer before it closes the window or goes
    // through ImGui, which may not see the key while a game holds the mouse.
    if (m_mouse_captured && m_window.keyDown(GLFW_KEY_ESCAPE))
    {
        captureMouse(false);
        return;
    }

    // Page Up and Page Down only: the arrows belong to the examples.
    auto pressed = [](ImGuiKey p_key)
    { return ImGui::IsKeyPressed(p_key, false); };
    const bool next = pressed(ImGuiKey_PageDown);
    const bool previous = pressed(ImGuiKey_PageUp);

    if (pressed(ImGuiKey_Escape) && !m_mouse_captured)
    {
        m_window.close();
    }
    else if (pressed(ImGuiKey_F1))
    {
        m_show_overlay = !m_show_overlay;
    }
    else if (pressed(ImGuiKey_F2))
    {
        compages::gpu::showWireframe(!compages::gpu::wireframeShown());
    }
    else if (pressed(ImGuiKey_F5))
    {
        show(m_index);
    }
    else if (pressed(ImGuiKey_F6))
    {
        m_debug.paused = !m_debug.paused;
    }
    else if (pressed(ImGuiKey_F7))
    {
        m_debug.paused = true;
        m_debug.step = true;
    }
    else if (pressed(ImGuiKey_F12))
    {
        (io.KeyShift ? m_window_screenshot_asked : m_screenshot_asked) = true;
    }
    else if (next)
    {
        show((m_index + 1u) % m_entries.size());
    }
    else if (previous)
    {
        show((m_index + m_entries.size() - 1u) % m_entries.size());
    }
}

compages::gpu::Status Gallery::run(Options const& p_options)
{
    if (!m_manifest_problems.empty())
    {
        std::string why = "invalid example manifest:";
        for (std::string const& problem : m_manifest_problems)
        {
            why += "\n  " + problem;
        }
        return compages::gpu::failure(why);
    }
    if (m_entries.empty())
    {
        return compages::gpu::failure("the gallery has no examples in it");
    }

    m_show_overlay = p_options.overlay;
    m_announce = (p_options.frames != 0u);

    // Before the window, so that what the device says when it starts is in
    // the Console panel too.
    g_gallery = this;
    compages::gpu::logger(&Gallery::onLog);
    COMPAGES_TRY(m_window.open("Compages", 1280, 800));

    // Dear ImGui talks to the same context we do, with its own shaders and
    // textures: our record of the device state is thrown away after it drew.
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& imgui_io = ImGui::GetIO();
    imgui_io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    imgui_io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    ui::applyGalleryTheme();
    // A run walking the list alone neither reads nor writes the layout of the
    // person who arranged the panels.
    imgui_io.IniFilename =
        (p_options.frames == 0u) ? layoutFile().c_str() : nullptr;
    ImGui_ImplGlfw_InitForOpenGL(m_window.handle(), true);
    ImGui_ImplOpenGL3_Init("#version 130");

    std::size_t start = 0u;
    if (!p_options.start.empty())
    {
        bool found = false;
        for (std::size_t i = 0u; i < m_entries.size(); ++i)
        {
            if (m_entries[i].name == p_options.start)
            {
                start = i;
                found = true;
                break;
            }
        }
        if (!found)
        {
            std::cerr << "No example is called " << p_options.start
                      << ", starting at the first one instead" << std::endl;
        }
    }
    show(start);

    //! How many examples are left to visit when the gallery walks the list.
    std::size_t remaining = m_entries.size();

    while (!m_window.closing())
    {
        m_window.beginFrame();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        if (m_mouse_captured)
        {
            syncImGuiMouseCapture(true);
        }

        const std::uint32_t width = m_window.width();
        const std::uint32_t height = m_window.height();
        if ((width != 0u) && (height != 0u))
        {
            // The window pass: the background behind the panels, and the
            // example itself when the panels are hidden.
            compages::gpu::RenderPass screen(compages::gpu::PassDesc{
                .width = width, .height = height, .color = BACKGROUND });

            // The panels first: the Viewport panel decides how large the
            // example is drawn this very frame.
            if (m_show_overlay)
            {
                drawUi();
            }
            else
            {
                ImGuiIO const& io = ImGui::GetIO();
                drawHud(0.0f, 0.0f, io.DisplaySize.x, io.DisplaySize.y);
            }
            runExample();

            if (m_screenshot_asked)
            {
                m_screenshot_asked = false;
                saveScreenshot("screenshots", false);
            }
        }
        else
        {
            ++m_frames;
        }
        const bool last_frame =
            (p_options.frames != 0u) && (m_frames == p_options.frames);
        const bool shoot = !p_options.screenshots.empty() && last_frame;
        if (shoot && !p_options.screenshots_with_panels)
        {
            saveScreenshot(p_options.screenshots, false);
        }

        if (m_mouse_captured)
        {
            syncImGuiMouseCapture(true);
        }
        // Render() ends the frame ImGui started, drawn or not.
        // Drawn with the panels hidden too: what is left then is the HUD of
        // the example, if it has one.
        ImGui::Render();
        if (ImGui::GetDrawData()->TotalVtxCount > 0)
        {
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            // ImGui drew with plain OpenGL calls of its own: whatever state it
            // left is not what our cache believes.
            compages::gpu::forgetRenderState();
        }
        if (shoot && p_options.screenshots_with_panels)
        {
            saveScreenshot(p_options.screenshots, true);
        }
        if (m_window_screenshot_asked)
        {
            m_window_screenshot_asked = false;
            saveScreenshot("screenshots", true);
        }

        handleShortcuts();
        m_window.endFrame();

        if ((p_options.frames != 0u) && (m_frames >= p_options.frames))
        {
            --remaining;
            if (p_options.visit_all && (remaining == 0u))
            {
                m_window.close();
            }
            else
            {
                show((m_index + 1u) % m_entries.size());
            }
        }
    }

    hide();
    compages::gpu::showWireframe(false);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // The viewport lives on the context the window is about to destroy.
    m_view.release();
    m_view_color = compages::gpu::Texture();
    m_view_depth = compages::gpu::Texture();

    if (!m_problems.empty())
    {
        std::string why =
            std::to_string(m_problems.size()) + " example(s) did not behave:";
        for (std::string const& problem : m_problems)
        {
            why += "\n  " + problem;
        }
        return compages::gpu::failure(why);
    }
    return compages::gpu::success();
}

void Gallery::saveScreenshot(std::string const& p_directory,
                             bool p_whole_window)
{
    if (m_index >= m_entries.size())
    {
        return;
    }

    // Read through a pass, since a pass is what says which target is being
    // read: the viewport when the panels are shown, the window otherwise. It
    // clears nothing.
    const bool view = !p_whole_window && m_show_overlay && (m_view_width != 0u);
    const std::uint32_t width = view ? m_view_width : m_window.width();
    const std::uint32_t height = view ? m_view_height : m_window.height();
    compages::gpu::PassDesc keep{ .clear_color = false, .clear_depth = false };
    std::optional<compages::gpu::RenderPass> pass;
    if (view)
    {
        pass.emplace(m_view, keep);
    }
    else
    {
        keep.width = width;
        keep.height = height;
        pass.emplace(keep);
    }

    compages::gpu::Result<std::vector<std::byte>> picture = compages::gpu::readPixels();
    if (!picture)
    {
        m_console.add(compages::gpu::LogLevel::Error,
                      "cannot read the screen: " + picture.error(),
                      {});
        return;
    }

    std::error_code failed;
    std::filesystem::create_directories(p_directory, failed);
    if (failed)
    {
        m_console.add(compages::gpu::LogLevel::Error,
                      "cannot write into " + p_directory + ": " +
                          failed.message(),
                      {});
        return;
    }

    const std::string path =
        p_directory + "/" + m_entries[m_index].name + ".png";
    compages::gpu::Status saved = writePng(path, width, height, picture.value());
    const std::string said = saved ? ("wrote " + path) : saved.error();
    m_console.add(saved ? compages::gpu::LogLevel::Info : compages::gpu::LogLevel::Error, said, {});
    std::cerr << said << std::endl;
}

Gallery::SourceFile const& Gallery::sourceFile(std::string const& p_relative)
{
    auto found = m_sources.find(p_relative);
    if (found != m_sources.end())
    {
        return found->second;
    }

    SourceFile source;
    std::vector<std::string>& lines = source.lines;
    const std::filesystem::path path = findExampleFile(p_relative);
    if (path.empty())
    {
        lines.emplace_back("// " + p_relative +
                           " is not where the gallery looked for it:");
        lines.emplace_back(
            "// examples/ next to the working directory or to the program.");
    }
    else
    {
        std::ifstream file(path);
        std::string line;
        while (std::getline(file, line))
        {
            lines.emplace_back(line);
        }

        // The licence between the two //==== rules says nothing about the
        // example: shown from the first line after it that is not blank.
        if (!lines.empty() && (lines.front().rfind("//===", 0u) == 0u))
        {
            auto end =
                std::find_if(lines.begin() + 1,
                             lines.end(),
                             [](std::string const& p_line)
                             { return p_line.rfind("//===", 0u) == 0u; });
            if (end != lines.end())
            {
                ++end;
                while ((end != lines.end()) && end->empty())
                {
                    ++end;
                }
                source.first += static_cast<std::size_t>(end - lines.begin());
                lines.erase(lines.begin(), end);
            }
        }
    }
    return m_sources.emplace(p_relative, std::move(source)).first->second;
}

} // namespace examples
