// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Transformation.hpp"



TEST(AABB, StartsEmpty)
{
    const compages::core::AABB box;
    ASSERT_TRUE(box.empty());
}

TEST(AABB, IntersectsOverlappingBoxes)
{
    const compages::core::AABB a = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                          compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    const compages::core::AABB b = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.5f, 0.0f, 0.0f),
                                          compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    ASSERT_TRUE(a.intersects(b));
    ASSERT_FALSE(a.intersects(compages::core::AABB{}));
}

TEST(AABB, HoldsTheCornersItWasBuiltFrom)
{
    const compages::core::AABB box = compages::core::AABB::fromCorners(compages::core::Vector3f(1.0f, 2.0f, 3.0f),
                                       compages::core::Vector3f(-1.0f, 4.0f, 0.0f));
    ASSERT_FALSE(box.empty());
    ASSERT_FLOAT_EQ(box.min.x, -1.0f);
    ASSERT_FLOAT_EQ(box.min.y, 2.0f);
    ASSERT_FLOAT_EQ(box.min.z, 0.0f);
    ASSERT_FLOAT_EQ(box.max.x, 1.0f);
    ASSERT_FLOAT_EQ(box.max.y, 4.0f);
    ASSERT_FLOAT_EQ(box.max.z, 3.0f);
}

TEST(AABB, GrowsToHoldAPoint)
{
    compages::core::AABB box = compages::core::AABB::fromCorners(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                 compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    box.expand(compages::core::Vector3f(2.0f, -1.0f, 0.5f));
    ASSERT_FLOAT_EQ(box.min.y, -1.0f);
    ASSERT_FLOAT_EQ(box.max.x, 2.0f);
}

// A translation must move the box the way the shader moves a vertex: the
// matrices of this library put the translation in the last row of the CPU
// array, which is the last column the shader reads.
TEST(AABB, FollowsATranslationTheShaderWouldApply)
{
    const compages::core::AABB local = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                              compages::core::Vector3f(1.0f, 1.0f, 1.0f));
    const compages::core::Matrix44f model =
        compages::core::translate(compages::core::Matrix44f(compages::core::matrix::Identity),
                          compages::core::Vector3f(10.0f, 0.0f, 0.0f));
    const compages::core::AABB world = local.transformed(model);
    ASSERT_NEAR(world.center().x, 10.0f, 1.0e-5f);
    ASSERT_NEAR(world.min.x, 9.0f, 1.0e-5f);
    ASSERT_NEAR(world.max.x, 11.0f, 1.0e-5f);
}
