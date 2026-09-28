// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/Renderer/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Two walkers going round, their gait made by a behavior.
//!
//! The same pattern as 31_MovingRobot, pushed to a walk cycle: hips and
//! shoulders are joints, the limbs hang under them, and a Walk behavior
//! swings them in opposite phase while it moves its walker along a circle.
//! One class of behavior, two walkers, each with its own radius and phase:
//! \code
//! makeWalker("A", blue).add<Walk>(3.1f, 0.0f);
//! makeWalker("B", red).add<Walk>(2.2f, 1.7f);
//! \endcode
// ****************************************************************************
class AnimatedModel final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "36a_AnimatedModel";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    struct Walk;

    compages::world::Entity makeWalker(char const* p_name, compages::renderer::Look const& p_shirt);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
