// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Common/Example.hpp"

#include "Compages/Renderer/Assets/CanvasTexture.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/World/Controllers/Input.hpp"

namespace examples
{

// ****************************************************************************
//! \brief A vertical easel: left drag paints strokes on a dynamic texture.
//!
//! \c CanvasTexture keeps RGBA8 pixels on the CPU, stamps circular brushes,
//! and uploads dirty regions with \c Texture::write. The plane wears the
//! texture through \c Scene::look(entity, texture_id).
// ****************************************************************************
class CanvasPaint final: public Example
{
public:

    [[nodiscard]] std::string name() const override
    {
        return "39a_CanvasPaint";
    }

    [[nodiscard]] std::string description() const override;
    [[nodiscard]] compages::Status setUp() override;
    void draw(compages::world::ViewFrame const& p_frame) override;
    void controls() override;

private:

    static compages::renderer::CanvasTexture::Color swatchColor(int p_index);

    compages::world::World m_world;
    compages::renderer::Scene m_scene{ m_world };
    compages::world::Entity m_canvas;
    compages::renderer::CanvasTexture m_paint;
    compages::core::Vector3f m_last_stroke{};
    bool m_has_last_stroke = false;
    compages::world::Input m_prev_input{};
    int m_color_index = 2;
    float m_brush_radius = 0.035f;
};

} // namespace examples
