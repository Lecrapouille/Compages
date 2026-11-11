// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Fields/Field.hpp"

#include <cmath>
#include <vector>

namespace compages
{

// ----------------------------------------------------------------------------
//! \brief The line a speck would follow in a steady field of vectors, from a
//! start: a streamline. Each step goes \p p_step metres along the field
//! (Runge-Kutta 2: the direction at the middle of the step).
//!
//! \code
//! for (compages::Vector2f p : compages::streamline(wind, start, 0.05f, 200)) { ... }
//! \endcode
//! \param[in] p_field The vectors (bilinear between the points).
//! \param[in] p_start Where the line starts, in the plane of the grid.
//! \param[in] p_step The length of a step, in metres.
//! \param[in] p_steps How many steps at most.
//! \return The points of the line, the start first; it stops where it leaves
//! the grid or where the field is still.
// ----------------------------------------------------------------------------
[[nodiscard]] inline std::vector<Vector2f>
streamline(Field<Vector2f> const& p_field, Vector2f p_start, float p_step, int p_steps)
{
    Grid2D const& grid = p_field.grid();
    auto inside = [&grid](Vector2f p_at)
    {
        const Vector2f at = grid.coordinates(p_at);
        return (at.x >= 0.0f) && (at.y >= 0.0f) && (at.x <= float(grid.columns - 1)) &&
               (at.y <= float(grid.rows - 1));
    };
    auto direction = [&p_field](Vector2f p_at, Vector2f& p_out)
    {
        const Vector2f v = p_field.sample(p_at);
        const float length = std::hypot(v.x, v.y);
        if (length < 1.0e-6f)
        {
            return false;
        }
        p_out = v * (1.0f / length);
        return true;
    };
    std::vector<Vector2f> line;
    if (!inside(p_start))
    {
        return line;
    }
    line.push_back(p_start);
    Vector2f at = p_start;
    for (int k = 0; k < p_steps; ++k)
    {
        Vector2f first(0.0f, 0.0f);
        Vector2f middle(0.0f, 0.0f);
        if (!direction(at, first) || !direction(at + (first * (0.5f * p_step)), middle))
        {
            break;
        }
        at = at + (middle * p_step);
        if (!inside(at))
        {
            break;
        }
        line.push_back(at);
    }
    return line;
}

} // namespace compages
