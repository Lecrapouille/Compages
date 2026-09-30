// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/32b_CameraPick.hpp"

namespace examples
{

const compages::renderer::Look PLAIN =
    compages::renderer::color(0.55f, 0.62f, 0.78f);
const compages::renderer::Look PICKED =
    compages::renderer::color(0.95f, 0.72f, 0.22f);

std::string CameraPick::description() const
{
    return "1 orbits around the cubes, 2 flies (right drag looks, WASD and QE "
           "move). A left click selects the cube under the mouse.";
}

compages::Status CameraPick::setUp()
{
    // A ground the pick must ignore, and nine cubes it may select.
    m_scene.background(0.06f, 0.08f, 0.14f).ambient(0.14f, 0.15f, 0.18f);
    m_scene.sun();
    m_scene.box("Ground", PLAIN)
        .position(0.0f, -0.5f, 0.0f)
        .scale(40.0f, 1.0f, 40.0f);
    for (int x = -1; x <= 1; ++x)
    {
        for (int z = -1; z <= 1; ++z)
        {
            m_scene.box("Cube", PLAIN)
                .position(float(x) * 4.0f, 1.0f, float(z) * 4.0f)
                .scale(2.0f);
        }
    }

    // Orbit to start with. Key 2 in draw() swaps it for a fly control.
    m_camera = m_scene.camera().position(0.0f, 8.0f, 22.0f);
    m_camera.add<compages::world::Orbit>(
        compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    m_camera.get<compages::world::Orbit>().spin = 0.35f;
    return m_scene.prepare();
}

void CameraPick::select(compages::world::EntityId p_entity)
{
    // Put the previous selection back, then paint the new one. An empty id
    // is a click on nothing: the highlight just goes away.
    if (m_world.alive(m_selection))
    {
        m_scene.look(m_selection, PLAIN);
    }
    m_selection = p_entity;
    if (m_world.alive(m_selection))
    {
        m_scene.look(m_selection, PICKED);
    }
}

void CameraPick::draw(compages::world::ViewFrame const& p_frame)
{
    // One control at a time: taking the other off is what stops both from
    // reading the mouse.
    if (p_frame.input.down(compages::world::Key::D1) &&
        !m_camera.has<compages::world::Orbit>())
    {
        m_camera.remove<compages::world::Fly>().add<compages::world::Orbit>(
            compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    }
    else if (p_frame.input.down(compages::world::Key::D2) &&
             !m_camera.has<compages::world::Fly>())
    {
        m_camera.remove<compages::world::Orbit>().add<compages::world::Fly>();
    }

    m_scene.draw(p_frame);

    // Once at the start, the middle of the picture, so that a screenshot
    // shows a selection; then wherever the user clicks.
    const bool first = !m_auto_picked && (p_frame.total > 0.05f);
    if (p_frame.input.mouse_left_pressed || first)
    {
        const compages::core::Vector2f where =
            first ? compages::core::Vector2f(float(p_frame.width) * 0.5f,
                                             float(p_frame.height) * 0.45f)
                  : p_frame.input.mouse;
        auto hit = m_scene.pick(where);
        select(((hit) && (m_world.name(hit->entity) != "Ground"))
                   ? hit->entity
                   : compages::world::EntityId{});
        m_auto_picked = true;
    }
}

} // namespace examples
