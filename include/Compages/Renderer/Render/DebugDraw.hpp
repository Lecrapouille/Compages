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
#include "Compages/Renderer/Render/RenderSnapshot.hpp"
#include "Compages/World/Components/Camera.hpp"

#include <span>
#include <vector>

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{

// ****************************************************************************
//! \brief Immediate-mode debug overlay: lines, wire boxes, paths, arrows,
//! frames of poses and frustums of cameras.
//!
//! Call \c clear() at the start of a frame, queue primitives, then \c flush()
//! after the main Renderer while the same pass is open.
//!
//! A Scene owns one and does this for you: queue lines with Scene::debug()
//! at each frame, they are drawn by the next render and then forgotten. Use
//! it to see what code computes (a path, a force, a pose, what a camera
//! sees). A camera whose \c sees_helpers is false does not draw it.
//!
//! \code
//! scene.debug().arrow(body.worldPosition(), body.worldPosition() + velocity, { 1.0f, 0.3f, 0.2f });
//! scene.debug().frustum(eye.worldMatrix(), eye.get<compages::world::Camera>(), aspect, 20.0f, { 1.0f, 0.85f, 0.2f });
//! \endcode
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

    // ------------------------------------------------------------------------
    //! \brief Lines joining the points one after the other (a path, a
    //! trajectory, a streamline). A loop repeats its first point at the end.
    // ------------------------------------------------------------------------
    void polyline(std::span<const compages::Vector3f> p_points,
                  compages::Vector3f const& p_color);

    // ------------------------------------------------------------------------
    //! \brief A line from \c p_from to \c p_to with a head at \c p_to (a
    //! force, a velocity, a direction to go).
    // ------------------------------------------------------------------------
    void arrow(compages::Vector3f const& p_from,
               compages::Vector3f const& p_to,
               compages::Vector3f const& p_color);

    // ------------------------------------------------------------------------
    //! \brief The three axes of a pose: X red, Y green, Z blue, each
    //! \c p_length long (a joint, a tool, a target).
    // ------------------------------------------------------------------------
    void frame(compages::Matrix44f const& p_pose, float p_length);

    // ------------------------------------------------------------------------
    //! \brief What a camera sees, cut at \c p_length from it: the pyramid of a
    //! perspective lens, the box of an orthographic one.
    //!
    //! \param[in] p_pose where the camera stands (its world matrix; it looks
    //! down its -Z).
    //! \param[in] p_lens its projection and field of view.
    //! \param[in] p_aspect width over height of its picture (640 x 480: 4/3).
    //! \param[in] p_length how far from the camera the pyramid stops.
    //! \param[in] p_color colour of the lines.
    // ------------------------------------------------------------------------
    void frustum(compages::Matrix44f const& p_pose,
                 compages::world::Camera const& p_lens,
                 float p_aspect,
                 float p_length,
                 compages::Vector3f const& p_color);

    // ------------------------------------------------------------------------
    //! \brief Draw the lines queued so far, in the open pass, for this view
    //! (its camera, and whether it is drawn into a linear picture).
    // ------------------------------------------------------------------------
    [[nodiscard]] Status flush(RenderSnapshot const& p_view);

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
