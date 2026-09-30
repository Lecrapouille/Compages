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

#include <string>

namespace examples
{

// ****************************************************************************
//! \brief One robot described once as a prefab, placed three times, and the
//! World saved to a file.
//!
//! A prefab is a tree of entities kept as an asset. It names what it draws
//! with rather than holding it, so that it can be written to a file and read
//! back: the assets it names are registered first.
//! \code
//! m_scene.shapeMesh(compages::renderer::Shape::Box);                    //
//! "box" m_scene.material("wood", compages::renderer::color(0.62f, 0.42f,
//! 0.24f)); COMPAGES_TRY_ASSIGN(robot, m_scene.assets().addPrefab("robot",
//! compages::renderer::makeRobotPrefab())); COMPAGES_TRY_ASSIGN(first,
//! m_scene.instantiate(robot));
//! ...
//! compages::renderer::saveScene(m_scene, "/tmp/compages_prefab_scene.json");
//! \endcode
// ****************************************************************************
class PrefabAndSave final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "35_PrefabAndSave";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    bool m_saved = false;
};

} // namespace examples
