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
class SplitViews final : public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "32a_SplitViews";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::gpu::Status setUp() override;
    void draw(Frame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::renderer::Scene m_map{ m_world, m_scene.assets() };
    compages::world::Entity m_eye;
    compages::world::Entity m_top;
    compages::world::Entity m_pillar;
};

} // namespace examples
