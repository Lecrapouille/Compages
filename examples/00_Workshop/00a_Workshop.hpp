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
//! \brief The workshop of the tutorials, step 1: a table, a part, a lamp
//! (doc/tutorials/02-first-scene.md).
// ****************************************************************************
class WorkshopRoom final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "00a_Workshop";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
};

} // namespace examples
