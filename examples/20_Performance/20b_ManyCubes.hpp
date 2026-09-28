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
//! \brief Seventeen hundred cubes in five colours: one mesh, five looks.
//!
//! A copy shares the mesh and the look of what it copies, so the whole field
//! costs one mesh and five sets of shader parameters, however many cubes:
//! \code
//! compages::world::Entity sky = m_scene.box("sky", compages::renderer::color(0.35f, 0.55f, 0.85f));
//! m_scene.copy(sky).position(x, y, z);
//! \endcode
//!
//! Two things happen behind draw(). The cubes are sorted by look, so that
//! the shader parameters change five times a frame rather than at every
//! cube; and the cubes outside the view are not drawn at all, which the
//! draw-call counter of the overlay shows as the camera turns.
// ****************************************************************************
class ManyCubes final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "20b_ManyCubes";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
