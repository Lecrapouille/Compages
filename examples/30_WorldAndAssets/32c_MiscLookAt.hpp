// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Three.js \c misc_lookat: a thousand cones turning to face a moving
//! sphere.
//!
//! lookAt() turns an entity so that its -Z axis points at a place. The cone
//! is built with its tip along -Z, so a cone looking at the sphere points at
//! it. The thousand cones are copies of one: one mesh and one look, shared:
//! \code
//! COMPAGES_TRY_ASSIGN(cone, compages::renderer::makeCone(10, 0, 100, 12));
//! compages::world::Entity first = m_scene.mesh(std::move(cone));
//! for (...) m_cones.emplace_back(m_scene.copy(first).position(...).scale(s));
//! ...
//! for (compages::world::Entity& cone : m_cones) cone.lookAt(target);
//! \endcode
// ****************************************************************************
class MiscLookAt final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "32c_MiscLookAt";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_camera;
    compages::world::Entity m_sphere;
    std::vector<compages::world::Entity> m_cones;
};

} // namespace examples
