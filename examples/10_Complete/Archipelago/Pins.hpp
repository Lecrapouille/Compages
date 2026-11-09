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

#include <vector>

namespace examples::archipelago
{

// ****************************************************************************
//! \brief A pin on the cell of each link: a white stick, a head of the
//! colour of its island, at least as large as the eye needs. The names are
//! written by Signs, over the picture.
// ****************************************************************************
class Pins
{
public:

    //! \brief The pins, hidden. Once.
    void create(compages::renderer::Scene& p_scene,
                Knowledge const& p_knowledge);

    //! \brief Put them on a new land.
    void place(Land const& p_land);

    //! \brief Shown near the camera, sized for it.
    void update(compages::Vector3f p_eye);

private:

    std::vector<std::vector<compages::world::Entity>> m_pins;
};

} // namespace examples::archipelago
