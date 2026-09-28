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

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/Renderer/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A glTF model loaded, placed and framed in three lines.
//!
//! load() reads the file once into a prefab, and places it in the World.
//! frameAll() puts a camera and a sun where they see it whole, whatever its
//! size:
//! \code
//! COMPAGES_TRY(m_scene.load(dataPath("Duck.glb")));
//! const Vector3f middle = m_scene.frameAll();
//! m_scene.activeCamera().add<compages::world::Orbit>(middle);   // turn around it
//! \endcode
// ****************************************************************************
class GltfModel final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "34_GltfModel";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
