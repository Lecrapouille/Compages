// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Fields/Slice.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A volume, seen through and cut: the warm air of a room heated by
//! three moving heaters.
//!
//! \code
//! m_room = m_scene.show(m_heat, m_look, "Room");          // a Field<float, Grid3D>
//! m_cut = m_scene.show(compages::slice(m_heat, compages::Axis::Y, layer), m_cut_look, "Cut");
//! \endcode
// ****************************************************************************
class Volume final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11f_Volume";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief The temperature of the room at a time.
    void warm(float p_time);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::Field<float, compages::Grid3D> m_heat;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Inferno,
                                          .low = 22.0f,
                                          .high = 45.0f,
                                          .relief = 0.0f,
                                          .lit = false };
    compages::renderer::FieldLook m_cut_look{ .colors = compages::renderer::ColorMap::Viridis,
                                              .low = 20.0f,
                                              .high = 45.0f,
                                              .relief = 0.0f,
                                              .lit = false };
    compages::world::Entity m_room;
    compages::world::Entity m_cut;
    int m_layer = 6;
    bool m_moving = true;
    float m_time = 0.0f;
};

} // namespace examples
