// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The rivers of the map: a smooth spline through the cells they flow
//! through (the thesis tried Bézier, Catmull-Rom and B-splines for the axis
//! of a river), meanders of about eleven widths (Leopold and Wolman 1960), a
//! width that grows with the square root of the area drained (Leopold and
//! Maddock 1953); and an index of their segments to find the nearest one.
// ****************************************************************************

#include "10_Complete/Archipelago/Map.hpp"

#include "Compages/Core/Vector.hpp"

#include <cstdint>
#include <vector>

namespace examples::archipelago
{

//! \brief A river from its source to the sea or to the river it joins.
struct River
{
    std::vector<compages::Vector2f> points;
    //! \brief Its width at each point, in metres.
    std::vector<float> widths;
};

//! \brief The width of a river that drains \p p_area square metres.
[[nodiscard]] float riverWidth(float p_area);

//! \brief How far from its middle a river digs (its bed, then its valley),
//! for a river \p p_half wide on each side.
[[nodiscard]] float valleyReach(float p_half);

// ----------------------------------------------------------------------------
//! \brief A river through points (from upstream): a Catmull-Rom spline that
//! passes through each one (two rivers still meet at a cell), then its
//! meanders. The swing fades at both ends, so that a river still meets the
//! one it joins.
//! \param[in] p_widths The width at each point.
// ----------------------------------------------------------------------------
[[nodiscard]] River makeRiver(std::vector<compages::Vector2f> const& p_points,
                              std::vector<float> const& p_widths,
                              std::uint32_t p_seed);

// ****************************************************************************
//! \brief The segments of the rivers, sorted in the squares of a 64 x 64
//! grid that their valleys reach: the nearest river of a point is looked
//! for in its square only.
// ****************************************************************************
class RiverIndex
{
public:

    //! \brief Index the segments of these rivers.
    void build(std::vector<River> const& p_rivers);

    //! \brief The nearest river: the distance to its middle line, and its
    //! half width there. Far from any river: a large distance.
    void nearest(float p_x, float p_z, float& p_distance, float& p_half) const;

private:

    struct Segment
    {
        compages::Vector2f a;
        compages::Vector2f b;
        float width_a;
        float width_b;
    };

    std::vector<Segment> m_segments;
    std::vector<std::vector<int>> m_buckets;
};

} // namespace examples::archipelago
