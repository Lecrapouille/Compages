// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_Workshop/00a_Workshop.hpp"

namespace examples
{

// ----------------------------------------------------------------------------
std::string WorkshopRoom::description() const
{
    return "The robot workshop of the tutorials, step 1: a floor, a table, a blue "
           "part, a lamp, the sun and a camera, each made by the Scene in one "
           "line. Right drag turns the camera, the wheel zooms.";
}

// ----------------------------------------------------------------------------
compages::Status WorkshopRoom::setUp()
{
    workshop::buildRoom(m_scene);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void WorkshopRoom::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.draw(p_frame);
}

} // namespace examples
