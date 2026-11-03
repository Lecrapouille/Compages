// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/CameraFrame.hpp"

#include "Compages/Core/Transformation.hpp"

namespace compages::renderer
{

compages::Ray CameraFrame::screenRay(float p_x,
                           float p_y,
                           std::uint32_t p_width,
                           std::uint32_t p_height) const
{
    if ((p_width == 0u) || (p_height == 0u))
    {
        const compages::Vector3f forward = compages::transformPoint(
            inverse_view, compages::Vector3f(0.0f, 0.0f, -1.0f)) - position;
        return compages::Ray::fromPoints(position, position + forward);
    }

    const float ndc_x =
        ((p_x / static_cast<float>(p_width)) * 2.0f) - 1.0f;
    const float ndc_y =
        ((p_y / static_cast<float>(p_height)) * 2.0f) - 1.0f;

    // Column-vector unprojection: \c p_world = inverse(view_projection) * \c p_clip.
    // Near is z = -1, far is z = +1 (OpenGL clip convention).
    const compages::Matrix44f inv_vp = compages::inverse(view_projection);
    const compages::Vector3f world_near =
        compages::transformPoint(inv_vp, compages::Vector3f(ndc_x, ndc_y, -1.0f));
    const compages::Vector3f world_far =
        compages::transformPoint(inv_vp, compages::Vector3f(ndc_x, ndc_y, 1.0f));
    return compages::Ray::fromPoints(world_near, world_far);
}

} // namespace compages::renderer
