// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A car and its wheels: children move with their parent.
//!
//! The car itself is an entity without a shape, a pivot. The body and the
//! wheels hang from it with parent() and keep their place relative to it.
//! Moving the car moves all of them; nothing else is updated:
//! \code
//! m_scene.cylinder("Wheel", black).parent(m_car).position(-0.8f, 0.3f, 0.6f);
//! m_car.position(x, 0, z).rotation(angle, { 0, 1, 0 });
//! \endcode
// ****************************************************************************
class Hierarchy final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06c_Hierarchy";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_car;
};

} // namespace examples
