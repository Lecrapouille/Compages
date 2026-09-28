// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/Core/Frustum.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Units.hpp"

using namespace units::literals;

namespace
{

Frustum lookingAtOrigin()
{
    const Matrix44f view = compages::matrix::lookAt(Vector3f(0.0f, 0.0f, 10.0f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f projection =
        compages::matrix::perspective(60.0_deg, 1.0f, 0.1f, 100.0f);
    // Row-vector convention: applying view then projection to a point p is
    // p * view * projection, so the combined matrix is view * projection.
    return Frustum::fromViewProjection(view * projection);
}

} // namespace

TEST(Frustum, KeepsABoxInFrontOfTheCamera)
{
    const Frustum frustum = lookingAtOrigin();
    const AABB box = AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f),
                                            Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_TRUE(frustum.contains(box));
}

TEST(Frustum, DropsABoxBehindTheCamera)
{
    const Frustum frustum = lookingAtOrigin();
    const AABB box = AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 50.0f),
                                            Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_FALSE(frustum.contains(box));
}

TEST(Frustum, DropsAnEmptyBox)
{
    const Frustum frustum = lookingAtOrigin();
    ASSERT_FALSE(frustum.contains(AABB{}));
}

TEST(Frustum, KeepsABoxWhenTheCameraIsCloser)
{
    const Matrix44f view = compages::matrix::lookAt(Vector3f(0.0f, 0.0f, 3.0f),
                                          Vector3f(0.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f projection =
        compages::matrix::perspective(60.0_deg, 1.0f, 0.1f, 20.0f);
    const Frustum frustum = Frustum::fromViewProjection(view * projection);
    const AABB box = AABB::fromCenterExtent(Vector3f(0.0f, 0.0f, 0.0f),
                                            Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_TRUE(frustum.contains(box));
}

TEST(Frustum, DropsABoxBesideTheCamera)
{
    const Frustum frustum = lookingAtOrigin();
    const AABB box = AABB::fromCenterExtent(Vector3f(50.0f, 0.0f, 0.0f),
                                            Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_FALSE(frustum.contains(box));
}

TEST(Frustum, KeepsTheMovingRobotBodies)
{
    const Matrix44f view = compages::matrix::lookAt(Vector3f(0.0f, 10.0f, 100.0f),
                                          Vector3f(30.0f, 30.0f, 30.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f));
    const Matrix44f projection =
        compages::matrix::perspective(60.0_deg, 800.0f / 600.0f, 0.1f, 10000.0f);
    const Frustum frustum = Frustum::fromViewProjection(view * projection);
    const Vector3f extent(10.0f, 15.0f, 5.0f);
    ASSERT_TRUE(frustum.contains(
        AABB::fromCenterExtent(Vector3f(0.0f, 50.0f, 0.0f), extent)));
    ASSERT_TRUE(frustum.contains(
        AABB::fromCenterExtent(Vector3f(30.0f, 50.0f, 0.0f), extent)));
    ASSERT_TRUE(frustum.contains(
        AABB::fromCenterExtent(Vector3f(60.0f, 50.0f, 0.0f), extent)));
}
