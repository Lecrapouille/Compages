// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "00_Workshop/Workshop/Workshop.hpp"
#include "Common/Example.hpp"

namespace examples
{

// ****************************************************************************
//! \brief Step 2: an arm of three joints, moved by a routine
//! (doc/tutorials/03-make-it-move.md).
// ****************************************************************************
class WorkshopArm final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "00b_RobotArm";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    workshop::Arm m_arm;
};

} // namespace examples
