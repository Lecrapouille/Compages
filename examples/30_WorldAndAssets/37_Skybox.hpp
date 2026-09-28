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
//! \brief Six pictures around the scene, as far away as the sky.
//!
//! A skybox is a cube map drawn around the camera before anything else. It
//! turns with the camera but never comes closer, which is what makes it look
//! infinitely far:
//! \code
//! m_scene.skybox({ dataPath("right.jpg"), dataPath("left.jpg"),
//!                  dataPath("top.jpg"), dataPath("bottom.jpg"),
//!                  dataPath("front.jpg"), dataPath("back.jpg") });
//! \endcode
// ****************************************************************************
class Skybox final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "37_Skybox";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_cube;
};

} // namespace examples
