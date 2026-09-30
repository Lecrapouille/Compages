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

#include "Compages/Renderer/Render/CameraFrame.hpp"

#include "Compages/Core/Transformation.hpp"

namespace compages::renderer
{

//------------------------------------------------------------------------------
compages::core::Ray CameraFrame::screenRay(float p_x,
                           float p_y,
                           std::uint32_t p_width,
                           std::uint32_t p_height) const
{
    if ((p_width == 0u) || (p_height == 0u))
    {
        const compages::core::Vector3f forward = compages::core::transformPoint(
            inverse_view, compages::core::Vector3f(0.0f, 0.0f, -1.0f)) - position;
        return compages::core::Ray::fromPoints(position, position + forward);
    }

    const float ndc_x =
        ((p_x / static_cast<float>(p_width)) * 2.0f) - 1.0f;
    const float ndc_y =
        ((p_y / static_cast<float>(p_height)) * 2.0f) - 1.0f;

    // Column-vector unprojection: \c p_world = inverse(view_projection) * \c p_clip.
    // Near is z = -1, far is z = +1 (OpenGL clip convention).
    const compages::core::Matrix44f inv_vp = compages::core::inverse(view_projection);
    const compages::core::Vector3f world_near =
        compages::core::transformPoint(inv_vp, compages::core::Vector3f(ndc_x, ndc_y, -1.0f));
    const compages::core::Vector3f world_far =
        compages::core::transformPoint(inv_vp, compages::core::Vector3f(ndc_x, ndc_y, 1.0f));
    return compages::core::Ray::fromPoints(world_near, world_far);
}

} // namespace compages::renderer
