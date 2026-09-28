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

#include "Compages/World/Entity.hpp"
#include "main.hpp"

#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/World/World.hpp"

namespace
{

struct Counter : compages::world::Behavior
{
    int starts = 0;
    int updates = 0;
    float last_dt = 0.0f;

    void start() override
    {
        ++starts;
    }

    void update(float p_dt) override
    {
        ++updates;
        last_dt = p_dt;
    }
};

struct Mover : compages::world::Behavior
{
    explicit Mover(float p_speed) : speed(p_speed) {}

    void update(float p_dt) override
    {
        transform().position.x += speed * p_dt;
    }

    float speed;
};

struct SelfRemover : compages::world::Behavior
{
    void update(float /*p_dt*/) override
    {
        entity().remove<SelfRemover>();
    }
};

struct Velocity
{
    float x = 0.0f;
};

compages::world::Frame frameOf(float p_dt)
{
    compages::world::Frame frame;
    frame.width = 640u;
    frame.height = 480u;
    frame.elapsed = p_dt;
    return frame;
}

float distanceBetween(Vector3f const& p_a, Vector3f const& p_b)
{
    const Vector3f d = p_a - p_b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

} // namespace

//------------------------------------------------------------------------------
TEST(Behavior, StartsOnceThenUpdatesEveryFrame)
{
    compages::world::World world;
    compages::world::Entity actor = world.entity("Actor").add<Counter>();

    world.update(frameOf(0.5f));
    world.update(frameOf(0.25f));

    Counter& counter = actor.get<Counter>();
    EXPECT_EQ(counter.starts, 1);
    EXPECT_EQ(counter.updates, 2);
    EXPECT_FLOAT_EQ(counter.last_dt, 0.25f);
}

//------------------------------------------------------------------------------
TEST(Behavior, TakesConstructorArgumentsAndMovesItsEntity)
{
    compages::world::World world;
    compages::world::Entity actor = world.entity("Actor").add<Mover>(2.0f);

    world.update(frameOf(0.5f));

    EXPECT_FLOAT_EQ(actor.position().x, 1.0f);
}

//------------------------------------------------------------------------------
TEST(Behavior, DisabledEntitiesAndTheirChildrenDoNotRun)
{
    compages::world::World world;
    compages::world::Entity parent = world.entity("Parent");
    compages::world::Entity child = parent.child("Child").add<Counter>();

    parent.enable(false);
    world.update(frameOf(0.1f));
    EXPECT_EQ(child.get<Counter>().updates, 0);

    parent.enable(true);
    world.update(frameOf(0.1f));
    EXPECT_EQ(child.get<Counter>().updates, 1);
}

//------------------------------------------------------------------------------
TEST(Behavior, CanRemoveItselfWhileRunning)
{
    compages::world::World world;
    compages::world::Entity actor = world.entity("Actor").add<SelfRemover>().add<Counter>();

    world.update(frameOf(0.1f));

    EXPECT_FALSE(actor.has<SelfRemover>());
    EXPECT_TRUE(actor.has<Counter>());
    EXPECT_EQ(actor.get<Counter>().updates, 1);
}

//------------------------------------------------------------------------------
TEST(Behavior, HasAndFindTellBehaviorsFromComponents)
{
    compages::world::World world;
    compages::world::Entity actor = world.entity("Actor").set(Velocity{ 3.0f });

    EXPECT_TRUE(actor.has<Velocity>());
    EXPECT_FALSE(actor.has<Counter>());
    EXPECT_EQ(actor.find<Counter>(), nullptr);

    actor.add<Counter>();
    EXPECT_TRUE(actor.has<Counter>());
    EXPECT_NE(actor.find<Counter>(), nullptr);
    EXPECT_FLOAT_EQ(actor.get<Velocity>().x, 3.0f);
}

//------------------------------------------------------------------------------
TEST(Entity, LookupFollowsNamesFromTheRoot)
{
    compages::world::World world;
    compages::world::Entity body = world.entity("Body");
    compages::world::Entity head = body.child("Head");

    EXPECT_EQ(world.lookup("Body/Head"), head);
    EXPECT_EQ(world.lookup("/Body/Head"), head);
    EXPECT_EQ(body.lookup("Head"), head);
    EXPECT_FALSE(world.lookup("Body/Tail"));
}

//------------------------------------------------------------------------------
TEST(Entity, EachVisitsEntitiesWithEveryComponent)
{
    compages::world::World world;
    world.entity("A").set(Velocity{ 1.0f });
    world.entity("B").set(Velocity{ 2.0f });
    world.entity("C");

    float sum = 0.0f;
    world.each<Velocity>([&](compages::world::Entity, Velocity& p_velocity) { sum += p_velocity.x; });
    EXPECT_FLOAT_EQ(sum, 3.0f);
}

//------------------------------------------------------------------------------
TEST(Orbit, StartsFromWhereTheCameraWasPlaced)
{
    const Vector3f targets[] = { Vector3f(0.0f, 0.0f, 0.0f),
                                 Vector3f(0.0f, 8.0f, 0.0f),
                                 Vector3f(1.0f, 2.0f, -3.0f) };
    const Vector3f places[] = { Vector3f(0.0f, 7.0f, 14.0f),
                                Vector3f(0.0f, 25.0f, 90.0f),
                                Vector3f(-6.0f, 1.0f, 4.0f) };
    for (Vector3f const& target : targets)
    {
        for (Vector3f const& place : places)
        {
            compages::world::World world;
            compages::world::Entity camera =
                world.entity("Camera").position(place).add<compages::world::Orbit>(target);

            world.update(frameOf(0.016f));

            EXPECT_LT(distanceBetween(camera.position(), place), 1.0e-3f)
                << "target " << target.x << ',' << target.y << ',' << target.z
                << " place " << place.x << ',' << place.y << ',' << place.z;
        }
    }
}

//------------------------------------------------------------------------------
TEST(Fly, KeepsTheDirectionTheCameraWasLooking)
{
    compages::world::World world;
    compages::world::Entity camera = world.entity("Camera").position(0.0f, 3.0f, 10.0f);
    camera.lookAt(2.0f, 0.0f, 0.0f);
    const Quatf before = camera.rotation();
    const Vector3f forward_before = before * Vector3f(0.0f, 0.0f, -1.0f);

    camera.add<compages::world::Fly>();
    world.update(frameOf(0.016f));

    const Vector3f forward_after = camera.rotation() * Vector3f(0.0f, 0.0f, -1.0f);
    EXPECT_LT(distanceBetween(forward_before, forward_after), 1.0e-3f);
    EXPECT_LT(distanceBetween(camera.position(), Vector3f(0.0f, 3.0f, 10.0f)), 1.0e-3f);
}
