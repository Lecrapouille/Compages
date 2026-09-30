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


#include "Compages/World/Entity.hpp"
#include "Compages/World/World.hpp"

#include <stdexcept>



//------------------------------------------------------------------------------
TEST(WorldHierarchy, StartsWithNothing)
{
    compages::world::World world;
    ASSERT_EQ(world.living(), 0u);
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, AttachesAChildAndCountsTheTree)
{
    compages::world::World world;
    compages::world::EntityId obj0 = world.create("obj0");
    ASSERT_FALSE(world.parent(obj0).valid());
    ASSERT_FALSE(world.firstChild(obj0).valid());
    ASSERT_EQ(world.descendantCount(obj0), 1u);

    compages::world::EntityId obj1 = world.create("obj1");
    ASSERT_TRUE(bool(world.setParent(obj1, obj0)));
    ASSERT_EQ(world.descendantCount(obj0), 2u);
    ASSERT_EQ(world.parent(obj1), obj0);
    ASSERT_EQ(world.firstChild(obj0), obj1);

    compages::world::EntityId obj2 = world.create("obj2");
    ASSERT_TRUE(bool(world.setParent(obj2, obj1)));
    ASSERT_EQ(world.descendantCount(obj0), 3u);
    ASSERT_EQ(world.find(obj0, "obj1/obj2"), obj2);
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, DestroyDropsWholeSubtree)
{
    compages::world::World world;
    compages::world::EntityId a = world.create("a");
    compages::world::EntityId b = world.create("b");
    compages::world::EntityId c = world.create("c");
    ASSERT_TRUE(bool(world.setParent(b, a)));
    ASSERT_TRUE(bool(world.setParent(c, b)));

    world.destroy(a);
    ASSERT_FALSE(world.alive(a));
    ASSERT_FALSE(world.alive(b));
    ASSERT_FALSE(world.alive(c));
    ASSERT_EQ(world.living(), 0u);
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, EnableAndDisablePropagates)
{
    compages::world::World world;
    compages::world::EntityId a = world.create("a");
    compages::world::EntityId b = world.create("b");
    ASSERT_TRUE(bool(world.setParent(b, a)));
    ASSERT_TRUE(world.enabled(a));
    ASSERT_TRUE(world.enabledInHierarchy(b));

    world.setEnabled(a, false);
    ASSERT_FALSE(world.enabled(a));
    ASSERT_TRUE(world.enabled(b));
    ASSERT_FALSE(world.enabledInHierarchy(b));
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, StaleHandleAfterDestroy)
{
    compages::world::World world;
    compages::world::EntityId obj = world.create("obj");
    world.destroy(obj);
    ASSERT_FALSE(world.alive(obj));
    ASSERT_EQ(world.descendantCount(obj), 0u);
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, ReusedSlotDoesNotReviveTheOldHandle)
{
    compages::world::World world;
    compages::world::EntityId first = world.create("first");
    world.destroy(first);
    compages::world::EntityId second = world.create("second");
    ASSERT_TRUE(world.alive(second));
    ASSERT_FALSE(world.alive(first));
    ASSERT_EQ(second.index(), first.index());
    ASSERT_NE(second.generation(), first.generation());
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, RefusesACycle)
{
    compages::world::World world;
    compages::world::EntityId a = world.create("a");
    compages::world::EntityId b = world.create("b");
    ASSERT_TRUE(bool(world.setParent(b, a)));
    auto cycled = world.setParent(a, b);
    ASSERT_FALSE(bool(cycled));
    ASSERT_THAT(cycled.error(), HasSubstr("cycle"));
}

//------------------------------------------------------------------------------
// Regression: KeepWorld used to give a wrong rotation as soon as the child or
// one of its parents was rotated (the rotation matrix was not transposed before
// being turned into a quaternion). The existing test only used translations.
TEST(WorldHierarchy, KeepWorldReparentPreservesRotatedAndScaledPose)
{

    compages::world::World world;
    compages::world::Entity parent = world.entity("parent")
                        .position(3.0f, 1.0f, -2.0f)
                        .rotation(1.0f, compages::core::Vector3f(0.0f, 1.0f, 0.0f))
                        .scale(2.0f);
    compages::world::Entity child = parent.child("child")
                       .position(1.0f, 2.0f, 3.0f)
                       .rotation(0.5f, compages::core::Vector3f(1.0f, 0.0f, 0.0f))
                       .scale(1.0f, 2.0f, 3.0f);
    compages::core::Frame frame;
    frame.elapsed = 0.0f;
    world.update(frame);
    const compages::core::Matrix44f before = world.worldMatrix(child.id());

    ASSERT_TRUE(bool(world.setParent(child.id(), compages::world::EntityId{},
                                     compages::world::ReparentPolicy::KeepWorld)));
    world.update(frame);
    const compages::core::Matrix44f after = world.worldMatrix(child.id());

    for (std::size_t row = 0u; row < 4u; ++row)
    {
        for (std::size_t col = 0u; col < 4u; ++col)
        {
            EXPECT_NEAR(before[row][col], after[row][col], 1.0e-4f)
                << "row " << row << " column " << col;
        }
    }
}

//------------------------------------------------------------------------------
// Regression: past 65535 living entities the spatial graph (16-bit node index)
// silently linked the wrong nodes in release builds.
TEST(WorldHierarchy, RefusesMoreLivingEntitiesThanTheGraphCanIndex)
{
    compages::world::World world;
    for (std::uint32_t i = 0u; i < compages::world::EntityId::MAX_COUNT; ++i)
    {
        (void)world.create();
    }
    ASSERT_EQ(world.living(), compages::world::EntityId::MAX_COUNT);
    EXPECT_THROW((void)world.create(), std::length_error);
    ASSERT_EQ(world.living(), compages::world::EntityId::MAX_COUNT);
}

//------------------------------------------------------------------------------
TEST(WorldHierarchy, EntitySetParentReportsFailureInsteadOfAsserting)
{
    compages::world::World world;
    compages::world::Entity a = world.entity("a");
    compages::world::Entity b = a.child("b");

    EXPECT_FALSE(bool(a.setParent(b)));
    EXPECT_TRUE(bool(b.setParent(a)));
}
