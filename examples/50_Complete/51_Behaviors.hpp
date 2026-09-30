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
//! \brief Code attached to an entity, the Unity way.
//!
//! A behavior is a small class deriving from compages::world::Behavior. Added
//! to an entity, it is started once, then updated every frame the entity is
//! enabled, and it reaches its entity, its transform and the input:
//! \code
//! struct Spin : compages::world::Behavior
//! {
//!     explicit Spin(float p_speed) : speed(p_speed) {}
//!     void update(float p_dt) override { transform().rotateY(speed * p_dt); }
//!     float speed;
//! };
//!
//! m_scene.box("Cube").add<Spin>(2.0f);    // built with Spin(2.0f)
//! \endcode
//!
//! The camera control is a behavior too: add<compages::world::Orbit>().
// ****************************************************************************
class Behaviors final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "51_Behaviors";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    struct Spin;
    struct Bob;
    struct GrowOnSpace;

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    //! \brief The button of the Try it panel is held.
    bool m_grow = false;
};

} // namespace examples
