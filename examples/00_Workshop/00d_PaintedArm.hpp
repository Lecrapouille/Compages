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
//! \brief Step 4: warning stripes on the arm, from the hook "surface" of the
//! PBR shader (doc/tutorials/05-your-own-look.md).
// ****************************************************************************
class WorkshopPaint final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "00d_PaintedArm";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    workshop::Arm m_arm;
    workshop::HotTable m_plate;
};

} // namespace examples
