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
//! \brief Place, turn and size a shape: three calls that chain.
//!
//! position(), rotation() and scale() set a value; rotate() adds to the
//! current turn. All return the entity, so a shape is made and placed in one
//! statement:
//! \code
//! m_scene.cone("Cone", color).position(1.5f, 0, 0).scale(0.6f, 1.2f, 0.6f);
//! \endcode
// ****************************************************************************
class Transforms final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06b_Transforms";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_box;
    compages::world::Entity m_ball;
    compages::world::Entity m_cone;
};

} // namespace examples
