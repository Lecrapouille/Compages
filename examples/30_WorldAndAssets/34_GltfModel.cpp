// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "30_WorldAndAssets/34_GltfModel.hpp"
#include "Common/DataPath.hpp"

namespace examples
{

std::string GltfModel::description() const
{
    return "A GLB file loaded into the World and framed by the camera. Right "
           "drag turns around it, the wheel zooms. Needs Duck.glb in "
           "external/Compages-data/.";
}

compages::gpu::Status GltfModel::setUp()
{
    // The file is optional data: without it the example says so and stops.
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        return compages::gpu::failure("Duck.glb is missing: run make download in "
                            "external/, or set COMPAGES_DATA_PATH");
    }
    m_scene.background(0.12f, 0.14f, 0.18f);
    COMPAGES_TRY(m_scene.load(path));
    // Places the camera and a sun so the whole model is in frame.
    const Vector3f middle = m_scene.frameAll();
    m_scene.activeCamera().add<compages::world::Orbit>(middle);
    return m_scene.prepare();
}

void GltfModel::draw(Frame const& p_frame)
{
    // The orbit behavior reads the mouse inside the draw.
    m_scene.draw(p_frame);
}

} // namespace examples
