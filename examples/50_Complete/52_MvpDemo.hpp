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

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A small simulation: falling, bouncing cubes, a lamp going round,
//! and debug lines over them.
//!
//! Everything that moves is a behavior. The cubes fall under a gravity and
//! bounce on the floor, which is a behavior of a dozen lines rather than a
//! physics engine; the lamp turns around the scene. The debug lines are
//! drawn over the next frame and then forgotten:
//! \code
//! m_scene.update(p_frame);                  // move, then
//! for (compages::world::Entity& cube : m_cubes)       // outline where they are now
//!     m_scene.debug().box(UNIT, cube.worldMatrix(), { 1.0f, 0.8f, 0.2f });
//! m_scene.render();
//! \endcode
//!
//! The physics engine, when it comes back, will move the same entities.
// ****************************************************************************
class MvpDemo final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "52_MvpDemo";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    struct Spin;

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<compages::world::Entity> m_cubes;
};

} // namespace examples
