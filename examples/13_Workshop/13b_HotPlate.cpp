// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/13b_HotPlate.hpp"

#include "Compages/GPU/Errors.hpp"

#include <algorithm>

namespace examples
{

// ----------------------------------------------------------------------------
std::string WorkshopHeat::description() const
{
    return "The tip of the arm is hot. Where it touches the steel plate, the "
           "plate heats; the heat spreads and cools. The temperature is a "
           "Field<float> on a Grid2D, computed by the CPU each frame and shown "
           "by scene.show() in the colours of Inferno (20 to 300 degrees).";
}

// ----------------------------------------------------------------------------
compages::Status WorkshopHeat::setUp()
{
    workshop::buildRoom(m_scene);
    m_arm = workshop::buildArm(m_world);
    workshop::dressArm(m_scene, m_arm);
    m_plate.create(m_scene);
    m_plate.warmUp(m_scene, m_arm, 12.0f);
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void WorkshopHeat::draw(compages::world::ViewFrame const& p_frame)
{
    // Move the arm first, then heat where its tip is now.
    m_scene.update(p_frame);
    m_plate.step(std::min(p_frame.elapsed, 0.05f), m_arm.tool.worldPosition());
    if (const compages::Status shown = m_plate.show(m_scene); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_scene.render();
}

} // namespace examples
