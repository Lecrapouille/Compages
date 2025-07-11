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

#include "02_BasicWindowImGui.hpp"
#include <iostream>

float BasicWindowImGui::color[4] = { 0.5f, 0.5f, 1.0f, 1.0f };

//------------------------------------------------------------------------------
BasicWindowImGui::BasicWindowImGui(uint32_t const width,
                                   uint32_t const height,
                                   const char* title)
    : GLWindow(width, height, title)
{
    std::cout << "Hello BasicWindowImGui: " << info() << std::endl;
}

//------------------------------------------------------------------------------
BasicWindowImGui::~BasicWindowImGui()
{
    std::cout << "Bye BasicWindowImGui" << std::endl;
}

//------------------------------------------------------------------
bool BasicWindowImGui::onSetup()
{
    m_layers.push_back(std::make_unique<BasicWindowImGui::GUI>(*this));
    return true;
}

//------------------------------------------------------------------
//! \brief Paint our scene. Here we are using the delta time to
//------------------------------------------------------------------
bool BasicWindowImGui::onPaint()
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
bool BasicWindowImGui::GUI::onImGuiRender()
{
    BasicWindowImGui& win = owner<BasicWindowImGui>();

    ImGui::Begin("Hello, world!");
    ImGui::TextColored(
        ImVec4(win.color[0], win.color[1], win.color[2], win.color[3]),
        "%s",
        "Change the background color");
    ImGui::ColorEdit3("color", win.color);
    ImGui::End();

    // Success
    return true;
}

//------------------------------------------------------------------------------
void BasicWindowImGui::onSetupFailed(std::string const& reason)
{
    std::cerr << "Failure during the setup. Reason: " << reason << std::endl;
}

//------------------------------------------------------------------------------
void BasicWindowImGui::onPaintFailed(std::string const& reason)
{
    std::cerr << "Failure during rendering. Reason: " << reason << std::endl;
}
