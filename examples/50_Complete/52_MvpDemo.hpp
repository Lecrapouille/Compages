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

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

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
//! for (compages::world::Entity& cube : m_cubes)       // outline where they
//! are now
//!     m_scene.debug().box(UNIT, cube.worldMatrix(), { 1.0f, 0.8f, 0.2f });
//! m_scene.render();
//! \endcode
//!
//! The physics engine, when it comes back, will move the same entities.
// ****************************************************************************
class MvpDemo final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "52_MvpDemo";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    struct Spin;

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<compages::world::Entity> m_cubes;
};

} // namespace examples
