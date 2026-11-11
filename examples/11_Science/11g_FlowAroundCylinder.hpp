// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Fields/Field.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief A field of vectors: the wind around a column, as arrows, as
//! streamlines, over the pressure it makes.
//!
//! \code
//! m_scene.show(m_wind, m_arrows_look, "Wind");               // a Field<Vector2f>
//! for (Vector2f p : compages::streamline(m_wind, start, 0.05f, 400)) { ... }
//! \endcode
// ****************************************************************************
class FlowAroundCylinder final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11g_FlowAroundCylinder";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief The wind and the pressure for the speed and the turn.
    void blow();

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::Field<compages::Vector2f> m_wind;
    compages::Field<float> m_pressure;
    compages::renderer::ArrowLook m_arrows_look{ .colors = compages::renderer::ColorMap::Viridis,
                                                 .low = 0.0f,
                                                 .high = 2.0f,
                                                 .scale = 0.12f,
                                                 .every = 4 };
    compages::renderer::FieldLook m_pressure_look{ .colors = compages::renderer::ColorMap::Coolwarm,
                                                   .low = -2.0f,   // the free wind (0) in the middle
                                                   .high = 2.0f,
                                                   .relief = 0.0f,
                                                   .lit = false };
    compages::world::Entity m_arrows;
    compages::world::Entity m_floor;
    std::vector<std::vector<compages::Vector2f>> m_lines;
    float m_speed = 1.0f;
    float m_turn = 0.0f;
    bool m_show_arrows = true;
    bool m_show_lines = true;
};

} // namespace examples
