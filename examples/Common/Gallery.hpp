// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Console.hpp"
#include "Common/Example.hpp"
#include "Common/Window.hpp"

#include <functional>
#include <map>
#include <string>
#include <vector>

// ****************************************************************************
//! \file
//! \brief One program holding every example, switched without restarting.
//!
//! There is one binary rather than forty, for a reason that is not laziness:
//! switching from one example to the next inside a live device is what proves
//! the resources of the one being left are actually given back. The panels
//! show the counters of the device next to the counters of the frame, so a
//! leak shows up as a number that does not come back down when an example is
//! closed.
//!
//! The window is laid out like an editor, every panel docked and movable:
//!
//! - **Viewport**, in the middle: the example, drawn into a texture of the
//!   size of the panel, so that no panel ever hides part of it. Mouse and
//!   keyboard go to the example while the pointer is over it or it has focus.
//! - **Examples**, on the left: the list, by chapter, with a mark on those
//!   that failed or leaked.
//! - **Inspector**, **Source** and **Debug**, on the right: what the example
//!   shows, its code, and the switches of the debug mode (pause, one frame at
//!   a time, wireframe, keep drawing after an error...).
//! - **Console**, at the bottom: every message of the library and the driver,
//!   each said once with a count.
//!
//! An example is built only when it is chosen. Nothing is on the device for
//! the examples nobody is looking at.
// ****************************************************************************

namespace examples
{

// ****************************************************************************
//! \brief Every example, with the one being shown.
//!
//! \code
//! Gallery gallery;
//! gallery.add<Triangle>("01b_Triangle", "00_GettingStarted/01b_Triangle.cpp");
//! return gallery.run() ? EXIT_SUCCESS : EXIT_FAILURE;
//! \endcode
// ****************************************************************************
class Gallery
{
public:

    //! \brief How an example is built, when and only when it is asked for.
    using Factory = std::function<ExamplePtr()>;

    // ************************************************************************
    //! \brief How the gallery is meant to run this time.
    // ************************************************************************
    struct Options
    {
        //! \brief Which example to show first, by name. Empty for the first in
        //! the list.
        std::string start;

        //! \brief How many frames to give each example before moving on, or
        //! zero to stay on it until someone chooses another.
        std::size_t frames = 0u;

        //! \brief Close the window once every example has had its turn.
        //!
        //! With frames set, this turns the gallery into a test: every example
        //! is built, drawn, and closed, and anything it failed to give back is
        //! reported.
        bool visit_all = false;

        //! \brief Show the panels around the example.
        bool overlay = true;

        //! \brief Where to write one picture per example, or empty to write
        //! none. Taken on the last frame an example is given; a black picture
        //! means the example drew nothing.
        std::string screenshots;

        //! \brief Take those pictures of the whole window, panels included,
        //! rather than of the example alone.
        bool screenshots_with_panels = false;
    };

    ~Gallery();

    // ------------------------------------------------------------------------
    //! \brief Register an example, checking the class says the same name.
    //!
    //! \param[in] p_name the name the manifest gives it, which orders the list.
    //! \param[in] p_source its .cpp, relative to examples/: its folder is the
    //! chapter it is listed under, and the Source panel shows it.
    // ------------------------------------------------------------------------
    template <typename T>
    void add(char const* p_name, char const* p_source)
    {
        // Built once here and thrown away: an example puts nothing on the
        // device until setUp(), so this costs nothing.
        T probe;
        checkManifest(probe.name(), p_name);
        add(p_name, p_source, []() { return ExamplePtr(new T()); });
    }

    // ------------------------------------------------------------------------
    //! \brief Register an example built by a function.
    // ------------------------------------------------------------------------
    void add(std::string p_name, std::string p_source, Factory p_factory);

    // ------------------------------------------------------------------------
    //! \brief Open a window and show examples until the user closes it.
    //!
    //! \return why the gallery could not run, or which examples failed or
    //! leaked, so that a run through the whole list is worth something to a
    //! build script.
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::Status run(Options const& p_options);

    //! \brief Same, starting at the first example.
    [[nodiscard]] compages::Status run()
    {
        return run(Options{});
    }

private:

    //! \brief How an example did the last time it ran.
    enum class Outcome
    {
        NotRun,
        Fine,
        Failed,
        Leaked,
    };

    // ------------------------------------------------------------------------
    //! \brief What the list holds for each example.
    // ------------------------------------------------------------------------
    struct Entry
    {
        std::string name;
        //! \brief The name shown to people, without its number: "Dummy" for
        //! "00a_Dummy". The name itself stays what --start and the files use.
        std::string title;
        std::string source;
        //! \brief The folder of the source, spelled for people: "Getting
        //! Started".
        std::string chapter;
        Factory factory;
        Outcome outcome = Outcome::NotRun;
        //! \brief What went wrong, for the tooltip of the list.
        std::string problem;
    };

    // ------------------------------------------------------------------------
    //! \brief The switches of the Debug panel.
    // ------------------------------------------------------------------------
    struct DebugMode
    {
        //! \brief Time stands still; the example keeps drawing.
        bool paused = false;
        //! \brief Give one frame of 1/60 s while paused.
        bool step = false;
        //! \brief How fast time runs for the example.
        float speed = 1.0f;
        //! \brief Stop the example at its first frame error, rather than
        //! logging each error and drawing on.
        bool stop_on_error = true;
    };

    void checkManifest(std::string const& p_actual, char const* p_expected);

    // The life of an example.
    void show(std::size_t p_index);
    void hide();
    void fail(std::string const& p_why);

    // One frame.
    void runExample();
    [[nodiscard]] compages::world::ViewFrame makeFrame(std::uint32_t p_width,
                                                       std::uint32_t p_height);
    void handleShortcuts();
    //! \brief Write a picture of the example, or of the whole window.
    void saveScreenshot(std::string const& p_directory, bool p_whole_window);

    //! \brief Make the texture the example is drawn into this large, keeping
    //! the counters of the example as they were.
    void resizeView(std::uint32_t p_width, std::uint32_t p_height);
    //! \brief Hide and hold the pointer for the example, or give it back.
    void captureMouse(bool p_captured);

    // The panels (GalleryPanels.cpp).
    void drawUi();
    void drawMenuBar();
    void drawStatusBar();
    void drawDockSpace();
    void drawViewport();
    void drawExamplesPanel();
    void drawInspectorPanel();
    //! \brief The widgets of Example::controls(), in a panel of their own.
    void drawTryItPanel();
    void drawSourcePanel();
    void drawDebugPanel();
    void drawConsolePanel();
    //! \brief What Example::hud() says, over the rectangle the picture
    //! covers on the screen, in points.
    void drawHud(float p_x, float p_y, float p_width, float p_height);

    //! \brief A file of examples/ as the Source panel shows it.
    struct SourceFile
    {
        //! \brief The number of the first line kept, the licence skipped.
        std::size_t first = 1u;
        std::vector<std::string> lines;
    };

    //! \brief A file of examples/, read once.
    [[nodiscard]] SourceFile const& sourceFile(std::string const& p_relative);

    //! \brief What the library logs, through the one gallery alive.
    static void onLog(compages::gpu::LogLevel p_level,
                      std::string_view p_message);

    std::vector<Entry> m_entries;
    std::vector<std::string> m_manifest_problems;
    Window m_window;
    Console m_console;
    DebugMode m_debug;

    ExamplePtr m_current;
    std::size_t m_index = 0u;
    //! \brief How many frames the current example has been given.
    std::size_t m_frames = 0u;
    //! \brief Every example that failed or leaked, and what it said.
    std::vector<std::string> m_problems;
    //! \brief Why the current example is not running, if it is not.
    std::string m_failure;
    //! \brief Time as the example sees it: paused, stepped, sped up.
    float m_example_time = 0.0f;

    //! \brief What the device held before the current example was set up.
    compages::gpu::ResourceStatistics m_before;
    //! \brief What the previous example left behind, if anything.
    std::string m_leak;
    //! \brief What the last frame of the example cost, read by the panels.
    compages::gpu::FrameStatistics m_frame_statistics;

    // The viewport: the example drawn into a texture shown by a panel.
    compages::gpu::Texture m_view_color;
    compages::gpu::Texture m_view_depth;
    compages::gpu::Framebuffer m_view;
    std::uint32_t m_view_width = 0u;
    std::uint32_t m_view_height = 0u;
    //! \brief Where the mouse is over the viewport, in its pixels, y up.
    compages::core::Vector2f m_view_mouse{ 0.0f, 0.0f };
    bool m_view_hovered = false;
    bool m_view_focused = false;
    //! \brief A drag started on the viewport goes on to the example even when
    //! the pointer leaves it.
    bool m_view_dragged = false;
    //! \brief Give the keyboard focus to the viewport at the next frame, so
    //! that an example just chosen from a panel gets its keys at once.
    bool m_focus_view = true;
    //! \brief The pointer is hidden and held by an example that asked for it
    //! with Example::capturesMouse(); Escape gives it back.
    bool m_mouse_captured = false;

    std::map<std::string, SourceFile> m_sources;

    bool m_show_overlay = true;
    bool m_reset_layout = false;
    bool m_screenshot_asked = false;
    bool m_window_screenshot_asked = false;
    //! \brief Say on the terminal which example is being shown: on when the
    //! gallery walks the list on its own, where the terminal is all there is.
    bool m_announce = false;
};

} // namespace examples
