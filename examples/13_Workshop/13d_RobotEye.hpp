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
//! \brief A camera on a mast of the arm, drawn in a corner and read by the
//! program (doc/tutorials/07-what-the-robot-sees.md).
// ****************************************************************************
class WorkshopEye final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "13d_RobotEye";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    [[nodiscard]] std::string hud() const override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    workshop::Arm m_arm;
    workshop::HotPlate m_plate;
    workshop::RobotEye m_eye;
};

} // namespace examples
