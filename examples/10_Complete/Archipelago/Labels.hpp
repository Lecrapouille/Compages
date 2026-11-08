// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "10_Complete/Archipelago/Knowledge.hpp"
#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

#include <string>
#include <vector>

namespace examples::archipelago
{

//! \brief A title short enough for a label: at most 40 bytes, cut between
//! two UTF-8 characters.
[[nodiscard]] std::string shortTitle(std::string const& p_title);

// ****************************************************************************
//! \brief What is written on the map: the names of the islands from afar,
//! of the regions closer, of the links closer still; a pin on the cell of
//! each link (a white stick, a head of the colour of its island). What is
//! under the mouse is told by the panels (Panels.hpp).
// ****************************************************************************
class Labels
{
public:

    //! \brief The names and the pins. Once.
    void create(compages::renderer::Scene& p_scene,
                Knowledge const& p_knowledge);

    //! \brief Put them on a new land.
    void place(Land const& p_land, Knowledge const& p_knowledge);

    //! \brief The names readable from where the camera is, the size of the
    //! pins.
    //! \param[in] p_names Show the names at all.
    void update(compages::Vector3f p_eye,
                Knowledge const& p_knowledge,
                bool p_names);

private:

    std::vector<compages::world::Entity> m_islands;
    std::vector<compages::world::Entity> m_regions;
    std::vector<std::vector<compages::world::Entity>> m_pins;
};

} // namespace examples::archipelago
