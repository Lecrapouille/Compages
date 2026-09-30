// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

namespace compages::renderer
{

// ****************************************************************************
//! \brief Presentation-level knobs a Scene carries for the renderer.
//!
//! These belong to a given view of the compages::world::World, not to the compages::world::World itself. Two
//! Scenes observing the same compages::world::World can have different clear colours.
// ****************************************************************************
struct RenderSettings
{
    //! \brief What the framebuffer starts from before the frame draws.
    compages::core::Vector4f clear_color{ 0.0f, 0.0f, 0.1f, 1.0f };
    //! \brief Whether frustum culling is enabled. Disable it to compare the
    //! cost of the cull against the cost of drawing more objects.
    bool frustum_culling = true;
};

} // namespace compages::renderer
