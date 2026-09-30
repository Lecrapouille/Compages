// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Spheres wearing a colour or a picture.
//!
//! A look is given when the shape is made. A picture is loaded once, however
//! many shapes wear it, and a colour given with it tints it:
//! \code
//! m_scene.sphere("Red", compages::renderer::color(0.85f, 0.25f, 0.2f));
//! m_scene.sphere("Grass",
//! compages::renderer::texture(dataPath("grassFlowers.png")));
//! \endcode
// ****************************************************************************
class TexturedSpheres final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "33a_TexturedSpheres";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<compages::world::Entity> m_spheres;
};

} // namespace examples
