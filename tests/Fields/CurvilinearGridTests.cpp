// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Fields/CurvilinearGrid.hpp"
#include "Compages/Fields/Field.hpp"
#include "Compages/Fields/Streamline.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <numbers>

using compages::CurvilinearGrid;
using compages::Field;
using compages::Grid2D;
using compages::Vector2f;

namespace
{

constexpr float PI = std::numbers::pi_v<float>;

//! \brief An O-grid round a disc of radius 1: i goes round (the first
//! column repeated as the last one), j goes out to radius 4, tighter near
//! the disc.
CurvilinearGrid ring()
{
    auto made = CurvilinearGrid::create(65, 25, [](int i, int j) {
        const float angle = 2.0f * PI * float(i) / 64.0f;
        const float r = std::pow(4.0f, float(j) / 24.0f);
        return Vector2f(r * std::cos(angle), r * std::sin(angle));
    });
    EXPECT_TRUE(bool(made)) << made.error();
    return made.take();
}

} // namespace

// Fewer than 2 x 2 points is no grid.
TEST(CurvilinearGrid, NeedsTwoByTwoPoints)
{
    EXPECT_FALSE(bool(CurvilinearGrid::create(1, 5, [](int, int) { return Vector2f(0.0f, 0.0f); })));
}

// Placed like a Grid2D, it samples like a Grid2D.
TEST(CurvilinearGrid, RegularPointsSampleLikeAGrid2D)
{
    const Grid2D regular{ .columns = 6, .rows = 4, .origin = { -1.0f, 2.0f }, .spacing = 0.5f };
    auto made = CurvilinearGrid::create(6, 4, [&regular](int i, int j) { return regular.point(i, j); });
    ASSERT_TRUE(bool(made));
    Field<float> a(regular, 0.0f);
    Field<float, CurvilinearGrid> b(made.value(), 0.0f);
    for (std::size_t k = 0u; k < a.size(); ++k)
    {
        a[k] = float(k * k % 7u);
        b[k] = a[k];
    }
    for (Vector2f p : { Vector2f(-0.8f, 2.1f), Vector2f(0.3f, 3.2f), Vector2f(1.4f, 2.9f) })
    {
        EXPECT_NEAR(a.sample(p), b.sample(p), 1.0e-4f);
        EXPECT_TRUE(made.value().locate(p).inside);
    }
}

// A value that is a plane over the points (a x + b y + c) is a bilinear
// function of the coordinates of any cell: the sample finds it exactly,
// even in the curved cells of a ring.
TEST(CurvilinearGrid, FindsTheCellOfAPlaceInARing)
{
    const CurvilinearGrid grid = ring();
    Field<float, CurvilinearGrid> plane(grid, 0.0f);
    for (std::size_t k = 0u; k < plane.size(); ++k)
    {
        const Vector2f p = grid.point(k);
        plane[k] = (3.0f * p.x) - (2.0f * p.y) + 1.0f;
    }
    // Between the corners of a cell the true value of a plane is not the
    // bilinear one when the cell is curved: compare at the corners' mix.
    for (Vector2f p : { Vector2f(1.5f, 0.2f), Vector2f(-2.0f, 1.0f), Vector2f(0.1f, -3.5f), Vector2f(2.5f, 2.5f) })
    {
        const compages::GridCell c = grid.locate(p);
        ASSERT_TRUE(c.inside);
        const Vector2f a = grid.point(c.i, c.j);
        const Vector2f b = grid.point(c.i + 1, c.j);
        const Vector2f d = grid.point(c.i, c.j + 1);
        const Vector2f e = grid.point(c.i + 1, c.j + 1);
        const Vector2f back = (a * ((1.0f - c.s) * (1.0f - c.t))) + (b * (c.s * (1.0f - c.t))) +
                              (d * ((1.0f - c.s) * c.t)) + (e * (c.s * c.t));
        EXPECT_NEAR(back.x, p.x, 1.0e-4f); // the inverse is right
        EXPECT_NEAR(back.y, p.y, 1.0e-4f);
        EXPECT_NEAR(plane.sample(p), (3.0f * p.x) - (2.0f * p.y) + 1.0f, 1.0e-3f);
    }
    // In the hole and beyond the ring: outside.
    EXPECT_FALSE(grid.locate(Vector2f(0.2f, 0.1f)).inside);
    EXPECT_FALSE(grid.locate(Vector2f(5.0f, 0.0f)).inside);
}

// A streamline of a field turning round the ring keeps its radius.
TEST(CurvilinearGrid, StreamlinesGoRoundTheRing)
{
    const CurvilinearGrid grid = ring();
    Field<Vector2f, CurvilinearGrid> turn(grid, Vector2f(0.0f, 0.0f));
    for (std::size_t k = 0u; k < turn.size(); ++k)
    {
        const Vector2f p = grid.point(k);
        turn[k] = Vector2f(-p.y, p.x);
    }
    const std::vector<Vector2f> line = compages::streamline(turn, Vector2f(2.0f, 0.0f), 0.02f, 300);
    ASSERT_GT(line.size(), 200u);
    for (Vector2f p : line)
    {
        EXPECT_NEAR(std::hypot(p.x, p.y), 2.0f, 0.02f);
    }
}
