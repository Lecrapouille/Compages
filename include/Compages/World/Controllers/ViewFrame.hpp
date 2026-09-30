// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Frame.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/World/Controllers/Input.hpp"

namespace compages::world
{



// ****************************************************************************
//! \brief One interactive frame: timing, viewport size and user input.
//!
//! Used by the gallery and \c compages::renderer::Scene. Headless simulation uses
//! \c compages::core::Frame alone; behaviors that read the mouse or keys need a \c ViewFrame
//! passed to \c World::update(ViewFrame).
//!
//! \code
//! compages::world::ViewFrame frame;
//! frame.width = 1280;
//! frame.height = 720;
//! frame.elapsed = 1.0f / 60.0f;
//! frame.input.set(compages::world::Key::W, true);
//! world.update(frame);
//! compages::world::aspect(frame);
//! m_scene.draw(frame);
//! \endcode
// ****************************************************************************
struct ViewFrame : compages::core::Frame
{
    //! \brief Mouse and keyboard for this frame (controllers and behaviors).
    Input input{};
};

// ****************************************************************************
//! \brief Mouse position in clip space (−1…1, y up), for picking and shaders.
//!
//! \code
//! compages::core::Vector2f clip = compages::world::mouseInClipSpace(frame);
//! \endcode
// ****************************************************************************
[[nodiscard]] inline compages::core::Vector2f mouseInClipSpace(ViewFrame const& p_view)
{
    if ((p_view.width == 0u) || (p_view.height == 0u))
    {
        return compages::core::Vector2f(0.0f, 0.0f);
    }
    return compages::core::Vector2f(((p_view.input.mouse.x / float(p_view.width)) * 2.0f) - 1.0f,
                    ((p_view.input.mouse.y / float(p_view.height)) * 2.0f) - 1.0f);
}

// ****************************************************************************
//! \brief Timing and viewport only (drops \c input), e.g. for a headless step
//! after an interactive \c ViewFrame.
// ****************************************************************************
[[nodiscard]] inline compages::core::Frame frameStep(ViewFrame const& p_view)
{
    return static_cast<compages::core::Frame const&>(p_view);
}

} // namespace compages::world
