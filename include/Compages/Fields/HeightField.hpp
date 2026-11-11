// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Ray.hpp"
#include "Compages/Fields/Field.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace compages
{

// ----------------------------------------------------------------------------
//! \brief Where a ray first goes under a ground y = height(x, z): the ground
//! under the mouse, the landing point of a shot.
//!
//! The ray is walked from \p p_from to \p p_to by steps of \p p_step; the
//! first step that ends under the ground is then halved eight times. A step
//! smaller than the bumps of the ground does not jump over them.
//!
//! \param[in] p_ray The ray.
//! \param[in] p_height The ground: float(float x, float z).
//! \param[in] p_from, p_to The part of the ray to walk (distances along it).
//! \param[in] p_step The length of a step, more than zero.
//! \return The distance along the ray, or none when it stays above.
// ----------------------------------------------------------------------------
template <typename Height>
[[nodiscard]] std::optional<float> intersectHeight(Ray const& p_ray,
                                                   Height const& p_height,
                                                   float p_from,
                                                   float p_to,
                                                   float p_step)
{
    auto below = [&p_ray, &p_height](float p_t)
    {
        const Vector3f p = p_ray.pointAt(p_t);
        return p.y <= p_height(p.x, p.z);
    };
    if ((p_step <= 0.0f) || (p_to < p_from))
    {
        return std::nullopt;
    }
    const auto steps = int(std::ceil((p_to - p_from) / p_step));
    for (int k = 0; k <= steps; ++k)
    {
        const float t = std::min(p_from + (float(k) * p_step), p_to);
        if (!below(t))
        {
            continue;
        }
        float low = std::max(p_from, t - p_step);
        float high = t;
        for (int halve = 0; halve < 8; ++halve)
        {
            const float middle = 0.5f * (low + high);
            (below(middle) ? high : low) = middle;
        }
        return high;
    }
    return std::nullopt;
}

// ----------------------------------------------------------------------------
//! \brief Where a ray first goes under a field of heights, sampled
//! bilinearly, with steps of half its spacing.
//! \param[in] p_ray The ray.
//! \param[in] p_heights The heights of the ground, y up.
//! \return The point, or none when the ray misses the grid or stays above.
// ----------------------------------------------------------------------------
[[nodiscard]] inline std::optional<Vector3f>
intersect(Ray const& p_ray, Field<float> const& p_heights)
{
    if (p_heights.size() == 0u)
    {
        return std::nullopt;
    }
    const auto [low, high] =
        std::minmax_element(p_heights.begin(), p_heights.end());
    // The part of the ray between the highest and the lowest height.
    const float dy = p_ray.direction.y;
    float from = 0.0f;
    float to = 0.0f;
    if (std::abs(dy) < 1.0e-6f)
    {
        if ((p_ray.origin.y > *high) || (p_ray.origin.y < *low))
        {
            return std::nullopt;
        }
        Grid2D const& grid = p_heights.grid();
        to = grid.spacing * float(grid.columns + grid.rows);
    }
    else
    {
        const float a = (*high - p_ray.origin.y) / dy;
        const float b = (*low - p_ray.origin.y) / dy;
        from = std::max(0.0f, std::min(a, b));
        to = std::max(a, b);
    }
    // Half a spacing along the ray: never more than that across the ground.
    const float step = 0.5f * p_heights.grid().spacing;
    // No ground outside the grid: the ray passes under its edge.
    auto ground = [&p_heights](float p_x, float p_z)
    {
        Grid2D const& grid = p_heights.grid();
        const Vector2f at = grid.coordinates(Vector2f(p_x, p_z));
        const bool inside = (at.x >= 0.0f) && (at.y >= 0.0f) &&
                            (at.x <= float(grid.columns - 1)) &&
                            (at.y <= float(grid.rows - 1));
        return inside ? p_heights.sample(Vector2f(p_x, p_z))
                      : -std::numeric_limits<float>::infinity();
    };
    const std::optional<float> t =
        intersectHeight(p_ray, ground, from, to + step, step);
    if (!t)
    {
        return std::nullopt;
    }
    return p_ray.pointAt(*t);
}

} // namespace compages
