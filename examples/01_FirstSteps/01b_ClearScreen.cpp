// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "01_FirstSteps/01b_ClearScreen.hpp"

#include <cmath>

namespace examples
{

// ----------------------------------------------------------------------------
std::string ClearScreen::description() const
{
    return "The window is a pass the gallery opened before calling draw(), so "
           "compages::gpu::clear() is all it takes to paint it. A pass is also "
           "a region: two more are opened here, one per half of the window. No "
           "buffer, no shader: the counters below stay at zero.";
}

// ----------------------------------------------------------------------------
compages::Status ClearScreen::setUp()
{
    // Nothing to build: clearing costs nothing and holds nothing.
    return compages::success();
}

// ----------------------------------------------------------------------------
void ClearScreen::draw(compages::world::ViewFrame const& p_frame)
{
    // A pulse makes the two halves of the window change colour in opposite
    // directions. The sine is -1 to +1, so scale and shift it to 0 to 1. The
    // pulse is faster than the frame counter, so that the two halves change
    // colour at different speeds.
    const float pulse = 0.5f + (0.5f * std::sin(p_frame.total * 1.5f));

    // The whole window, in one call.
    compages::gpu::clear({ 0.05f, 0.05f, 0.08f });

    // A pass says where it draws and what it starts from. Opened over the
    // window, it suspends it, and closing it (the end of the scope) resumes
    // the window as it was left.
    const std::uint32_t half = p_frame.width / 2u;
    {
        compages::gpu::RenderPass left(
            { .width = half,
              .height = p_frame.height,
              .color = { 0.1f, 0.1f + (0.6f * pulse), 0.3f, 1.0f } });
    }
    {
        compages::gpu::RenderPass right(
            { .x = half,
              .width = p_frame.width - half,
              .height = p_frame.height,
              .color = { 0.3f, 0.1f, 0.7f - (0.6f * pulse), 1.0f } });
    }
}

} // namespace examples
