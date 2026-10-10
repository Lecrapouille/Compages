// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/39a_CanvasPaint.hpp"

#include "Common/Gui.hpp"

#include "Compages/GPU/Errors.hpp"

namespace examples
{

compages::renderer::CanvasTexture::Color CanvasPaint::swatchColor(int p_index)
{
    switch (p_index)
    {
        case 0:
            return compages::renderer::CanvasTexture::Color::rgb(210u, 45u, 45u);
        case 1:
            return compages::renderer::CanvasTexture::Color::rgb(25u, 25u, 30u);
        case 3:
            return compages::renderer::CanvasTexture::Color::rgb(220u, 170u, 40u);
        case 2:
        default:
            return compages::renderer::CanvasTexture::Color::rgb(25u, 45u, 120u);
    }
}

std::string CanvasPaint::description() const
{
    return "Left drag paints on a canvas texture. Pick a colour in Try it "
           "(Space clears). The brush follows the raycast hit on the easel.";
}

compages::Status CanvasPaint::setUp()
{
    m_scene.background(0.10f, 0.12f, 0.16f).ambient(0.18f, 0.18f, 0.20f);
    m_scene.sun();
    m_scene.camera()
        .position(0.0f, 1.6f, 4.5f)
        .add<compages::world::Orbit>(
            compages::core::Vector3f(0.0f, 1.2f, 0.0f));

    m_scene.box("Easel", compages::renderer::color(0.45f, 0.32f, 0.22f))
        .position(-0.05f, 1.0f, 0.0f)
        .scale(0.08f, 2.0f, 0.08f);
    m_scene.box("Easel", compages::renderer::color(0.45f, 0.32f, 0.22f))
        .position(0.05f, 1.0f, 0.0f)
        .scale(0.08f, 2.0f, 0.08f);
    m_scene.box("Tray", compages::renderer::color(0.35f, 0.26f, 0.18f))
        .position(0.0f, 0.35f, 0.25f)
        .scale(0.9f, 0.06f, 0.35f);

    m_canvas = m_scene.plane("Canvas", compages::renderer::color(0.95f, 0.95f, 0.93f))
                   .position(0.0f, 1.35f, 0.02f)
                   .scale(2.4f, 1.6f, 1.0f);

    COMPAGES_TRY(m_paint.create(512u, 512u));
    m_paint.setMapping(
        compages::core::AABB::fromCorners({ -0.5f, -0.5f, 0.0f },
                                          { 0.5f, 0.5f, 0.0f }));
    COMPAGES_TRY(m_paint.bind(m_scene.assets(), "canvas_paint"));
    m_paint.applyLook(m_scene, m_canvas);

    return m_scene.prepare();
}

void CanvasPaint::draw(compages::world::ViewFrame const& p_frame)
{
    if (compages::world::pressed(p_frame.input,
                                 m_prev_input,
                                 compages::world::Key::Space))
    {
        m_paint.clear();
        m_has_last_stroke = false;
    }

    m_scene.draw(p_frame);

    const compages::renderer::CanvasTexture::Color ink =
        swatchColor(m_color_index);

    if (p_frame.input.mouse_left)
    {
        if (auto hit = m_scene.pick(p_frame.input.mouse))
        {
            if (hit->entity == m_canvas.id())
            {
                if (m_has_last_stroke)
                {
                    m_paint.paintLineWorld(m_last_stroke,
                                           hit->point,
                                           m_canvas.worldMatrix(),
                                           ink,
                                           m_brush_radius);
                }
                else
                {
                    m_paint.paintWorld(hit->point,
                                       m_canvas.worldMatrix(),
                                       ink,
                                       m_brush_radius);
                }
                m_last_stroke = hit->point;
                m_has_last_stroke = true;
            }
        }
    }
    else
    {
        m_has_last_stroke = false;
    }

    if (!m_paint.sync())
    {
        compages::gpu::reportError("CanvasPaint: texture sync failed");
    }

    m_prev_input = p_frame.input;
}

void CanvasPaint::controls()
{
    ImGui::TextUnformatted("Paint colour");
    struct Swatch
    {
        char const* label;
        ImVec4 preview;
        int index;
    };
    static Swatch const swatches[] = {
        { "Red", { 0.82f, 0.18f, 0.18f, 1.0f }, 0 },
        { "Black", { 0.10f, 0.10f, 0.12f, 1.0f }, 1 },
        { "Blue", { 0.10f, 0.18f, 0.47f, 1.0f }, 2 },
        { "Gold", { 0.86f, 0.67f, 0.16f, 1.0f }, 3 },
    };

    for (Swatch const& swatch : swatches)
    {
        ImGui::PushID(swatch.index);
        const bool selected = (m_color_index == swatch.index);
        if (selected)
        {
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
            ImGui::PushStyleColor(ImGuiCol_Border,
                                  ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        }
        ImGui::PushStyleColor(ImGuiCol_Button, swatch.preview);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, swatch.preview);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, swatch.preview);
        if (ImGui::Button(swatch.label, ImVec2(72.0f, 28.0f)))
        {
            m_color_index = swatch.index;
        }
        ImGui::PopStyleColor(3);
        if (selected)
        {
            ImGui::PopStyleColor();
            ImGui::PopStyleVar();
        }
        ImGui::PopID();
        ImGui::SameLine();
    }
    ImGui::NewLine();

    ImGui::SliderFloat("Brush radius", &m_brush_radius, 0.01f, 0.08f, "%.3f m");

    if (ImGui::Button("Clear canvas"))
    {
        m_paint.clear();
        m_has_last_stroke = false;
    }
    ImGui::SameLine();
    ImGui::TextUnformatted("(or Space in the viewport)");
}

} // namespace examples
