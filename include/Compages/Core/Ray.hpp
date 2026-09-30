// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Vector.hpp"

#include <cmath>
#include <limits>
#include <optional>

namespace compages::core
{
// ****************************************************************************
//! \file
//! \brief A half-line, and the two intersections a picker needs.
//!
//! A CameraFrame turns a pixel into a Ray. The World then asks which
//! MeshRenderer the ray hits first. Those two uses share this type so neither
//! layer invents its own parametrization.
// ****************************************************************************

// ****************************************************************************
//! \brief Origin plus a unit direction. Point at \c t is \c origin + t *
//! direction.
// ****************************************************************************
struct Ray
{
    Vector3f origin{ 0.0f, 0.0f, 0.0f };
    Vector3f direction{ 0.0f, 0.0f, -1.0f };

    [[nodiscard]] Vector3f pointAt(float p_t) const
    {
        return origin + (direction * p_t);
    }

    // ------------------------------------------------------------------------
    //! \brief A ray from \c p_from through \c p_to, with a unit direction.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Ray fromPoints(Vector3f const& p_from,
                                        Vector3f const& p_to)
    {
        Ray ray;
        ray.origin = p_from;
        const Vector3f delta = p_to - p_from;
        const float length = compages::core::vector::norm(delta);
        ray.direction =
            (length < 1.0e-8f) ? Vector3f(0.0f, 0.0f, -1.0f) : (delta / length);
        return ray;
    }
};

// ****************************************************************************
//! \brief First hit of the ray against the box, or empty.
//!
//! \c t is the distance along the unit direction, so it is in world units.
//! A ray that starts inside the box reports \c t = 0.
// ****************************************************************************
[[nodiscard]] inline std::optional<float> intersect(Ray const& p_ray,
                                                    AABB const& p_box)
{
    if (p_box.empty())
    {
        return std::nullopt;
    }

    float t_min = 0.0f;
    float t_max = std::numeric_limits<float>::infinity();

    const float origins[3] = { p_ray.origin.x, p_ray.origin.y, p_ray.origin.z };
    const float dirs[3] = { p_ray.direction.x,
                            p_ray.direction.y,
                            p_ray.direction.z };
    const float mins[3] = { p_box.min.x, p_box.min.y, p_box.min.z };
    const float maxs[3] = { p_box.max.x, p_box.max.y, p_box.max.z };

    for (int axis = 0; axis < 3; ++axis)
    {
        if (std::abs(dirs[axis]) < 1.0e-8f)
        {
            if ((origins[axis] < mins[axis]) || (origins[axis] > maxs[axis]))
            {
                return std::nullopt;
            }
            continue;
        }
        const float inv = 1.0f / dirs[axis];
        float t0 = (mins[axis] - origins[axis]) * inv;
        float t1 = (maxs[axis] - origins[axis]) * inv;
        if (t0 > t1)
        {
            const float tmp = t0;
            t0 = t1;
            t1 = tmp;
        }
        t_min = std::max(t_min, t0);
        t_max = std::min(t_max, t1);
        if (t_min > t_max)
        {
            return std::nullopt;
        }
    }
    return t_min;
}

} // namespace compages::core

