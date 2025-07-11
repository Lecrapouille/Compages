//=====================================================================
// OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
// Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of OpenGLCppWrapper.
//
// OpenGLCppWrapper is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OpenGLCppWrapper is distributedin the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
//=====================================================================

// *****************************************************************************
//! \file This file just gathers imgui cpp files and around them with pragma
//! for reducing compilation warnings. Yep this is misery!
// *****************************************************************************

#include "UI/DearImGui.hpp"

void DearImGuiLayer::setFont()
{
    ImGuiIO& io = ImGui::GetIO();
    float fontSize = 18.0f; // *2.0f;
    io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf",
                                 fontSize);
    io.FontDefault = io.Fonts->AddFontFromFileTTF(
        "assets/fonts/opensans/OpenSans-Regular.ttf", fontSize);
}

void DearImGuiLayer::theme(Theme const style)
{
    if (style == DearImGuiLayer::Theme::Classic)
    {
        ImGui::StyleColorsClassic();
    }
    else
    {
        ImGui::StyleColorsDark();
    }
}

void DearImGuiLayer::reactTo(GLWindow::Event const events)
{
    ImGuiIO& io = ImGui::GetIO();
    if ((events & GLWindow::Event::Keyboard) == GLWindow::Event::Keyboard)
        io.ConfigFlags |=
            ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
                                                        // / Platform Windows
    // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable
    // Gamepad Controls io.ConfigFlags |=
    // ImGuiConfigFlags_ViewportsNoTaskBarIcons; io.ConfigFlags |=
    // ImGuiConfigFlags_ViewportsNoMerge;
}

// setup
bool DearImGuiLayer::onSetup()
{
    ImGui::CreateContext();
    theme(DearImGuiLayer::Theme::Dark);
    reactTo(GLWindow::Event::Keyboard);
    ImGui_ImplGlfw_InitForOpenGL(&m_window.context(), true);
    ImGui_ImplOpenGL3_Init("#version 330");
    ImGui::StyleColorsDark(); // ImGui::StyleColorsClassic();

    return true;
}

bool DearImGuiLayer::onRelease()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    return true;
}

void DearImGuiLayer::begin()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    // ImGuizmo::BeginFrame();
}

void DearImGuiLayer::end()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(m_window.width<float>(), m_window.height<float>());
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

bool DearImGuiLayer::onPaint()
{
    begin();
    bool res = onImGuiRender();
    end();

    return res;
}
