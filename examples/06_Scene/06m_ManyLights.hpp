// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Scene/Scene.hpp"

#include <vector>

namespace examples
{

// ****************************************************************************
//! \brief Two hundred lamps in one picture: clustered lighting.
//!
//! Each lamp is an ordinary lamp of the scene: the four nearest the camera
//! get their own uniforms (and shadows when asked); the Renderer sorts the
//! others into clusters (LightClusters: tiles of the picture times slices of
//! the depth), and each pixel reads only the lamps of its cluster.
//! \code
//! for (int i = 0; i < 200; ++i)
//! {
//!     m_scene.lamp("Firefly", colour, 1.5f, 2.5f).position(x, 0.4f, z);
//! }
//! \endcode
// ****************************************************************************
class ManyLights final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "06m_ManyLights";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    //! \brief A lamp flying a loop of its own over the floor.
    struct Firefly
    {
        compages::world::Entity lamp;
        float radius = 1.0f;
        float speed = 1.0f;
        float phase = 0.0f;
        float height = 0.5f;
        compages::Vector2f centre{ 0.0f, 0.0f };
    };

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    std::vector<Firefly> m_flies;
    int m_shown = 200;
    float m_range = 2.5f;
    bool m_moving = true;
    float m_time = 0.0f;
};

} // namespace examples
