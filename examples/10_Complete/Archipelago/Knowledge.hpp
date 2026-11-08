// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief What 10e_Archipelago shows: themes (the islands), topics (the
//! regions) and pages (the links), each with its place in a graph layout.
//!
//! The pages and their places come from IslandedBrowser
//! (github.com/Lecrapouille/IslandedBrowser): organize.py groups the
//! bookmarks of Firefox into islands and communities, then a force directed
//! layout (springs between similar pages) gives every node a position,
//! written by `IslandedBrowser organization.json --layout layout.json`. The
//! land is then built around this graph (Land.hpp): related pages end up on
//! the same island, side by side.
// ****************************************************************************

#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"

#include <array>
#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace examples::archipelago
{

//! \brief A link: a web page and its place in the layout (any unit).
struct Page
{
    std::string title;
    std::string url;
    compages::Vector2f node;
};

//! \brief A region: a topic of a theme, and its pages.
struct Topic
{
    std::string name;
    std::size_t theme;
    compages::Vector2f node;
    std::vector<Page> pages;
};

//! \brief An island: a theme. Its colour (sRGB) is the head of its pins.
struct Theme
{
    std::string name;
    std::array<float, 3> hue;
    compages::Vector2f node;
};

//! \brief Everything the map shows. Every theme has a topic, every topic a
//! page.
struct Knowledge
{
    //! \brief Where it was read from ("built in" for builtinKnowledge()).
    std::string source;
    std::vector<Theme> themes;
    std::vector<Topic> topics;
};

// ----------------------------------------------------------------------------
//! \brief Read a layout written by IslandedBrowser (--layout).
//!
//! The islands are the nodes without a parent; the regions their folders;
//! the links the bookmarks under each region (a bookmark deeper in folders
//! goes to the region above it; a bookmark right under an island goes to a
//! region named after the island).
//! \param[in] p_path The file, in the format "islanded-browser-layout".
// ----------------------------------------------------------------------------
[[nodiscard]] compages::Result<Knowledge>
loadKnowledge(std::filesystem::path const& p_path);

// ----------------------------------------------------------------------------
//! \brief Five themes, twenty five topics, three Wikipedia pages each, laid
//! out by hand: what the map shows without a layout file.
// ----------------------------------------------------------------------------
[[nodiscard]] Knowledge builtinKnowledge();

} // namespace examples::archipelago
