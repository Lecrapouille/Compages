// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Another model of light: flat bands of a cartoon (hook "light" of
//! the PBR shader), and a black outline (a shell drawn inside out).
//!
//! \code
//! auto pbr = compages::renderer::shaders::pbr();
//! pbr.fragment().hook("light", BANDS);   // vec3 light(Surface s) { ... }
//! m_cel = m_scene.assets().createMaterial("Cel", { .shaders = pbr, ... });
//! m_scene.look(entity, m_cel);           // keeps the colour of the entity
//! \endcode
// ****************************************************************************
class CelShading final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06o_CelShading";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    [[nodiscard]] compages::Status makeMaterials();
    void add(compages::renderer::Shape p_shape, compages::Vector3f p_color, compages::Vector3f p_position);
    void useCartoon(bool p_cartoon);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    //! The bands of the cartoon, the PBR shader as it is, the outline.
    compages::renderer::MaterialId m_cel;
    compages::renderer::MaterialId m_plain;
    compages::renderer::MaterialId m_outline;
    //! The shapes, and their shells drawn inside out.
    std::vector<compages::world::Entity> m_shapes;
    std::vector<compages::world::Entity> m_shells;
    std::vector<compages::renderer::MeshRenderer> m_inks;
    bool m_cartoon = true;
    int m_tones = 3;
    float m_time = 0.0f;
};

} // namespace examples
