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
class MiscLookAt final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "32c_MiscLookAt";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_camera;
    compages::world::Entity m_sphere;
    std::vector<compages::world::Entity> m_cones;
};

} // namespace examples
