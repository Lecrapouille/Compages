// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/37_Skybox.hpp"
#include "Common/DataPath.hpp"

namespace examples
{

std::string Skybox::description() const
{
    return "Six JPG faces from Compages-data around a spinning cube. The sky "
           "turns with the camera but never comes closer. Right drag looks "
           "around.";
}

compages::Status Skybox::setUp()
{
    // The six faces, in the order a cube map expects.
    std::array<std::string, 6u> faces{ "right.jpg",  "left.jpg",  "top.jpg",
                                       "bottom.jpg", "front.jpg", "back.jpg" };
    for (std::string& face : faces)
    {
        face = dataPath(face);
        if (face.empty())
        {
            return compages::failure(
                "the six skybox faces are missing from "
                "external/Compages-data/: run make download "
                "in external/");
        }
    }
    // The sky is part of the scene, not a second draw.
    m_scene.skybox(faces).ambient(0.18f, 0.18f, 0.20f);
    m_scene.camera().position(0.0f, 1.5f, 4.5f).add<compages::world::Orbit>();
    m_scene.activeCamera().get<compages::world::Orbit>().spin = 0.15f;
    m_scene.sun();
    m_cube = m_scene.box("Cube", compages::renderer::color(0.85f, 0.55f, 0.25f))
                 .scale(1.2f);
    return m_scene.prepare();
}

void Skybox::draw(compages::world::ViewFrame const& p_frame)
{
    // The cube turns. The sky only turns when the camera does.
    m_cube.rotate(0.9f * p_frame.elapsed, { 0.0f, 1.0f, 0.0f })
        .rotate(0.5f * p_frame.elapsed, { 1.0f, 0.0f, 0.0f });
    m_scene.draw(p_frame);
}

} // namespace examples
