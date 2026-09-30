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

//------------------------------------------------------------------------------
TEST(Frustum, KeepsABoxInFrontOfTheCamera)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_TRUE(frustum.contains(box));
}

//------------------------------------------------------------------------------
TEST(Frustum, DropsABoxBehindTheCamera)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 50.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_FALSE(frustum.contains(box));
}

//------------------------------------------------------------------------------
TEST(Frustum, DropsAnEmptyBox)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    ASSERT_FALSE(frustum.contains(compages::core::AABB{}));
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
TEST(Frustum, DropsABoxBesideTheCamera)
{
    const compages::core::Frustum frustum = lookingAtOrigin();
    const compages::core::AABB box = compages::core::AABB::fromCenterExtent(compages::core::Vector3f(50.0f, 0.0f, 0.0f),
                                            compages::core::Vector3f(0.5f, 0.5f, 0.5f));
    ASSERT_FALSE(frustum.contains(box));
}

//------------------------------------------------------------------------------
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
