// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Fields/Field.hpp"
#include "Compages/Fields/Grid2D.hpp"
#include "Compages/Fields/HeightField.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

using compages::Field;
using compages::Grid2D;
using compages::Vector2f;
using compages::Vector3f;

namespace
{

//! \brief Vectors compare per component: true when all are equal.
template <typename V>
bool same(V const& p_a, V const& p_b)
{
    return (p_a.x == p_b.x) && (p_a.y == p_b.y);
}

const Grid2D GRID{ .columns = 4, .rows = 3, .origin = { 10.0f, 20.0f }, .spacing = 2.0f };

} // namespace

TEST(Grid2D, NumbersItsPointsRowAfterRow)
{
    EXPECT_EQ(GRID.count(), 12u);
    EXPECT_EQ(GRID.index(1, 2), 9u);
    EXPECT_EQ(GRID.column(9u), 1);
    EXPECT_EQ(GRID.row(9u), 2);
    EXPECT_TRUE(GRID.contains(3, 2));
    EXPECT_FALSE(GRID.contains(4, 0));
    EXPECT_FALSE(GRID.contains(0, -1));
}

TEST(Grid2D, PlacesItsPointsInThePlane)
{
    EXPECT_TRUE(same(GRID.point(1, 2), Vector2f(12.0f, 24.0f)));
    EXPECT_TRUE(same(GRID.coordinates(Vector2f(13.0f, 21.0f)), Vector2f(1.5f, 0.5f)));
    EXPECT_TRUE(same(GRID.nearest(Vector2f(13.1f, 20.9f)), compages::Vector2i(2, 0)));
}

TEST(Grid2D, KeepsTheNeighboursInside)
{
    std::vector<std::size_t> corner;
    for (std::size_t n : GRID.neighbours4(GRID.index(0, 0)))
    {
        corner.push_back(n);
    }
    EXPECT_THAT(corner, ::testing::UnorderedElementsAre(1u, 4u));

    std::size_t middle = 0u;
    for (std::size_t n : GRID.neighbours8(GRID.index(1, 1)))
    {
        (void)n;
        ++middle;
    }
    EXPECT_EQ(middle, 8u);
    EXPECT_EQ(GRID.neighbours8(GRID.index(3, 2)).size, 3u);
}

TEST(Field, HoldsOneValuePerPoint)
{
    Field<float> field(GRID, 1.5f);
    EXPECT_EQ(field.size(), 12u);
    field(2, 1) = 7.0f;
    EXPECT_EQ(field[GRID.index(2, 1)], 7.0f);
    EXPECT_EQ(field.values().size(), 12u);
    EXPECT_EQ(std::count(field.begin(), field.end(), 1.5f), 11);
    field.fill(0.0f);
    EXPECT_EQ(field(2, 1), 0.0f);
}

// Bilinear between the points, the value of the edge outside.
TEST(Field, SamplesBetweenItsPoints)
{
    Field<float> field(GRID, 0.0f);
    for (int j = 0; j < GRID.rows; ++j)
    {
        for (int i = 0; i < GRID.columns; ++i)
        {
            field(i, j) = float(i) + (10.0f * float(j));
        }
    }
    EXPECT_FLOAT_EQ(field.sample(GRID.point(2, 1)), 12.0f);
    EXPECT_FLOAT_EQ(field.sample(Vector2f(13.0f, 21.0f)), 6.5f);
    EXPECT_FLOAT_EQ(field.sample(GRID.point(3, 2)), 23.0f);
    EXPECT_FLOAT_EQ(field.sample(Vector2f(-100.0f, -100.0f)), 0.0f);
    EXPECT_FLOAT_EQ(field.sample(Vector2f(100.0f, 100.0f)), 23.0f);

    Field<Vector2f> wind(GRID, Vector2f(1.0f, 2.0f));
    const Vector2f mixed = wind.sample(Vector2f(13.0f, 21.0f));
    EXPECT_FLOAT_EQ(mixed.x, 1.0f);
    EXPECT_FLOAT_EQ(mixed.y, 2.0f);
}

// A slope rising along x: y = x - 10 between x = 10 and x = 16.
TEST(HeightField, FindsWhereARayMeetsTheGround)
{
    Field<float> ground(GRID, 0.0f);
    for (int j = 0; j < GRID.rows; ++j)
    {
        for (int i = 0; i < GRID.columns; ++i)
        {
            ground(i, j) = 2.0f * float(i);
        }
    }
    compages::Ray down;
    down.origin = Vector3f(13.0f, 50.0f, 22.0f);
    down.direction = Vector3f(0.0f, -1.0f, 0.0f);
    const std::optional<Vector3f> hit = compages::intersect(down, ground);
    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->y, 3.0f, 0.01f);

    compages::Ray up = down;
    up.direction = Vector3f(0.0f, 1.0f, 0.0f);
    EXPECT_FALSE(compages::intersect(up, ground).has_value());

    compages::Ray beside = down;
    beside.origin.x = 100.0f;
    EXPECT_FALSE(compages::intersect(beside, ground).has_value());

    // A slanted ray, toward +x, down onto the slope.
    compages::Ray slanted;
    slanted.origin = Vector3f(0.0f, 10.0f, 22.0f);
    slanted.direction = Vector3f(0.8f, -0.6f, 0.0f);
    const std::optional<Vector3f> slope = compages::intersect(slanted, ground);
    ASSERT_TRUE(slope.has_value());
    EXPECT_NEAR(slope->x, 10.0f + (10.0f / 7.0f), 0.05f);
    EXPECT_NEAR(slope->y, slope->x - 10.0f, 0.05f);
}

TEST(HeightField, WalksAnyGround)
{
    compages::Ray ray;
    ray.origin = Vector3f(0.0f, 10.0f, 0.0f);
    ray.direction = Vector3f(0.0f, -1.0f, 0.0f);
    auto flat = [](float, float) { return 4.0f; };
    const std::optional<float> t = compages::intersectHeight(ray, flat, 0.0f, 20.0f, 1.0f);
    ASSERT_TRUE(t.has_value());
    EXPECT_NEAR(*t, 6.0f, 0.01f);
    EXPECT_FALSE(compages::intersectHeight(ray, flat, 0.0f, 3.0f, 1.0f).has_value());
}
