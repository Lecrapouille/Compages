// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_Workshop/00b_RobotArm.hpp"

namespace examples
{

// ----------------------------------------------------------------------------
std::string WorkshopArm::description() const
{
    return "Step 2: an arm on the table. Its joints are entities, each the child "
           "of the one before, with a revolute joint: turning the shoulder "
           "turns the whole arm after it. A routine sets the three angles from "
           "the time; the World moves it, the Scene only draws it.";
}

// ----------------------------------------------------------------------------
compages::Status WorkshopArm::setUp()
{
    workshop::buildRoom(m_scene);
    m_arm = workshop::buildArm(m_scene);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void WorkshopArm::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.draw(p_frame);
}

} // namespace examples
