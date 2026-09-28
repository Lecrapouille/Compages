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

#pragma once

#include "Compages/Core/Vector.hpp"

namespace compages::renderer
{

// ****************************************************************************
//! \brief Presentation-level knobs a Scene carries for the renderer.
//!
//! These belong to a given view of the World, not to the World itself. Two
//! Scenes observing the same World can have different clear colours.
// ****************************************************************************
struct RenderSettings
{
    //! \brief What the framebuffer starts from before the frame draws.
    Vector4f clear_color{ 0.0f, 0.0f, 0.1f, 1.0f };
    //! \brief Whether frustum culling is enabled. Disable it to compare the
    //! cost of the cull against the cost of drawing more objects.
    bool frustum_culling = true;
};

} // namespace compages::renderer
