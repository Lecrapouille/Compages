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
//! \brief Read a field under the mouse: the altitude of an island, written
//! next to the point.
//!
//! \code
//! if (auto found = m_scene.probe(m_altitude, m_look, p_frame.input.mouse))
//! {
//!     // found->point: where on the relief, found->value: the altitude
//! }
//! \endcode
// ****************************************************************************
class FieldProbe final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "11c_FieldProbe";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::Field<float> m_altitude;
    compages::renderer::FieldLook m_look{ .colors = compages::renderer::ColorMap::Terrain,
                                          .low = -400.0f,
                                          .high = 400.0f,
                                          .relief = 1.0f,
                                          .lit = true };
    compages::world::Entity m_island;
    //! \brief Carries the tooltip, at the point under the mouse.
    compages::world::Entity m_probe;
};

} // namespace examples
