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
//! \brief Steer the camera two ways, and click a cube to select it.
//!
//! A camera control is a behavior; changing the way of steering is taking one
//! off and putting another on:
//! \code
//! m_camera.remove<compages::world::Orbit>().add<compages::world::Fly>();
//! \endcode
//!
//! A click asks the Scene what is under the mouse, as the last frame drew it,
//! and a selection is only a change of look:
//! \code
//! if (auto hit = m_scene.pick(p_frame.input.mouse))
//!     m_scene.look(hit->entity, compages::renderer::color(0.95f, 0.72f, 0.22f));
//! \endcode
// ****************************************************************************
class CameraPick final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "32b_CameraPick";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    void select(compages::world::EntityId p_entity);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_camera;
    compages::world::EntityId m_selection;
    bool m_auto_picked = false;
};

} // namespace examples
