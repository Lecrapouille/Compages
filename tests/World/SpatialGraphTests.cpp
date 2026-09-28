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

#include "Compages/World/Spatial/SpatialGraph.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"
#include "Compages/World/World.hpp"

//------------------------------------------------------------------------------
TEST(SpatialGraph, AttachIsIdempotent)
{
    compages::world::World world;
    compages::world::SpatialGraph graph;

    compages::world::EntityId e = world.create();
    compages::world::NodeId a = graph.attach(e);
    compages::world::NodeId b = graph.attach(e);
    ASSERT_EQ(a, b);
    ASSERT_EQ(graph.size(), 1u);
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, ParentAndChildLink)
{
    compages::world::World world;
    compages::world::SpatialGraph graph;

    compages::world::EntityId ea = world.create();
    compages::world::EntityId eb = world.create();
    compages::world::NodeId a = graph.attach(ea);
    compages::world::NodeId b = graph.attach(eb);

    ASSERT_TRUE(bool(graph.setParent(b, a)));
    ASSERT_EQ(graph.parent(b), a);
    ASSERT_EQ(graph.firstChild(a), b);
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, RefusesACycle)
{
    compages::world::World world;
    compages::world::SpatialGraph graph;

    compages::world::EntityId ea = world.create();
    compages::world::EntityId eb = world.create();
    compages::world::NodeId a = graph.attach(ea);
    compages::world::NodeId b = graph.attach(eb);
    ASSERT_TRUE(bool(graph.setParent(b, a)));

    auto cycled = graph.setParent(a, b);
    ASSERT_FALSE(bool(cycled));
    ASSERT_THAT(cycled.error(), HasSubstr("cycle"));
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, DestroySubtree)
{
    compages::world::World world;
    compages::world::SpatialGraph graph;

    compages::world::EntityId ea = world.create();
    compages::world::EntityId eb = world.create();
    compages::world::EntityId ec = world.create();
    compages::world::NodeId a = graph.attach(ea);
    compages::world::NodeId b = graph.attach(eb);
    compages::world::NodeId c = graph.attach(ec);
    ASSERT_TRUE(bool(graph.setParent(b, a)));
    ASSERT_TRUE(bool(graph.setParent(c, b)));
    ASSERT_EQ(graph.descendantCount(a), 3u);

    graph.destroy(a);
    ASSERT_FALSE(graph.alive(a));
    ASSERT_FALSE(graph.alive(b));
    ASSERT_FALSE(graph.alive(c));
    ASSERT_EQ(graph.size(), 0u);
}

//------------------------------------------------------------------------------
TEST(SpatialGraph, KeepWorldReparentPreservesWorldPose)
{
    compages::world::World world;
    compages::world::SpatialGraph graph;
    compages::world::TransformStore transforms;

    compages::world::EntityId er = world.create();
    compages::world::EntityId ea = world.create();
    compages::world::EntityId eb = world.create();
    compages::world::EntityId ec = world.create();
    transforms.allocate(er);
    transforms.allocate(ea);
    transforms.allocate(eb);
    transforms.allocate(ec);

    compages::world::NodeId a = graph.attach(ea);
    compages::world::NodeId b = graph.attach(eb);
    compages::world::NodeId c = graph.attach(ec);
    ASSERT_TRUE(bool(graph.setParent(b, a)));
    ASSERT_TRUE(bool(graph.setParent(c, b)));

    // A at (10, 0, 0). B at (0, 5, 0) relative to A. C at (0, 2, 0) relative
    // to B. World position of C is (10, 7, 0).
    transforms.localMutable(ea).position = Vector3f(10.0f, 0.0f, 0.0f);
    transforms.localMutable(eb).position = Vector3f(0.0f, 5.0f, 0.0f);
    transforms.localMutable(ec).position = Vector3f(0.0f, 2.0f, 0.0f);

    // Manually build world matrices (no TransformSystem here).
    transforms.setWorld(ea, compages::world::localMatrix(transforms.local(ea)));
    transforms.setWorld(eb,
                        compages::world::localMatrix(transforms.local(eb)) * transforms.world(ea));
    transforms.setWorld(ec,
                        compages::world::localMatrix(transforms.local(ec)) * transforms.world(eb));

    // Reparent C from B to A, keeping the world pose.
    ASSERT_TRUE(bool(graph.setParent(c, a, compages::world::ReparentPolicy::KeepWorld,
                                     &transforms)));

    // C's world position should still be (10, 7, 0), so its new local
    // relative to A should be (0, 7, 0).
    ASSERT_NEAR(transforms.local(ec).position.x, 0.0f, 1.0e-3f);
    ASSERT_NEAR(transforms.local(ec).position.y, 7.0f, 1.0e-3f);
}
