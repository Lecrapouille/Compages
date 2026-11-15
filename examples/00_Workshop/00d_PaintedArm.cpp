// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_Workshop/00d_PaintedArm.hpp"

#include "Compages/GPU/Errors.hpp"

#include <algorithm>

namespace examples
{

// ----------------------------------------------------------------------------
std::string WorkshopPaint::description() const
{
    return "Step 4: the arm gets a look of its own. The hook \"surface\" of the "
           "PBR shader paints black stripes across the colour of each link; "
           "the sun, the lamp, the shadows and the fog stay what the Scene "
           "gives. Ten lines of GLSL, one material.";
}

// ----------------------------------------------------------------------------
compages::Status WorkshopPaint::setUp()
{
    workshop::buildRoom(m_scene);
    m_arm = workshop::buildArm(m_scene);
    m_plate.create(m_scene);
    m_plate.warmUp(m_scene, m_arm, 12.0f);
    COMPAGES_TRY(workshop::paintArm(m_scene, m_arm));
    return m_scene.prepare();
}

// ----------------------------------------------------------------------------
void WorkshopPaint::draw(compages::world::ViewFrame const& p_frame)
{
    m_scene.update(p_frame);
    m_plate.step(std::min(p_frame.elapsed, 0.05f), m_arm.tool.worldPosition());
    if (const compages::Status shown = m_plate.show(m_scene); !shown)
    {
        compages::gpu::reportError(shown.error());
    }
    m_scene.render();
}

} // namespace examples
