// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "00_GettingStarted/01a_ClearScreen.hpp"

#include <cmath>

namespace examples
{

std::string ClearScreen::description() const
{
    return "The window is a pass the gallery opened before calling draw(), so "
           "compages::gpu::clear() is all it takes to paint it. A pass is also a region: "
           "two more are opened here, one per half of the window. No buffer, no "
           "shader: the counters below stay at zero.";
}

compages::gpu::Status ClearScreen::setUp()
{
    // Nothing to build: clearing costs nothing and holds nothing.
    return compages::gpu::success();
}

void ClearScreen::draw(Frame const& p_frame)
{
    const float pulse = 0.5f + (0.5f * std::sin(p_frame.total * 1.5f));
    const std::uint32_t half = p_frame.width / 2u;

    // The whole window, in one call.
    compages::gpu::clear({ 0.05f, 0.05f, 0.08f });

    // A pass says where it draws and what it starts from. Opened over the
    // window, it suspends it, and closing it (the end of the scope) resumes
    // the window as it was left.
    {
        compages::gpu::RenderPass left({ .width = half,
                               .height = p_frame.height,
                               .color = { 0.1f, 0.1f + (0.6f * pulse), 0.3f, 1.0f } });
    }
    {
        compages::gpu::RenderPass right({ .x = half,
                                .width = p_frame.width - half,
                                .height = p_frame.height,
                                .color = { 0.3f, 0.1f, 0.7f - (0.6f * pulse), 1.0f } });
    }
}

} // namespace examples
