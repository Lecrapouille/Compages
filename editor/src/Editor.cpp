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

#include "Editor.hpp"
#include "Loaders/Textures/SOIL.hpp"
#include <iostream>

float Editor::color[4] = { 0.5f, 0.5f, 1.0f, 1.0f };

//------------------------------------------------------------------
static bool loadTexture(GLTexture2D& texture, std::string const& path)
{
    if (!texture.load<SOIL>(path))
    {
        std::cerr << "Failed loading texture: " << path << std::endl;
        return false;
    }
    texture.begin();
    return true;
}

//------------------------------------------------------------------------------
Editor::Editor(uint32_t const width, uint32_t const height, const char* title)
    : GLWindow(width, height, title), m_texture("texture")
{
    std::cout << "Hello Editor" << std::endl;
}

//------------------------------------------------------------------------------
Editor::~Editor()
{
    std::cout << "Bye Editor" << std::endl;
}

//------------------------------------------------------------------
bool Editor::onSetup()
{
    if (!loadTexture(m_texture, "../examples/external/assets/hazard.png"))
        return false;

    m_layers.push_back(std::make_unique<Editor::GUI>(*this));
    return true;
}

//------------------------------------------------------------------
//! \brief Paint our scene. Here we are using the delta time to
//------------------------------------------------------------------
bool Editor::onPaint()
{
    // First draw your OpenGL scene
    glCheck(glClear(GL_COLOR_BUFFER_BIT));
    glCheck(glClearColor(color[0], color[1], color[2], color[3]));

    // Success
    return true;
}

//------------------------------------------------------------------
//! \brief Paint some DearImGui widgets. In this example change the
//! background color.
//------------------------------------------------------------------
bool Editor::GUI::onImGuiRender()
{
    Editor& win = owner<Editor>();

    static bool dockspace_open = true;
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent
    // window not dockable into, because it would be confusing to have two
    // docking targets within each others.
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar |
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus |
                        ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }

    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will
    // render our background and handle the pass-thru hole, so we ask Begin() to
    // not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    // Important: note that we proceed even if Begin() returns false (aka window
    // is collapsed). This is because we want to keep our DockSpace() active. If
    // a DockSpace() is inactive, all active windows docked into it will lose
    // their parent and become undocked. We cannot preserve the docking
    // relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in
    // limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", &dockspace_open, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();

    if (opt_fullscreen)
        ImGui::PopStyleVar(2);

    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Exit"))
                win.halt();
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }

    // QQ
    ImGui::Begin("Hello, world!");
    ImGui::TextColored(
        ImVec4(win.color[0], win.color[1], win.color[2], win.color[3]),
        "%s",
        "Change the background color");
    ImGui::ColorEdit3("color", win.color);

    // https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples

    ImGui::Image((void*)win.m_texture.handle(), ImVec2(128, 128));
    ImGui::End();

    ImGui::End();

    // Success
    return true;
}

//------------------------------------------------------------------------------
void Editor::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void Editor::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
