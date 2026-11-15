// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Components/ScreenEffects.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Light brighter than the screen spills round it (bloom): the
//! view is drawn in linear light, so a bulb can be seven times the white of
//! the screen, and only that excess makes a halo.
//!
//! \code
//! m_scene.camera().set(compages::renderer::ScreenEffects{
//!     .bloom = { .enabled = true, .threshold = 1.0f, .strength = 0.6f } });
//! \endcode
// ****************************************************************************
class BloomDemo final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "12b_Bloom";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_camera;
    std::vector<compages::world::Entity> m_lamps;
    float m_time = 0.0f;
};

} // namespace examples
