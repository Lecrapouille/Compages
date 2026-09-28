// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "50_Complete/50_ThreeJsLike.hpp"

namespace examples
{

std::string ThreeJsLike::description() const
{
    return "A shape, a camera and a light made by the Scene in one line each, "
           "then one draw() per frame. Right drag turns the camera, the wheel "
           "zooms.";
}

compages::gpu::Status ThreeJsLike::setUp()
{
    // Background, camera, light, shape: the whole scene.
    m_scene.background(0.04f, 0.05f, 0.08f);
    // Orbit starts from wherever the camera is: place it first.
    m_scene.camera().position(0.0f, 1.0f, 3.0f).add<compages::world::Orbit>();
    m_scene.sun("Sun", { 1.0f, 0.95f, 0.85f }, 1.4f);
    m_cube = m_scene.box("Cube", compages::renderer::color(0.9f, 0.18f, 0.12f));
    return m_scene.prepare();
}

void ThreeJsLike::draw(Frame const& p_frame)
{
    // The cube turns; the orbit behavior turns the camera from the mouse.
    m_cube.rotate(p_frame.elapsed, { 0.4f, 1.0f, 0.0f });
    m_scene.draw(p_frame);
}

} // namespace examples
