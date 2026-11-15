// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "13_Workshop/Workshop/Workshop.hpp"
#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief The heat the tip leaves on a steel plate, a Field on a Grid2D
//! (doc/tutorials/05-values-on-a-grid.md).
// ****************************************************************************
class WorkshopHeat final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "13b_HotPlate";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    workshop::Arm m_arm;
    workshop::HotPlate m_plate;
};

} // namespace examples
