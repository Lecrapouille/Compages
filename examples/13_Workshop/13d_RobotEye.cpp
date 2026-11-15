// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/13d_RobotEye.hpp"

#include "Compages/GPU/Errors.hpp"

#include <algorithm>
#include <format>

namespace examples
{

// ----------------------------------------------------------------------------
std::string WorkshopEye::description() const
{
    return "A camera on a mast of the arm. Top right, what the robot sees: no "
           "grid on the floor, since its camera does not see the helpers drawn "
           "for you (sees_helpers). Four times a second it also draws into a "
           "RenderTarget of 160 x 120 pixels, and the program counts the blue "
           "pixels of the part: what a vision program would read.";
}

// ----------------------------------------------------------------------------
compages::Status WorkshopEye::setUp()
{
    workshop::buildRoom(m_scene);
    m_arm = workshop::buildArm(m_world);
    COMPAGES_TRY(workshop::paintArm(m_scene, workshop::dressArm(m_scene, m_arm)));
    m_plate.create(m_scene);
    m_plate.warmUp(m_scene, m_arm, 12.0f);
    COMPAGES_TRY(m_eye.create(m_scene, m_arm));
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void WorkshopEye::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.update(p_frame);
    m_plate.step(std::min(p_frame.elapsed, 0.05f), m_arm.tool.worldPosition());
    if (const compages::Status shown = m_plate.show(m_scene); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    if (const compages::Status seen = m_eye.look(m_scene, p_frame.elapsed); !seen)
    {
        compages::gpu::reportError(seen.error());
    }
    m_scene.render();    // the active camera: the first one made
    m_eye.show(m_scene); // then the robot, in its corner
}

// ----------------------------------------------------------------------------
std::string WorkshopEye::hud() const
{
    return std::format("The robot sees the part: {:.1f} % of its picture", 100.0f * m_eye.seen());
}

} // namespace examples
