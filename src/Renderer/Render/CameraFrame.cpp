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

Ray CameraFrame::screenRay(float p_x,
                           float p_y,
                           std::uint32_t p_width,
                           std::uint32_t p_height) const
{
    if ((p_width == 0u) || (p_height == 0u))
    {
        const Vector3f forward = compages::matrix::transformPoint(
            inverse_view, Vector3f(0.0f, 0.0f, -1.0f)) - position;
        return Ray::fromPoints(position, position + forward);
    }

    const float ndc_x =
        ((p_x / static_cast<float>(p_width)) * 2.0f) - 1.0f;
    const float ndc_y =
        ((p_y / static_cast<float>(p_height)) * 2.0f) - 1.0f;

    // Row-vector unprojection: a clip-space point is a 1x4 row, and applying
    // inverse(view * projection) recovers the world point. Near is z = -1,
    // far is z = +1, the OpenGL clip convention this backend uses.
    const Matrix44f inv_vp = compages::matrix::inverse(view_projection);
    const Vector3f world_near =
        compages::matrix::transformPoint(inv_vp, Vector3f(ndc_x, ndc_y, -1.0f));
    const Vector3f world_far =
        compages::matrix::transformPoint(inv_vp, Vector3f(ndc_x, ndc_y, 1.0f));
    return Ray::fromPoints(world_near, world_far);
}

} // namespace compages::renderer
