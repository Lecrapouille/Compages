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

#include "30_WorldAndAssets/34_GltfModel.hpp"
#include "Common/DataPath.hpp"

namespace examples
{

//------------------------------------------------------------------------------
std::string GltfModel::description() const
{
    return "A GLB file loaded into the World and framed by the camera. Right "
           "drag turns around it, the wheel zooms. Needs Duck.glb in "
           "external/Compages-data/.";
}

//------------------------------------------------------------------------------
compages::Status GltfModel::setUp()
{
    // The file is optional data: without it the example says so and stops.
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        return compages::failure("Duck.glb is missing: run make download in "
                                 "external/, or set COMPAGES_DATA_PATH");
    }
    m_scene.background(0.12f, 0.14f, 0.18f);
    COMPAGES_TRY(m_scene.load(path));
    // Places the camera and a sun so the whole model is in frame.
    const compages::core::Vector3f middle = m_scene.frameAll();
    m_scene.activeCamera().add<compages::world::Orbit>(middle);
    return m_scene.prepare();
}

//------------------------------------------------------------------------------
void GltfModel::draw(compages::world::ViewFrame const& p_frame)
{
    // The orbit behavior reads the mouse inside the draw.
    m_scene.draw(p_frame);
}

} // namespace examples
