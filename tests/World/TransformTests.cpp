// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/World/World.hpp"

// A child sits on its parent. Moving the parent must move the child, which is
// the whole reason the graph exists.
TEST(WorldTransform, ChildFollowsParent)
{
    compages::world::World world;
    compages::world::EntityId body = world.create("body");
    compages::world::EntityId head = world.create("head");
    ASSERT_TRUE(bool(world.setParent(head, body)));

    world.transform(body).position = Vector3f(10.0f, 0.0f, 0.0f);
    world.transform(head).position = Vector3f(0.0f, 2.0f, 0.0f);
    world.update();

    const Matrix44f& world_head = world.worldMatrix(head);
    ASSERT_NEAR(world_head[3].x, 10.0f, 1.0e-4f);
    ASSERT_NEAR(world_head[3].y, 2.0f, 1.0e-4f);
}

// Scale is inherited by children through the standard TRS composition, the
// way Three.js and Unity do it. A child at (0, 1, 0) local under a parent of
// scale (10, 10, 10) ends up at world y = 10.
TEST(WorldTransform, ScaleIsInheritedByChildren)
{
    compages::world::World world;
    compages::world::EntityId root = world.create("root");
    compages::world::EntityId child = world.create("child");
    ASSERT_TRUE(bool(world.setParent(child, root)));

    world.transform(root).scale = Vector3f(10.0f, 10.0f, 10.0f);
    world.transform(child).position = Vector3f(0.0f, 1.0f, 0.0f);
    world.update();

    const Matrix44f& world_child = world.worldMatrix(child);
    ASSERT_NEAR(world_child[3].y, 10.0f, 1.0e-3f);
}

// Dirty propagation: writing to a parent's transform re-runs its children's
// world matrices next update.
TEST(WorldTransform, WritingParentDirtiesChildren)
{
    compages::world::World world;
    compages::world::EntityId a = world.create("a");
    compages::world::EntityId b = world.create("b");
    ASSERT_TRUE(bool(world.setParent(b, a)));

    world.transform(a).position = Vector3f(0.0f, 0.0f, 0.0f);
    world.transform(b).position = Vector3f(1.0f, 0.0f, 0.0f);
    world.update();
    ASSERT_NEAR(world.worldMatrix(b)[3].x, 1.0f, 1.0e-4f);

    world.transform(a).position = Vector3f(5.0f, 0.0f, 0.0f);
    world.update();
    ASSERT_NEAR(world.worldMatrix(b)[3].x, 6.0f, 1.0e-4f);
}

// A destroyed subtree does not leak transforms.
TEST(WorldTransform, DestroyReleasesTransform)
{
    compages::world::World world;
    compages::world::EntityId e = world.create("e");
    ASSERT_TRUE(world.transforms().has(e));
    world.destroy(e);
    ASSERT_FALSE(world.transforms().has(e));
}
