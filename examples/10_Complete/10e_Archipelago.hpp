// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Browser.hpp"
#include "10_Complete/Archipelago/FineTiles.hpp"
#include "10_Complete/Archipelago/Highlight.hpp"
#include "10_Complete/Archipelago/Job.hpp"
#include "10_Complete/Archipelago/Knowledge.hpp"
#include "10_Complete/Archipelago/Labels.hpp"
#include "10_Complete/Archipelago/Land.hpp"
#include "10_Complete/Archipelago/Panels.hpp"
#include "10_Complete/Archipelago/Sea.hpp"
#include "10_Complete/Archipelago/Wildlife.hpp"
#include "Common/Example.hpp"

#include "Compages/Core/Vector.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <memory>
#include <mutex>
#include <string>

namespace examples
{

// ****************************************************************************
//! \brief A map of knowledge, after Chréage: islands of themes made of
//! Voronoi cells around the graph of their links, a main river per island,
//! a tributary per region, a link per cell along it; rivers that dig their
//! valleys; a living sea.
//!
//! The contents and their graph come from IslandedBrowser (bookmarks of
//! Firefox, Archipelago/Knowledge.hpp). The land is made on the CPU, in a
//! thread, around this graph and from a seed (Archipelago/Land.hpp); the
//! gallery shows a progress bar meanwhile. The folder Archipelago/ holds the
//! parts; this class puts them together:
//! - Sea: the coarse tiles of the map, the ocean, the surface of the sea
//!   whose corners move with Gerstner waves;
//! - FineTiles: near the camera, finer tiles made in the background;
//! - Wildlife: boats, birds and fish, a few lines each;
//! - Labels, Highlight: the names, the pins, the region under the mouse;
//! - Panels: over the picture, the interface of IslandedBrowser (a tooltip,
//!   the details of the choice, a search, a status bar, a help on H);
//! - Browser: the links open in the web browser.
//! The camera is a Rig; a double click flies to a region with
//! RigController::flyTo.
// ****************************************************************************
class Archipelago: public Example, private archipelago::Navigator
{
public:

    ~Archipelago() override;

    [[nodiscard]] std::string name() const override
    {
        return "10e_Archipelago";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;
    void overlay(Overlay const& p_view) override;
    [[nodiscard]] Progress progress() const override;

private:

    //! \brief What the loading thread makes: the land and its coarse patch.
    struct Loaded
    {
        archipelago::Land land;
        archipelago::Patch base;
    };

    //! \brief How far the loading thread has gone, read by the gallery.
    struct Report
    {
        std::mutex mutex;
        float done = 0.0f;
        std::string step = "Starting";
    };

    //! \brief How the land is made (Try it panel).
    struct Generation
    {
        int seed = 7;
        int erosion_steps = 60;
    };

    //! \brief The mouse: the place under it, the place chosen by a click,
    //! where the left button went down (a click if it comes up near), the
    //! last click (a second one on the same place soon after is a double
    //! click), the middle button.
    struct Pointer
    {
        archipelago::Place hovered;
        archipelago::Place chosen;
        compages::Vector2f press{ 0.0f, 0.0f };
        bool was_down = false;
        archipelago::Place clicked;
        float clicked_at = -10.0f;
        bool middle_was_down = false;
    };

    //! \brief Read the layout of IslandedBrowser, or take the pages built in.
    void loadKnowledge();

    //! \brief Make the land and its coarse patch, telling how far it goes.
    [[nodiscard]] static Loaded makeMap(archipelago::Recipe const& p_recipe,
                                        Report& p_report);

    //! \brief Start making the land in a thread.
    void startLoading();

    //! \brief The land is made: show it, on the first load or after Generate.
    [[nodiscard]] compages::Status finishLoading(Loaded p_loaded);

    //! \brief The region and the link under a pixel of the picture.
    [[nodiscard]] archipelago::Place
    placeUnder(compages::Vector2f p_mouse) const;

    //! \brief As IslandedBrowser: a click selects, a double click opens a
    //! link or flies to a region, a middle or Ctrl click opens.
    void handleMouse(compages::world::Input const& p_input);

    //! \brief A click on the picture, released where it was pressed.
    void click(archipelago::Place p_place, bool p_open);

    // Navigator: what the panels ask.
    void select(archipelago::Place p_place) override;
    void zoomOn(archipelago::Place p_place) override;
    void zoomOnIsland(std::size_t p_island) override;
    void home() override;
    void open(archipelago::Place p_place) override;

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_camera;
    archipelago::Sea m_sea;
    archipelago::FineTiles m_fine;
    archipelago::Wildlife m_wildlife;
    archipelago::Highlight m_highlight;
    archipelago::Labels m_labels;
    archipelago::Browser m_browser;
    archipelago::Panels m_panels;

    archipelago::Job<Loaded> m_loading;
    std::shared_ptr<Report> m_report;
    Loaded m_map;
    bool m_ready = false;
    //! \brief What the map shows, and why it is not the layout file.
    archipelago::Knowledge m_knowledge;
    std::string m_message;

    Generation m_generation;
    archipelago::View m_view;
    Pointer m_pointer;
    //! \brief Seconds since the start, for the waves.
    float m_time = 0.0f;
};

} // namespace examples
