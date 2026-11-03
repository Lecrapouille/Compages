// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/Core/Vector.hpp"
#include "Compages/GPU/Buffer.hpp"
#include "Compages/GPU/Pipeline.hpp"
#include "Compages/GPU/Shader.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"

#include <vector>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{

// ****************************************************************************
//! \brief Immediate-mode debug overlay: lines and wire boxes.
//!
//! Call \c clear() at the start of a frame, queue primitives, then \c flush()
//! after the main Renderer while the same pass is open.
// ****************************************************************************
class DebugDraw
{
public:

    [[nodiscard]] Status ensureInitialized();

    void clear();

    void line(compages::Vector3f const& p_a,
              compages::Vector3f const& p_b,
              compages::Vector3f const& p_color =
                  compages::Vector3f(1.0f, 0.2f, 0.2f));

    void box(compages::AABB const& p_local_bounds,
             compages::Matrix44f const& p_world_matrix,
             compages::Vector3f const& p_color =
                 compages::Vector3f(0.2f, 1.0f, 0.4f));

    void ray(compages::Vector3f const& p_origin,
             compages::Vector3f const& p_direction,
             float p_length,
             compages::Vector3f const& p_color =
                 compages::Vector3f(1.0f, 1.0f, 0.2f));

    [[nodiscard]] Status flush(CameraFrame const& p_camera);

private:

    struct DebugVertex
    {
        compages::Vector3f position;
        compages::Vector3f color;
    };

    bool m_ready = false;
    compages::gpu::Program m_program;
    compages::gpu::Pipeline m_pipeline;
    compages::gpu::VertexBuffer<DebugVertex> m_vertices;
    std::vector<DebugVertex> m_pending;
};

} // namespace compages::renderer
