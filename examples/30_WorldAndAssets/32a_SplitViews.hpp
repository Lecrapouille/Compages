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
//! \brief One World seen twice at once: a perspective view and a top-down
//! map, side by side.
//!
//! A camera says which part of the picture it draws into, its viewport, and
//! render() is given the camera:
//! \code
//! m_eye = m_scene.camera("Eye");
//! m_eye.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 0.5f, 1.0f };
//! ...
//! m_scene.update(p_frame);     // the World moves once
//! m_scene.render(m_eye);       // then is drawn twice
//! m_map.render(m_top);
//! \endcode
//!
//! The map is a second Scene over the same World, for its own background; it
//! shares the assets of the first, so nothing is built twice.
// ****************************************************************************
class SplitViews final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "32a_SplitViews";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::renderer::Scene m_map{ m_world, m_scene.assets() };
    compages::world::Entity m_eye;
    compages::world::Entity m_top;
    compages::world::Entity m_pillar;
};

} // namespace examples
