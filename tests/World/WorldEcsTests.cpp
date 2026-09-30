// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "main.hpp"


#include "Compages/World/World.hpp"



namespace
{
struct Payload
{
    int value = 0;
};
} // namespace

TEST(WorldEcs, CreatesDestroysAndRejectsStaleEntities)
{
    compages::world::World world;
    const compages::world::EntityId first = world.create("first");
    ASSERT_TRUE(world.alive(first));
    ASSERT_EQ(world.living(), 1u);

    world.destroy(first);
    ASSERT_FALSE(world.alive(first));
    ASSERT_EQ(world.living(), 0u);

    const compages::world::EntityId replacement = world.create("replacement");
    ASSERT_TRUE(world.alive(replacement));
    ASSERT_NE(first, replacement);
}

TEST(WorldEcs, AddsReplacesRemovesAndIteratesComponents)
{
    compages::world::World world;
    const compages::world::EntityId first = world.create();
    const compages::world::EntityId second = world.create();

    world.add(first, Payload{ 2 });
    world.add(second, Payload{ 3 });
    world.add(first, Payload{ 5 });

    int total = 0;
    world.each<Payload>(
        [&](compages::world::EntityId entity, Payload& payload) {
            ASSERT_TRUE(world.alive(entity));
            total += payload.value;
        });
    ASSERT_EQ(total, 8);

    world.remove<Payload>(first);
    ASSERT_FALSE(world.has<Payload>(first));
    ASSERT_EQ(world.view<Payload>().size(), 1u);
}

TEST(WorldEcs, EntityRefProvidesTheErgonomicApi)
{
    compages::world::World world;
    compages::world::Entity object = world.entity("object");
    object.add<Payload>(42);
    ASSERT_EQ(object.get<Payload>().value, 42);

    object.transform().position = compages::core::Vector3f(1.0f, 2.0f, 3.0f);
    world.update();
    compages::core::Vector3f const position = world.transform(object.id()).position;
    ASSERT_FLOAT_EQ(position.x, 1.0f);
    ASSERT_FLOAT_EQ(position.y, 2.0f);
    ASSERT_FLOAT_EQ(position.z, 3.0f);
}
