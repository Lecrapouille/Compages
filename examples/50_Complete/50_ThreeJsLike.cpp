//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "50_Complete/50_ThreeJsLike.hpp"

namespace examples
{

std::string ThreeJsLike::description() const
{
    return "A shape, a camera and a light made by the Scene in one line each, "
           "then one draw() per frame. Right drag turns the camera, the wheel "
           "zooms.";
}

compages::Status ThreeJsLike::setUp()
{
    // Background, camera, light, shape: the whole scene.
    m_scene.background(0.04f, 0.05f, 0.08f);
    // Orbit starts from wherever the camera is: place it first.
    m_scene.camera().position(0.0f, 1.0f, 3.0f).add<compages::world::Orbit>();
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
    m_cube = m_scene.box("Cube", compages::renderer::color(0.9f, 0.18f, 0.12f));
    return m_scene.prepare();
}

void ThreeJsLike::draw(compages::world::ViewFrame const& p_frame)
{
    // The cube turns; the orbit behavior turns the camera from the mouse.
    m_cube.rotate(Radians(p_frame.elapsed), { 0.4f, 1.0f, 0.0f });
    m_scene.draw(p_frame);
}

} // namespace examples
