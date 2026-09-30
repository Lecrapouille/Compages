// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "20_Performance/20b_ManyCubes.hpp"

#include <array>

namespace examples
{

std::string ManyCubes::description() const
{
    return "1728 cubes sharing one mesh and five looks. They are drawn sorted "
           "by look, and those outside the view are not drawn: the draw-call "
           "counter of the overlay shows the survivors as the camera turns.";
}

compages::Status ManyCubes::setUp()
{
    // The camera turns by itself, so the culling has something to drop as the
    // view sweeps past the grid.
    m_scene.background(0.03f, 0.05f, 0.10f).ambient(0.12f, 0.13f, 0.17f);
    m_scene.camera().position(0.0f, 30.0f, 80.0f).add<compages::world::Orbit>();
    m_scene.activeCamera().get<compages::world::Orbit>().spin = 0.2f;
    m_scene.sun();

    // One cube of each colour, that all the others copy.
    const std::array<compages::world::Entity, 5u> palette{
        m_scene.box("sky", compages::renderer::color(0.35f, 0.55f, 0.85f)),
        m_scene.box("olive", compages::renderer::color(0.65f, 0.75f, 0.35f)),
        m_scene.box("orange", compages::renderer::color(0.85f, 0.55f, 0.35f)),
        m_scene.box("rose", compages::renderer::color(0.80f, 0.35f, 0.55f)),
        m_scene.box("violet", compages::renderer::color(0.55f, 0.35f, 0.75f)),
    };

    // A 12 x 12 x 12 grid, coloured by (x + y + z) mod 5 so that neighbours
    // never share a colour: in the order they are made, the looks alternate
    // all the time, which is what the sort undoes.
    constexpr int GRID = 12;
    constexpr float SPACING = 4.0f;
    const float corner = -0.5f * float(GRID - 1) * SPACING;
    for (int x = 0; x < GRID; ++x)
    {
        for (int y = 0; y < GRID; ++y)
        {
            for (int z = 0; z < GRID; ++z)
            {
                m_scene.copy(palette[std::size_t(x + y + z) % palette.size()])
                    .position(corner + SPACING * float(x),
                              corner + SPACING * float(y),
                              corner + SPACING * float(z))
                    .scale(2.4f);
            }
        }
    }
    // The models were only there to be copied; their looks stay, worn by
    // the copies.
    for (compages::world::Entity model : palette)
    {
        model.destroy();
    }
    return m_scene.prepare();
}

void ManyCubes::draw(compages::world::ViewFrame const& p_frame)
{
    // Sorting and frustum culling happen inside the draw, from the camera
    // the orbit behavior just moved.
    m_scene.draw(p_frame);
}

} // namespace examples
