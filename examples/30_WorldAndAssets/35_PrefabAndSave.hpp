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
//! m_scene.shapeMesh(compages::renderer::Shape::Box);                    // "box"
//! m_scene.material("wood", compages::renderer::color(0.62f, 0.42f, 0.24f));
//! COMPAGES_TRY_ASSIGN(robot, m_scene.assets().addPrefab("robot", compages::renderer::makeRobotPrefab()));
//! COMPAGES_TRY_ASSIGN(first, m_scene.instantiate(robot));
//! ...
//! compages::renderer::saveScene(m_scene, "/tmp/compages_prefab_scene.json");
//! \endcode
// ****************************************************************************
class PrefabAndSave final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "35_PrefabAndSave";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    bool m_saved = false;
};

} // namespace examples
