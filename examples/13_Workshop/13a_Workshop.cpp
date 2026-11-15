// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/13a_Workshop.hpp"

namespace examples
{

// ----------------------------------------------------------------------------
std::string WorkshopRoom::description() const
{
    return "The robot workshop of the tutorials. The arm is made in the World, "
           "with three joints and a routine that moves its tip along the "
           "table; the Scene only gives it shapes and draws it. Right drag "
           "turns the camera, the wheel zooms.";
}

// ----------------------------------------------------------------------------
compages::Status WorkshopRoom::setUp()
{
    workshop::buildRoom(m_scene);
    m_arm = workshop::buildArm(m_world);
    workshop::dressArm(m_scene, m_arm);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void WorkshopRoom::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.draw(p_frame);
}

} // namespace examples
