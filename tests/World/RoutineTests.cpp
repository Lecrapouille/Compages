// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "main.hpp"


#include "Compages/World/Controllers/Controls.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"
#include "Compages/World/World.hpp"

#include <memory>
#include <string>
#include <vector>



namespace
{

struct Counter : compages::world::Routine
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

struct Mover : compages::world::Routine
{
    explicit Mover(float p_speed) : speed(p_speed) {}

    void update(float p_dt) override
    {
        transform().position.x += speed * p_dt;
    }

    float speed;
};

struct SelfRemover : compages::world::Routine
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

compages::core::Frame frameOf(float p_dt)
{
    compages::core::Frame frame;
    frame.width = 640u;
    frame.height = 480u;
    frame.elapsed = p_dt;
    return frame;
}

float distanceBetween(compages::core::Vector3f const& p_a, compages::core::Vector3f const& p_b)
{
    const compages::core::Vector3f d = p_a - p_b;
    return std::sqrt(d.x * d.x + d.y * d.y + d.z * d.z);
}

//! \brief Writes each call it receives into a shared log.
struct Logger : compages::world::Routine
{
    Logger(std::shared_ptr<std::vector<std::string>> p_log, std::string p_name, int p_priority = 0)
        : log(std::move(p_log)), name(std::move(p_name)), rank(p_priority)
    {
    }

    void onEnable() override { log->push_back(name + ".onEnable"); }
    void start() override { log->push_back(name + ".start"); }
    void update(float) override { log->push_back(name + ".update"); }
    void onDisable() override { log->push_back(name + ".onDisable"); }
    void onDestroy() override { log->push_back(name + ".onDestroy"); }
    [[nodiscard]] int priority() const override { return rank; }

    std::shared_ptr<std::vector<std::string>> log;
    std::string name;
    int rank;
};

struct FixedCounter : compages::world::Routine
{
    void fixedUpdate(float p_dt) override
    {
        ++calls;
        last_dt = p_dt;
    }

    int calls = 0;
    float last_dt = 0.0f;
};

//! \brief Reads, in lateUpdate(), where its entity is in the world.
struct Follower : compages::world::Routine
{
    void lateUpdate(float) override
    {
        seen = entity().worldPosition();
    }

    compages::core::Vector3f seen{ 0.0f, 0.0f, 0.0f };
};

} // namespace

TEST(Routine, StartsOnceThenUpdatesEveryFrame)
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

TEST(Routine, TakesConstructorArgumentsAndMovesItsEntity)
{
    compages::world::World world;
    compages::world::Entity actor = world.entity("Actor").add<Mover>(2.0f);

    world.update(frameOf(0.5f));

    EXPECT_FLOAT_EQ(actor.position().x, 1.0f);
}

TEST(Routine, DisabledEntitiesAndTheirChildrenDoNotRun)
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

TEST(Routine, CanRemoveItselfWhileRunning)
{
    compages::world::World world;
    compages::world::Entity actor = world.entity("Actor").add<SelfRemover>().add<Counter>();

    world.update(frameOf(0.1f));

    EXPECT_FALSE(actor.has<SelfRemover>());
    EXPECT_TRUE(actor.has<Counter>());
    EXPECT_EQ(actor.get<Counter>().updates, 1);
}

TEST(Routine, HasAndFindTellRoutinesFromComponents)
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

TEST(Orbit, StartsFromWhereTheCameraWasPlaced)
{
    const compages::core::Vector3f targets[] = { compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                 compages::core::Vector3f(0.0f, 8.0f, 0.0f),
                                 compages::core::Vector3f(1.0f, 2.0f, -3.0f) };
    const compages::core::Vector3f places[] = { compages::core::Vector3f(0.0f, 7.0f, 14.0f),
                                compages::core::Vector3f(0.0f, 25.0f, 90.0f),
                                compages::core::Vector3f(-6.0f, 1.0f, 4.0f) };
    for (compages::core::Vector3f const& target : targets)
    {
        for (compages::core::Vector3f const& place : places)
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

TEST(Fly, KeepsTheDirectionTheCameraWasLooking)
{
    compages::world::World world;
    compages::world::Entity camera = world.entity("Camera").position(0.0f, 3.0f, 10.0f);
    camera.lookAt(2.0f, 0.0f, 0.0f);
    const compages::core::Quatf before = camera.rotation();
    const compages::core::Vector3f forward_before = before * compages::core::Vector3f(0.0f, 0.0f, -1.0f);

    camera.add<compages::world::Fly>();
    world.update(frameOf(0.016f));

    const compages::core::Vector3f forward_after = camera.rotation() * compages::core::Vector3f(0.0f, 0.0f, -1.0f);
    EXPECT_LT(distanceBetween(forward_before, forward_after), 1.0e-3f);
    EXPECT_LT(distanceBetween(camera.position(), compages::core::Vector3f(0.0f, 3.0f, 10.0f)), 1.0e-3f);
}

// Regression: a headless update(compages::core::Frame) kept the keys and the mouse of the last
// update(compages::world::ViewFrame), so a routine saw them held forever.
struct InputProbe : compages::world::Routine
{
    bool left = false;
    bool forward = false;

    void update(float /*p_dt*/) override
    {
        left = input().mouse_left;
        forward = input().down(compages::world::Key::W);
    }
};

TEST(WorldInput, HeadlessUpdateDoesNotKeepTheInputOfTheLastViewFrame)
{
    compages::world::World world;
    compages::world::Entity probe = world.entity("Probe").add<InputProbe>();

    compages::world::ViewFrame view;
    view.elapsed = 0.016f;
    view.input.mouse_left = true;
    view.input.set(compages::world::Key::W, true);
    world.update(view);
    EXPECT_TRUE(probe.get<InputProbe>().left);
    EXPECT_TRUE(probe.get<InputProbe>().forward);

    world.update(frameOf(0.016f));
    EXPECT_FALSE(probe.get<InputProbe>().left);
    EXPECT_FALSE(probe.get<InputProbe>().forward);
    EXPECT_FALSE(world.input().mouse_left);
}

TEST(Routine, FixedUpdateKeepsItsPaceWhateverTheFrames)
{
    compages::world::World world;
    world.routines().fixedStep(0.1f);
    FixedCounter& counter = world.entity("e").add<FixedCounter>().get<FixedCounter>();

    world.update(frameOf(0.25f)); // 0.25 s: two steps, 0.05 s left
    EXPECT_EQ(counter.calls, 2);
    EXPECT_FLOAT_EQ(counter.last_dt, 0.1f);
    world.update(frameOf(0.06f)); // 0.11 s: one more
    EXPECT_EQ(counter.calls, 3);
    world.update(frameOf(0.01f)); // 0.02 s: none
    EXPECT_EQ(counter.calls, 3);
}

TEST(Routine, FixedUpdateDropsWhatASlowFrameCannotCatchUp)
{
    compages::world::World world;
    world.routines().fixedStep(0.01f);
    FixedCounter& counter = world.entity("e").add<FixedCounter>().get<FixedCounter>();
    world.update(frameOf(10.0f));
    EXPECT_EQ(counter.calls, 8);
    world.update(frameOf(0.0f)); // the ten seconds are not owed any more
    EXPECT_LE(counter.calls, 9);
}

TEST(Routine, LateUpdateSeesTheWorldMatricesOfThisStep)
{
    compages::world::World world;
    compages::world::Entity parent = world.entity("parent").add<Mover>(1.0f);
    Follower& follower = parent.child("child").add<Follower>().get<Follower>();
    world.update(frameOf(0.5f));
    EXPECT_FLOAT_EQ(follower.seen.x, 0.5f);
    world.update(frameOf(0.5f));
    EXPECT_FLOAT_EQ(follower.seen.x, 1.0f);
}

TEST(Routine, RunsByPriorityThenInTheOrderOfAddition)
{
    auto log = std::make_shared<std::vector<std::string>>();
    compages::world::World world;
    compages::world::Entity a = world.entity("a");
    compages::world::Entity b = world.entity("b");
    b.add<Logger>(log, "b1");
    a.add<Logger>(log, "a1");
    b.add<Logger>(log, "early", -1);
    a.add<Logger>(log, "late", 1);
    world.update(frameOf(0.1f));
    log->clear();
    world.update(frameOf(0.1f));
    const std::vector<std::string> expected{ "early.update", "b1.update", "a1.update", "late.update" };
    EXPECT_EQ(*log, expected);
}

TEST(Routine, FollowsItsLifeFromEnableToDestroy)
{
    auto log = std::make_shared<std::vector<std::string>>();
    compages::world::World world;
    compages::world::Entity e = world.entity("e").add<Logger>(log, "r");

    world.update(frameOf(0.1f));
    EXPECT_EQ(*log, (std::vector<std::string>{ "r.onEnable", "r.start", "r.update" }));

    log->clear();
    e.enable(false);
    world.update(frameOf(0.1f));
    e.enable(true);
    world.update(frameOf(0.1f));
    EXPECT_EQ(*log, (std::vector<std::string>{ "r.onDisable", "r.onEnable", "r.update" }));

    log->clear();
    e.get<Logger>().enabled(false);
    world.update(frameOf(0.1f));
    EXPECT_EQ(*log, (std::vector<std::string>{ "r.onDisable" }));

    log->clear();
    e.get<Logger>().enabled(true);
    world.update(frameOf(0.1f));
    e.destroy();
    EXPECT_EQ(*log, (std::vector<std::string>{ "r.onEnable", "r.update", "r.onDisable", "r.onDestroy" }));
}

TEST(Routine, IsDestroyedWhenRemovedOrWhenItsWorldGoes)
{
    auto log = std::make_shared<std::vector<std::string>>();
    {
        compages::world::World world;
        compages::world::Entity e = world.entity("e").add<Logger>(log, "kept");
        world.entity("f").add<Logger>(log, "gone");
        world.update(frameOf(0.1f));
        log->clear();
        world.lookup("f").remove<Logger>();
        EXPECT_EQ(*log, (std::vector<std::string>{ "gone.onDisable", "gone.onDestroy" }));
        log->clear();
        (void)e;
    }
    EXPECT_EQ(*log, (std::vector<std::string>{ "kept.onDisable", "kept.onDestroy" }));
}

TEST(Routine, OnDestroySeesItsEntityAndChildrenAlive)
{
    struct Checker : compages::world::Routine
    {
        explicit Checker(bool* p_ok) : ok(p_ok) {}
        void onDestroy() override
        {
            *ok = world().alive(entity().id()) && entity().lookup("child");
        }
        bool* ok;
    };
    bool ok = false;
    compages::world::World world;
    compages::world::Entity e = world.entity("e").add<Checker>(&ok);
    (void)e.child("child");
    e.destroy();
    EXPECT_TRUE(ok);
    EXPECT_EQ(world.living(), 0u);
}

TEST(Routine, StepWithoutMovingAnythingSkipsTheMatrices)
{
    compages::world::World world;
    (void)world.entity("e").position(1.0f, 0.0f, 0.0f);
    world.update();
    EXPECT_FALSE(world.transforms().anyDirty());
    world.update();
    EXPECT_FALSE(world.transforms().anyDirty());
}
