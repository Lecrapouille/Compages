// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"


#include "Compages/World/Entity.hpp"
#include "Compages/World/World.hpp"

#include <stdexcept>



TEST(WorldHierarchy, StartsWithNothing)
{
    compages::world::World world;
    ASSERT_EQ(world.living(), 0u);
}

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

TEST(WorldHierarchy, StaleHandleAfterDestroy)
{
    compages::world::World world;
    compages::world::EntityId obj = world.create("obj");
    world.destroy(obj);
    ASSERT_FALSE(world.alive(obj));
    ASSERT_EQ(world.descendantCount(obj), 0u);
}

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

// Regression: KeepWorld used to give a wrong rotation as soon as the child or
// one of its parents was rotated (the rotation matrix was not transposed before
// being turned into a quaternion). The existing test only used translations.
TEST(WorldHierarchy, KeepWorldReparentPreservesRotatedAndScaledPose)
{

    compages::world::World world;
    compages::world::Entity parent = world.entity("parent")
                        .position(3.0f, 1.0f, -2.0f)
                        .rotation(1.0f, compages::Vector3f(0.0f, 1.0f, 0.0f))
                        .scale(2.0f);
    compages::world::Entity child = parent.child("child")
                       .position(1.0f, 2.0f, 3.0f)
                       .rotation(0.5f, compages::Vector3f(1.0f, 0.0f, 0.0f))
                       .scale(1.0f, 2.0f, 3.0f);
    compages::Frame frame;
    frame.elapsed = 0.0f;
    world.update(frame);
    const compages::Matrix44f before = world.worldMatrix(child.id());

    ASSERT_TRUE(bool(world.setParent(child.id(), compages::world::EntityId{},
                                     compages::world::ReparentPolicy::KeepWorld)));
    world.update(frame);
    const compages::Matrix44f after = world.worldMatrix(child.id());

    for (std::size_t row = 0u; row < 4u; ++row)
    {
        for (std::size_t col = 0u; col < 4u; ++col)
        {
            EXPECT_NEAR(before[row][col], after[row][col], 1.0e-4f)
                << "row " << row << " column " << col;
        }
    }
}

// Regression: the spatial graph used 16-bit node indices and the World
// refused more than 65535 living entities.
TEST(WorldHierarchy, HoldsMoreThan65535LinkedEntities)
{
    constexpr std::uint32_t COUNT = 70000u;
    compages::world::World world;
    compages::world::EntityId root = world.create("root");
    compages::world::EntityId last;
    for (std::uint32_t i = 0u; i < COUNT; ++i)
    {
        last = world.create();
        ASSERT_TRUE(world.setParent(last, root));
    }
    ASSERT_EQ(world.living(), COUNT + 1u);
    ASSERT_EQ(world.parent(last), root);
    ASSERT_EQ(world.descendantCount(root), COUNT + 1u);
    world.destroy(root);
    ASSERT_EQ(world.living(), 0u);
}

// A chain deeper than the call stack allows with recursion: update and
// destroy must walk it with a loop.
TEST(WorldHierarchy, UpdatesAndDestroysAVeryDeepChain)
{
    constexpr std::uint32_t DEPTH = 200000u;
    compages::world::World world;
    compages::world::Entity root = world.entity("root");
    compages::world::Entity tip = root;
    for (std::uint32_t i = 0u; i < DEPTH; ++i)
    {
        tip = tip.child();
    }
    root.position(1.0f, 2.0f, 3.0f);
    world.update();
    const compages::Matrix44f at = world.worldMatrix(tip.id());
    EXPECT_FLOAT_EQ(at(0, 3), 1.0f);
    EXPECT_FLOAT_EQ(at(1, 3), 2.0f);
    EXPECT_FLOAT_EQ(at(2, 3), 3.0f);
    ASSERT_EQ(world.descendantCount(root.id()), DEPTH + 1u);
    world.destroy(root.id());
    ASSERT_EQ(world.living(), 0u);
}

TEST(WorldHierarchy, EntitySetParentReportsFailureInsteadOfAsserting)
{
    compages::world::World world;
    compages::world::Entity a = world.entity("a");
    compages::world::Entity b = a.child("b");

    EXPECT_FALSE(bool(a.setParent(b)));
    EXPECT_TRUE(bool(b.setParent(a)));
}
