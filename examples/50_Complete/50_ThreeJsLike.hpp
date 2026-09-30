// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
