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
#include "Compages/Renderer/Visualization/Chart.hpp"

#include <array>

namespace examples
{

// ****************************************************************************
//! \brief Follow values over time: two thermometers on a heated plate, and
//! their curves on a board.
//!
//! \code
//! m_chart.add(m_curves[k], m_time, m_temperature.sample(m_spots[k]));
//! COMPAGES_TRY(m_chart.draw());      // the picture on the board
//! \endcode
// ****************************************************************************
class Thermometers final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11h_Thermometers";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    void spread(float p_seconds);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::Field<float> m_temperature;
    compages::Field<float> m_next;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Inferno,
                                          .low = 0.0f,
                                          .high = 300.0f,
                                          .relief = 0.0f,
                                          .lit = true };
    compages::world::Entity m_plate;
    compages::renderer::Chart m_chart;
    //! \brief Where the two thermometers are (x, z), their marks, their curves.
    std::array<compages::Vector2f, 2> m_spots{ compages::Vector2f(0.4f, 0.4f), compages::Vector2f(-0.6f, 0.1f) };
    std::array<compages::world::Entity, 2> m_marks;
    std::array<std::size_t, 2> m_curves{};
    float m_time = 0.0f;
    float m_speed = 60.0f;
};

} // namespace examples
