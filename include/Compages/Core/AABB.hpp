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

#pragma once

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace compages::core
{

// ****************************************************************************
//! \file
//! \brief An axis-aligned box, for culling and for nothing else.
//!
//! A hierarchy of transforms turns a mesh's own box into a world box. The
//! frustum then asks whether that world box is in front of the camera. Those
//! two uses are why this type lives in Math rather than in World: neither
//! needs to know about entities.
// ****************************************************************************

// ****************************************************************************
//! \brief The smallest box aligned with the axes that still holds a set of
//! points.
// ****************************************************************************
struct AABB
{
    Vector3f min{ std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::infinity(),
                  std::numeric_limits<float>::infinity() };
    Vector3f max{ -std::numeric_limits<float>::infinity(),
                  -std::numeric_limits<float>::infinity(),
                  -std::numeric_limits<float>::infinity() };

    // ------------------------------------------------------------------------
    //! \brief A box from its two opposite corners. The corners may be given
    //! in any order.
    // ------------------------------------------------------------------------
    [[nodiscard]] static AABB fromCorners(Vector3f const& p_a,
                                          Vector3f const& p_b)
    {
        AABB box;
        box.min = Vector3f(std::min(p_a.x, p_b.x),
                           std::min(p_a.y, p_b.y),
                           std::min(p_a.z, p_b.z));
        box.max = Vector3f(std::max(p_a.x, p_b.x),
                           std::max(p_a.y, p_b.y),
                           std::max(p_a.z, p_b.z));
        return box;
    }

    // ------------------------------------------------------------------------
    //! \brief A box around a centre, of the given half-size on each axis.
    // ------------------------------------------------------------------------
    [[nodiscard]] static AABB fromCenterExtent(Vector3f const& p_center,
                                               Vector3f const& p_extent)
    {
        AABB box;
        box.min = p_center - p_extent;
        box.max = p_center + p_extent;
        return box;
    }

    // ------------------------------------------------------------------------
    //! \brief Has this box no point inside it?
    //!
    //! The default constructed box is empty, so a mesh that forgot to fill
    //! its bounds is culled rather than treated as covering the world.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return (min.x > max.x) || (min.y > max.y) || (min.z > max.z);
    }

    // ------------------------------------------------------------------------
    //! \brief Grow so that this point is inside.
    // ------------------------------------------------------------------------
    void expand(Vector3f const& p_point)
    {
        min.x = std::min(min.x, p_point.x);
        min.y = std::min(min.y, p_point.y);
        min.z = std::min(min.z, p_point.z);
        max.x = std::max(max.x, p_point.x);
        max.y = std::max(max.y, p_point.y);
        max.z = std::max(max.z, p_point.z);
    }

    // ------------------------------------------------------------------------
    //! \brief The smallest box that holds both this one and another.
    // ------------------------------------------------------------------------
    [[nodiscard]] AABB merged(AABB const& p_other) const
    {
        if (empty())
        {
            return p_other;
        }
        if (p_other.empty())
        {
            return *this;
        }
        AABB box;
        box.min = Vector3f(std::min(min.x, p_other.min.x),
                           std::min(min.y, p_other.min.y),
                           std::min(min.z, p_other.min.z));
        box.max = Vector3f(std::max(max.x, p_other.max.x),
                           std::max(max.y, p_other.max.y),
                           std::max(max.z, p_other.max.z));
        return box;
    }

    // ------------------------------------------------------------------------
    //! \brief The centre, or the origin when the box is empty.
    // ------------------------------------------------------------------------
    [[nodiscard]] Vector3f center() const
    {
        return empty() ? Vector3f(0.0f, 0.0f, 0.0f) : ((min + max) * 0.5f);
    }

    // ------------------------------------------------------------------------
    //! \brief Half-size on each axis, or zero when the box is empty.
    // ------------------------------------------------------------------------
    [[nodiscard]] Vector3f extent() const
    {
        return empty() ? Vector3f(0.0f, 0.0f, 0.0f) : ((max - min) * 0.5f);
    }

    // ------------------------------------------------------------------------
    //! \brief Do these two boxes overlap?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool intersects(AABB const& p_other) const
    {
        if (empty() || p_other.empty())
        {
            return false;
        }
        return (min.x <= p_other.max.x) && (max.x >= p_other.min.x) &&
               (min.y <= p_other.max.y) && (max.y >= p_other.min.y) &&
               (min.z <= p_other.max.z) && (max.z >= p_other.min.z);
    }

    // ------------------------------------------------------------------------
    //! \brief Apply a transform the way a shader does: model * vec4(p, 1).
    //!
    //! The eight corners move, then a new aligned box is built around them.
    //! Rotation therefore grows the box; that is the price of staying aligned
    //! and the reason a tight cull wants an oriented box instead.
    // ------------------------------------------------------------------------
    [[nodiscard]] AABB transformed(Matrix44f const& p_model) const
    {
        if (empty())
        {
            return {};
        }

        AABB box;
        const float xs[2] = { min.x, max.x };
        const float ys[2] = { min.y, max.y };
        const float zs[2] = { min.z, max.z };
        for (float x : xs)
        {
            for (float y : ys)
            {
                for (float z : zs)
                {
                    Vector4f const h = p_model * Vector4f(x, y, z, 1.0f);
                    const float w = (std::abs(h.w) < 1.0e-8f) ? 1.0f : h.w;
                    box.expand(Vector3f(h.x / w, h.y / w, h.z / w));
                }
            }
        }
        return box;
    }
};

} // namespace compages::core

