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
//! \brief Ambient, sun and lamps: the three lights of a scene.
//!
//! The ambient light reaches everything the same way, the sun has a
//! direction only, a lamp lights around its position up to a range. A lamp is
//! an entity: it can move and carry children, here a small bulb.
//! \code
//! m_scene.ambient(0.05f, 0.05f, 0.08f);
//! m_scene.sun("Sun", { 1, 1, 1 }, 0.2f);
//! m_red = m_scene.lamp("Red", { 1, 0.2f, 0.1f }, 3.0f, 6.0f);
//! \endcode
// ****************************************************************************
class Lights final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06d_Lights";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_red;
    compages::world::Entity m_blue;
};

} // namespace examples
