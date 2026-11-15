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

namespace examples
{

// ****************************************************************************
//! \brief Filters of the picture of one view: a few lines of GLSL each, run
//! one after the other on the light of the view before the tone curve.
//!
//! \code
//! camera.set(compages::renderer::ScreenEffects{ .filters = {
//!     { .name = "Sepia", .glsl = SEPIA, .values = { { "amount", 0.8f } } },
//!     { .name = "Vignette", .glsl = VIGNETTE, .values = { { "strength", 0.6f } } } } });
//! \endcode
// ****************************************************************************
class ScreenFilters final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "12a_ScreenFilters";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    void applyFilters();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_plain;
    compages::world::Entity m_filtered;
    bool m_edges = true;
    bool m_sepia = true;
    bool m_vignette = true;
    float m_amount = 0.8f;
    float m_strength = 0.7f;
};

} // namespace examples
