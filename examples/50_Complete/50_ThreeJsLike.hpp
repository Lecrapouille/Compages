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

namespace examples
{

// ****************************************************************************
//! \brief A lit, turning cube in a dozen lines: the Three.js way.
//!
//! A World holds the entities; a Scene shows it. The Scene makes the common
//! things one line each, a shape, a camera, a light, and one call draws a
//! frame:
//! \code
//! m_scene.camera().position(0, 1, 3).add<compages::world::Orbit>();
//! m_scene.sun();
//! m_cube = m_scene.box("Cube", compages::renderer::color(0.9f, 0.2f, 0.1f));
//! ...
//! m_cube.rotate(p_frame.elapsed, { 0.4f, 1, 0 });
//! m_scene.draw(p_frame);
//! \endcode
//!
//! What the entity returned by box() is: a handle to the World, where every
//! method chains, position(), rotate(), set<Component>(), add<Behavior>().
// ****************************************************************************
class ThreeJsLike final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "50_ThreeJsLike";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_cube;
};

} // namespace examples
