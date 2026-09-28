//==============================================================================
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
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Transformation.hpp"

//------------------------------------------------------------------------------
TEST(AABB, StartsEmpty)
{
    const AABB box;
    ASSERT_TRUE(box.empty());
}

//------------------------------------------------------------------------------
TEST(AABB, IntersectsOverlappingBoxes)
{
    const AABB a = AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(1.0f, 1.0f, 1.0f));
    const AABB b = AABB::fromCenterExtent(Vector3f(0.5f, 0.0f, 0.0f),
                                          Vector3f(1.0f, 1.0f, 1.0f));
    ASSERT_TRUE(a.intersects(b));
    ASSERT_FALSE(a.intersects(AABB{}));
}

//------------------------------------------------------------------------------
TEST(AABB, HoldsTheCornersItWasBuiltFrom)
{
    const AABB box = AABB::fromCorners(Vector3f(1.0f, 2.0f, 3.0f),
                                       Vector3f(-1.0f, 4.0f, 0.0f));
    ASSERT_FALSE(box.empty());
    ASSERT_FLOAT_EQ(box.min.x, -1.0f);
    ASSERT_FLOAT_EQ(box.min.y, 2.0f);
    ASSERT_FLOAT_EQ(box.min.z, 0.0f);
    ASSERT_FLOAT_EQ(box.max.x, 1.0f);
    ASSERT_FLOAT_EQ(box.max.y, 4.0f);
    ASSERT_FLOAT_EQ(box.max.z, 3.0f);
}

//------------------------------------------------------------------------------
TEST(AABB, GrowsToHoldAPoint)
{
    AABB box = AABB::fromCorners(Vector3f(0.0f, 0.0f, 0.0f),
                                 Vector3f(1.0f, 1.0f, 1.0f));
    box.expand(Vector3f(2.0f, -1.0f, 0.5f));
    ASSERT_FLOAT_EQ(box.min.y, -1.0f);
    ASSERT_FLOAT_EQ(box.max.x, 2.0f);
}

//------------------------------------------------------------------------------
// A translation must move the box the way the shader moves a vertex: the
// matrices of this library put the translation in the last row of the CPU
// array, which is the last column the shader reads.
//------------------------------------------------------------------------------
TEST(AABB, FollowsATranslationTheShaderWouldApply)
{
    const AABB local = AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f),
                                              Vector3f(1.0f, 1.0f, 1.0f));
    const Matrix44f model =
        compages::matrix::translate(Matrix44f(compages::matrix::Identity),
                          Vector3f(10.0f, 0.0f, 0.0f));
    const AABB world = local.transformed(model);
    ASSERT_NEAR(world.center().x, 10.0f, 1.0e-5f);
    ASSERT_NEAR(world.min.x, 9.0f, 1.0e-5f);
    ASSERT_NEAR(world.max.x, 11.0f, 1.0e-5f);
}
