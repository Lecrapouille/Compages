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

namespace examples
{

// ****************************************************************************
//! \brief Code attached to an entity, the Unity way.
//!
//! A behavior is a small class deriving from compages::world::Behavior. Added to an
//! entity, it is started once, then updated every frame the entity is
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
class Behaviors final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "51_Behaviors";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;
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
