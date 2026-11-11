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

namespace examples
{

// ****************************************************************************
//! \brief Show a field: the temperature of a copper plate, as colours and as
//! a relief, in one call.
//!
//! The values live in a compages::Field<float> on a Grid2D; the CPU solves
//! the heat equation on it, then:
//! \code
//! m_scene.show(m_plate, m_temperature, m_look);   // each frame, new values
//! \endcode
//! The colours and the heights are made on the GPU from the values.
// ****************************************************************************
class HeatPlate final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11a_HeatPlate";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief Let the heat spread for p_seconds of the plate's time.
    void spread(float p_seconds, float p_time);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::Field<float> m_temperature;
    compages::Field<float> m_next;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Inferno,
                                          .low = 0.0f,
                                          .high = 300.0f,
                                          .relief = 0.0015f,
                                          .lit = true };
    compages::world::Entity m_plate;
    //! \brief Seconds of the plate per second of the screen.
    float m_speed = 200.0f;
    float m_time = 0.0f;
};

} // namespace examples
