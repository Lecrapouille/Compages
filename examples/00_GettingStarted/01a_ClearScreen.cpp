//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "00_GettingStarted/01a_ClearScreen.hpp"

#include <cmath>

namespace examples
{

//------------------------------------------------------------------------------
std::string ClearScreen::description() const
{
    return "The window is a pass the gallery opened before calling draw(), so "
           "compages::gpu::clear() is all it takes to paint it. A pass is also "
           "a region: "
           "two more are opened here, one per half of the window. No buffer, "
           "no "
           "shader: the counters below stay at zero.";
}

//------------------------------------------------------------------------------
compages::Status ClearScreen::setUp()
{
    // Nothing to build: clearing costs nothing and holds nothing.
    return compages::success();
}

//------------------------------------------------------------------------------
void ClearScreen::draw(compages::world::ViewFrame const& p_frame)
{
    const float pulse = 0.5f + (0.5f * std::sin(p_frame.total * 1.5f));
    const std::uint32_t half = p_frame.width / 2u;

    // The whole window, in one call.
    compages::gpu::clear({ 0.05f, 0.05f, 0.08f });

    // A pass says where it draws and what it starts from. Opened over the
    // window, it suspends it, and closing it (the end of the scope) resumes
    // the window as it was left.
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
