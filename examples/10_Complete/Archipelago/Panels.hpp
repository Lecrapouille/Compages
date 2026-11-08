// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Knowledge.hpp"
#include "10_Complete/Archipelago/Land.hpp"
#include "Common/Example.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace examples::archipelago
{

//! \brief What the map shows; the keys of Panels change it.
struct View
{
    bool borders = true;
    bool names = true;
    bool animals = true;
    bool details = true;
    //! \brief How much faster the animals live than the clock: a boat at 7
    //! m/s takes an hour round an island.
    float time_scale = 8.0f;
};

// ****************************************************************************
//! \brief What the panels ask the demo to do.
// ****************************************************************************
class Navigator
{
public:

    virtual ~Navigator() = default;
    //! \brief Choose a region, or a link of it (part >= 0); none: region -1.
    virtual void select(Place p_place) = 0;
    //! \brief Fly to a region or a link of it.
    virtual void zoomOn(Place p_place) = 0;
    //! \brief Fly over an island.
    virtual void zoomOnIsland(std::size_t p_island) = 0;
    //! \brief Fly back over the whole map.
    virtual void home() = 0;
    //! \brief Open a link (part >= 0) or the links of a region.
    virtual void open(Place p_place) = 0;
};

// ****************************************************************************
//! \brief The interface over the picture, as IslandedBrowser draws it: a
//! tooltip next to the pointer (title, URL, where it is, what a click does),
//! the details of the choice on the right (its links, clickable), a search
//! box (/ or Ctrl+F), a status bar with the keys of the moment, short
//! notifications, and a help (H).
// ****************************************************************************
class Panels
{
public:

    //! \brief A short notification, faded out after a few seconds.
    void toast(std::string p_message);

    //! \brief The keys and the windows of this frame.
    //! \param[in] p_hovered The place under the pointer.
    //! \param[in] p_chosen The place chosen.
    //! \param[in] p_camera How far the camera is, for the status bar.
    void draw(Example::Overlay const& p_view,
              Knowledge const& p_knowledge,
              Place p_hovered,
              Place p_chosen,
              float p_camera,
              View& p_options,
              Navigator& p_navigator);

    //! \brief The help or the search box is open: the clicks on the map
    //! only close the help.
    [[nodiscard]] bool helpShown() const
    {
        return m_help;
    }

private:

    struct Frame
    {
        Example::Overlay const& view;
        Knowledge const& knowledge;
        Place hovered;
        Place chosen;
    };

    void keys(Frame const& p_frame, View& p_options, Navigator& p_navigator);
    void searchKeys(Frame const& p_frame, Navigator& p_navigator);
    void tooltip(Frame const& p_frame) const;
    void details(Frame const& p_frame, Navigator& p_navigator);
    void search(Frame const& p_frame, Navigator& p_navigator);
    void statusBar(Frame const& p_frame, float p_camera) const;
    void notification(Frame const& p_frame);
    void help(Frame const& p_frame);
    void updateMatches(Knowledge const& p_knowledge);
    void nextMatch(int p_direction, Navigator& p_navigator);

    bool m_help = false;
    bool m_focus_help = false;
    bool m_searching = false;
    bool m_focus_search = false;
    //! \brief One of these windows had the focus last frame: the keys are
    //! still for the picture.
    bool m_focused = false;
    std::string m_query;
    std::vector<Place> m_matches;
    std::size_t m_match = 0u;
    std::string m_toast;
    double m_toast_time = -100.0;
};

} // namespace examples::archipelago
