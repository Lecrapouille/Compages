// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "13_Workshop/Workshop/Workshop.hpp"

#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Controllers/Controls.hpp"

namespace examples::workshop
{

// ----------------------------------------------------------------------------
Room buildRoom(compages::renderer::Scene& p_scene)
{
    using compages::renderer::color;

    // The sky behind, the camera, and the light of the day.
    p_scene.background(0.62f, 0.70f, 0.78f);
    p_scene.camera().position(1.3f, 1.55f, 1.55f).add<compages::world::Orbit>(compages::Vector3f(0.0f, 0.85f, 0.0f));
    p_scene.sun("Sun", { 1.0f, 0.96f, 0.9f }, 1.0f).position(3.0f, 6.0f, 2.0f).lookAt(0.0f, 0.0f, 0.0f);

    // The floor, and a grid on it: a help to judge sizes, that a robot
    // camera does not see (doc/tutorials/07-what-the-robot-sees.md).
    p_scene.box("Floor", color(0.42f, 0.42f, 0.44f)).position(0.0f, -0.05f, 0.0f).scale(6.0f, 0.1f, 6.0f);
    p_scene.grid(6.0f, 12u);

    // A table 1.6 m by 1 m: a top and four legs, children of the table so
    // that moving the table moves them.
    Room room;
    room.table = p_scene.world().entity("Table").position(0.0f, TABLE_TOP, 0.0f);
    p_scene.box("Top", color(0.55f, 0.38f, 0.22f)).parent(room.table).position(0.0f, -0.03f, 0.0f).scale(1.6f, 0.06f, 1.0f);
    for (float x : { -0.72f, 0.72f })
    {
        for (float z : { -0.42f, 0.42f })
        {
            p_scene.box("Leg", color(0.30f, 0.22f, 0.14f))
                .parent(room.table)
                .position(x, -0.5f * TABLE_TOP, z)
                .scale(0.06f, TABLE_TOP - 0.06f, 0.06f);
        }
    }

    // A part beyond the reach of the arm, and a lamp above the table.
    room.part = p_scene.box("Part", color(0.10f, 0.30f, 0.90f)).parent(room.table).position(0.55f, 0.05f, 0.2f).scale(0.18f, 0.1f, 0.12f);
    compages::world::Entity lamp = p_scene.lamp("Lamp", { 1.0f, 0.85f, 0.6f }, 2.0f, 3.0f).position(0.2f, 1.5f, 0.3f);
    // The bulb lets the light of the sun through: no shadow.
    p_scene.sphere("Bulb", compages::renderer::unlit(1.0f, 0.9f, 0.7f))
        .parent(lamp)
        .scale(0.08f)
        .get<compages::renderer::MeshRenderer>()
        .flags = compages::renderer::RenderFlags::NoShadowCaster;
    return room;
}

} // namespace examples::workshop
