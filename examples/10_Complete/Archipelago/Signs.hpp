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

#include "Compages/Renderer/Render/CameraFrame.hpp"

#include <optional>
#include <string>
#include <vector>

struct ImDrawList;

namespace examples::archipelago
{

// ****************************************************************************
//! \brief The names of the map, as Chréage writes them, over the picture:
//! - an island or a region: a rounded sign, a tab of the colour of its
//!   island on its left, raised on a thin stem over its place;
//! - the links of the region under the mouse or chosen: their names around
//!   it, on a circle, each joined to its pin by a dotted line; the names
//!   follow the order of the pins round the circle, so the lines do not
//!   cross much.
//! A sign that would cover another is raised, or left out; the islands come
//! first, then the links, then the nearest regions.
// ****************************************************************************
class Signs
{
public:

    //! \brief The signs of this frame.
    //! \param[in] p_camera The camera of the last picture (Scene::lastCamera).
    //! \param[in] p_names Write the names of the islands and the regions.
    void draw(Example::Overlay const& p_view,
              compages::renderer::CameraFrame const& p_camera,
              Land const& p_land,
              Knowledge const& p_knowledge,
              Place p_hovered,
              Place p_chosen,
              bool p_names);

private:

    //! \brief A rectangle on the screen.
    struct Box
    {
        float x0;
        float y0;
        float x1;
        float y1;
    };

    //! \brief How a sign looks.
    struct Style
    {
        float size;
        unsigned int back;
        unsigned int text;
        unsigned int tab;
        float stem;
    };

    //! \brief Where a point of the World falls on the picture, if it does.
    [[nodiscard]] std::optional<std::pair<float, float>>
    project(compages::Vector3f const& p_point) const;

    //! \brief Nothing written there yet: then it is taken.
    [[nodiscard]] bool reserve(Box const& p_box, bool p_force);

    //! \brief A sign raised over a point; left out if no height is free.
    void sign(compages::Vector3f const& p_point,
              std::string const& p_text,
              Style const& p_style);

    //! \brief The names of the links of a region on a circle around it.
    void circle(std::size_t p_region, int p_marked);

    Example::Overlay m_view{};
    compages::renderer::CameraFrame const* m_camera = nullptr;
    Land const* m_land = nullptr;
    Knowledge const* m_knowledge = nullptr;
    ImDrawList* m_draw = nullptr;
    std::vector<Box> m_taken;
};

} // namespace examples::archipelago
