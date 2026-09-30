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

compages::core::Frustum lookingAtOrigin()
{
    const compages::core::Matrix44f view = compages::core::lookAt(compages::core::Vector3f(0.0f, 0.0f, 10.0f),
                                          compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                          compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Matrix44f projection =
        compages::core::perspective(60.0_deg, 1.0f, 0.1f, 100.0f);
    return compages::core::Frustum::fromViewProjection(projection * view);
}

} // namespace

TEST(Frustum, KeepsABoxInFrontOfTheCamera)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_TRUE(frustum.contains(box));
}

TEST(Frustum, DropsABoxBehindTheCamera)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 50.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_FALSE(frustum.contains(box));
}

TEST(Frustum, DropsAnEmptyBox)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    ASSERT_FALSE(frustum.contains(compages::core::AABB{}));
}

TEST(Frustum, KeepsABoxWhenTheCameraIsCloser)
{
    const compages::core::Matrix44f view = compages::core::lookAt(compages::core::Vector3f(0.0f, 0.0f, 3.0f),
                                          compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                          compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Matrix44f projection =
        compages::core::perspective(60.0_deg, 1.0f, 0.1f, 20.0f);
    const compages::core::Frustum frustum =
        compages::core::Frustum::fromViewProjection(projection * view);
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_TRUE(frustum.contains(box));
}

TEST(Frustum, DropsABoxBesideTheCamera)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(50.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_FALSE(frustum.contains(box));
}

TEST(Frustum, KeepsTheMovingRobotBodies)
{
    const compages::core::Matrix44f view = compages::core::lookAt(compages::core::Vector3f(0.0f, 10.0f, 100.0f),
                                          compages::core::Vector3f(30.0f, 30.0f, 30.0f),
                                          compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    const compages::core::Matrix44f projection =
        compages::core::perspective(60.0_deg, 800.0f / 600.0f, 0.1f, 10000.0f);
    const compages::core::Frustum frustum =
        compages::core::Frustum::fromViewProjection(projection * view);
    const compages::core::Vector3f extent(10.0f, 15.0f, 5.0f);
    ASSERT_TRUE(frustum.contains(
        compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 50.0f, 0.0f), extent)));
    ASSERT_TRUE(frustum.contains(
        compages::core::AABB::fromCenterExtent(compages::core::Vector3f(30.0f, 50.0f, 0.0f), extent)));
    ASSERT_TRUE(frustum.contains(
        compages::core::AABB::fromCenterExtent(compages::core::Vector3f(60.0f, 50.0f, 0.0f), extent)));
}
