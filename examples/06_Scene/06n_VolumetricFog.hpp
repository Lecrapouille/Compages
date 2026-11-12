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
//! \brief Fog that the light crosses: shafts of the sun between the slats
//! of a roof, halos and shadows of moving lamps in the air.
//!
//! \code
//! compages::renderer::VolumetricFog& fog = m_scene.environment().volumetric;
//! fog.enabled = true;
//! fog.density = 0.06f;      // per metre
//! fog.anisotropy = 0.6f;    // mostly forward: bright toward the lights
//! \endcode
// ****************************************************************************
class VolumetricFogDemo final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06n_VolumetricFog";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_red;
    compages::world::Entity m_blue;
    float m_time = 0.0f;
};

} // namespace examples
