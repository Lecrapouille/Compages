// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Workshop.hpp"

#include <cstddef>

using namespace compages::si::literals;

namespace examples::workshop
{

// ----------------------------------------------------------------------------
//! The camera stands on a mast on the base of the arm and looks down at
//! where the tip works: it turns with the arm. It does not see the helpers
//! drawn for the person at the screen (the grid on the floor), as a real
//! camera would not.
// ----------------------------------------------------------------------------
compages::Status RobotEye::create(compages::renderer::Scene& p_scene, Arm const& p_arm)
{
    // The mast and the box of the camera, children of the base.
    p_scene.cylinder("Mast", compages::renderer::color(0.25f, 0.25f, 0.27f))
        .parent(p_arm.base)
        .position(-0.1f, 0.375f, 0.0f)
        .scale(0.03f, 0.75f, 0.03f);
    p_scene.box("Camera box", compages::renderer::color(0.1f, 0.1f, 0.1f)).parent(p_arm.base).position(-0.1f, 0.77f, 0.0f).scale(0.08f);
    // lookAt() takes a point in the axes of the parent: the work area, in
    // front of the base.
    m_camera = p_scene.camera("Robot eye");
    m_camera.parent(p_arm.base).position(-0.04f, 0.77f, 0.0f).lookAt(0.7f, 0.0f, 0.0f);
    compages::world::Camera& lens = m_camera.get<compages::world::Camera>();
    lens.sees_helpers = false;
    lens.near_plane = 0.02f;
    lens.far_plane = 20.0f;
    COMPAGES_TRY_ASSIGN(m_picture, compages::renderer::RenderTarget::create(160u, 120u));
    return compages::success();
}

// ----------------------------------------------------------------------------
//! Four times a second, draw what the robot sees into its own picture and
//! count the blue pixels (the part). readColor() waits for the GPU: a
//! sensor reads at its own rate, not at each frame.
// ----------------------------------------------------------------------------
compages::Status RobotEye::look(compages::renderer::Scene& p_scene, compages::si::Seconds p_elapsed)
{
    m_wait -= p_elapsed;
    if (m_wait > 0.0_s)
    {
        return compages::success();
    }
    m_wait = 0.25_s;
    m_camera.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 1.0f, 1.0f }; // the whole picture
    p_scene.render(m_camera, m_picture);
    std::vector<std::byte> pixels;
    COMPAGES_TRY_ASSIGN(pixels, m_picture.readColor());
    std::size_t blue = 0u;
    for (std::size_t k = 0u; k + 3u < pixels.size(); k += 4u)
    {
        const int r = int(pixels[k]);
        const int g = int(pixels[k + 1u]);
        const int b = int(pixels[k + 2u]);
        blue += ((b > 100) && (b > r + 60) && (b > g + 40)) ? 1u : 0u;
    }
    m_seen = float(blue) / float(m_picture.width() * m_picture.height());
    return compages::success();
}

// ----------------------------------------------------------------------------
void RobotEye::show(compages::renderer::Scene& p_scene)
{
    // In the window: a corner at the top right, a third of the width.
    m_camera.get<compages::world::Camera>().viewport = { 0.66f, 0.64f, 0.32f, 0.34f };
    p_scene.render(m_camera);
}

} // namespace examples::workshop
