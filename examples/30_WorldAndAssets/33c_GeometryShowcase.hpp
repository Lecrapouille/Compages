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
//! \brief Every built-in shape, and every built-in look, on one plateau.
//!
//! The shapes are one call each and all stand one unit tall; the looks are a
//! lit colour, a colour through the textured shader, the depth and the
//! normals:
//! \code
//! m_scene.cone("cone", compages::renderer::color(0.92f, 0.55f, 0.18f));
//! m_scene.box("depth", compages::renderer::depth(8.0f, 22.0f));
//! m_scene.sphere("normals", compages::renderer::normals());
//! \endcode
//!
//! A mesh the Scene has no shortcut for, a tube here, is built by a make
//! function and given to mesh().
// ****************************************************************************
class GeometryShowcase final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "33c_GeometryShowcase";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<compages::world::Entity> m_props;
};

} // namespace examples
