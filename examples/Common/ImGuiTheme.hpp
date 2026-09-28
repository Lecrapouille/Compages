// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <imgui.h>

namespace examples::ui
{

inline void applyGalleryTheme()
{
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding and padding, so the docked panels read as one editor.
    style.WindowRounding = 6.0f;
    style.ChildRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(8.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 4.0f);
    style.IndentSpacing = 18.0f;
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f);

    // A dark surface and one blue accent, used for hover, active and the
    // docking preview.
    ImVec4* const colors = style.Colors;
    const ImVec4 background(0.09f, 0.10f, 0.12f, 0.94f);
    const ImVec4 surface(0.13f, 0.14f, 0.17f, 1.00f);
    const ImVec4 surface_high(0.17f, 0.19f, 0.23f, 1.00f);
    const ImVec4 accent(0.34f, 0.61f, 0.84f, 1.00f);
    const ImVec4 accent_dim(0.34f, 0.61f, 0.84f, 0.45f);
    const ImVec4 text(0.90f, 0.92f, 0.95f, 1.00f);

    colors[ImGuiCol_WindowBg] = background;
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = surface;
    colors[ImGuiCol_MenuBarBg] = surface;
    colors[ImGuiCol_Border] = ImVec4(1.0f, 1.0f, 1.0f, 0.08f);
    colors[ImGuiCol_Text] = text;
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.53f, 0.58f, 1.00f);
    colors[ImGuiCol_FrameBg] = surface;
    colors[ImGuiCol_FrameBgHovered] = surface_high;
    colors[ImGuiCol_FrameBgActive] = accent_dim;
    colors[ImGuiCol_TitleBg] = surface;
    colors[ImGuiCol_TitleBgActive] = surface_high;
    colors[ImGuiCol_Header] = surface_high;
    colors[ImGuiCol_HeaderHovered] = accent_dim;
    colors[ImGuiCol_HeaderActive] = accent;
    colors[ImGuiCol_Button] = surface_high;
    colors[ImGuiCol_ButtonHovered] = accent_dim;
    colors[ImGuiCol_ButtonActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_Separator] = ImVec4(1.0f, 1.0f, 1.0f, 0.10f);
    colors[ImGuiCol_Tab] = surface;
    colors[ImGuiCol_TabHovered] = accent_dim;
    colors[ImGuiCol_TabActive] = surface_high;
    colors[ImGuiCol_DockingPreview] = accent_dim;
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
}

} // namespace examples::ui
