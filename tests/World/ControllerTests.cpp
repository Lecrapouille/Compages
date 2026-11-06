// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include <cmath>


#include "Compages/World/Controllers/FPSController.hpp"
#include "Compages/World/Controllers/FlyController.hpp"
#include "Compages/World/Controllers/OrbitController.hpp"
#include "Compages/World/Controllers/RigController.hpp"
#include "Compages/World/Controllers/ThirdPersonController.hpp"
#include "Compages/World/World.hpp"



TEST(OrbitController, PlacesTheCameraOppositeTheForwardAxis)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    compages::world::OrbitController orbit;
    orbit.target = compages::Vector3f(0.0f, 0.0f, 0.0f);
    orbit.yaw = 0.0f;
    orbit.pitch = 0.0f;
    orbit.distance = 10.0f;
    orbit.writePose(world, camera);

    // Yaw 0, pitch 0: looking along -Z, so the camera sits at +Z.
    ASSERT_NEAR(world.transform(camera).position.x, 0.0f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.y, 0.0f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.z, 10.0f, 1.0e-4f);
}

TEST(OrbitController, ZoomPullsTheCameraIn)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    compages::world::OrbitController orbit;
    orbit.distance = 20.0f;
    orbit.pitch = 0.0f;
    compages::world::CameraInput input;
    input.zoom_delta = 2.0f;
    orbit.apply(world, camera, input, 10.0f);
    // Two notches take the same part of the way, near or far.
    const float asked = 20.0f * std::exp(-2.0f * orbit.zoom_step);
    ASSERT_NEAR(orbit.distance, asked, 1.0e-4f);
    // A long frame has reached it; the camera is there.
    ASSERT_NEAR(world.transform(camera).position.z, asked, 1.0e-3f);
}

TEST(OrbitController, ANotchOfTheWheelGlidesInsteadOfJumping)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    compages::world::OrbitController orbit;
    orbit.distance = 3.0f;
    orbit.pitch = 0.0f;
    compages::world::CameraInput notch;
    notch.zoom_delta = 1.0f;
    orbit.apply(world, camera, notch, 1.0f / 60.0f);

    // After one frame, the camera has started to move but is far from the
    // end: no jump. Then it keeps gliding without any more input.
    const float asked = orbit.distance;
    const float first = world.transform(camera).position.z;
    EXPECT_LT(first, 3.0f);
    EXPECT_GT(first - asked, 0.5f * (3.0f - asked));
    for (int i = 0; i < 120; ++i)
    {
        orbit.apply(world, camera, compages::world::CameraInput{}, 1.0f / 60.0f);
    }
    EXPECT_NEAR(world.transform(camera).position.z, asked, 1.0e-3f);
}

TEST(FlyController, ForwardMovesAlongLook)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    world.transform(camera).position = compages::Vector3f(0.0f, 0.0f, 0.0f);
    compages::world::FlyController fly;
    fly.yaw = 0.0f;
    fly.pitch = 0.0f;
    fly.move_speed = 10.0f;
    compages::world::CameraInput input;
    input.forward = true;
    fly.apply(world, camera, input, 1.0f);
    // Looking -Z, one second at 10 units/s.
    ASSERT_NEAR(world.transform(camera).position.z, -10.0f, 1.0e-3f);
}

TEST(FPSController, WalksOnTheGroundWithoutClimbing)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    world.transform(camera).position = compages::Vector3f(0.0f, 1.7f, 0.0f);
    compages::world::FPSController fps;
    fps.yaw = 0.0f;
    fps.pitch = -0.4f;
    fps.eye_height = 1.7f;
    fps.move_speed = 5.0f;
    compages::world::CameraInput input;
    input.forward = true;
    fps.apply(world, camera, input, 1.0f);
    ASSERT_NEAR(world.transform(camera).position.y, 1.7f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.z, -5.0f, 1.0e-3f);
}

TEST(ThirdPersonController, FollowsTheTargetEntity)
{
    compages::world::World world;
    const compages::world::EntityId target = world.create("hero");
    const compages::world::EntityId camera = world.create("cam");
    world.transform(target).position = compages::Vector3f(10.0f, 0.0f, 0.0f);
    compages::world::ThirdPersonController follow;
    follow.target = target;
    follow.look_offset = compages::Vector3f(0.0f, 0.0f, 0.0f);
    follow.yaw = 0.0f;
    follow.pitch = 0.0f;
    follow.distance = 6.0f;
    follow.writePose(world, camera);

    ASSERT_NEAR(world.transform(camera).position.x, 10.0f, 1.0e-4f);
    ASSERT_NEAR(world.transform(camera).position.z, 6.0f, 1.0e-4f);
}

TEST(RigController, GlidesTheSameWayAtAnyFrameRate)
{
    // One second at 10 Hz or at 1000 Hz ends at the same place, and a long
    // frame does not overshoot (the bug of Lerp(a, b, dt * speed)).
    auto run = [](int p_steps, float p_dt)
    {
        compages::world::World world;
        const compages::world::EntityId camera = world.create("cam");
        compages::world::RigController rig;
        rig.snap(world, camera);
        rig.target = compages::Vector3f(1.0f, 0.0f, 0.0f);
        for (int i = 0; i < p_steps; ++i)
        {
            rig.apply(world, camera, compages::world::RigInput{}, p_dt);
        }
        return rig.currentTarget().x;
    };
    const float slow = run(10, 0.1f);
    const float fast = run(1000, 0.001f);
    ASSERT_NEAR(slow, fast, 1.0e-3f);
    ASSERT_LT(slow, 1.0f);
    ASSERT_LE(run(1, 10.0f), 1.0f);
}

TEST(RigController, PanKeepsTheGrabbedGroundUnderTheMouse)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    compages::world::RigController rig;
    rig.pitch = -0.8f;
    rig.distance = 4.0f;
    rig.snap(world, camera);

    compages::world::RigInput input;
    input.pan = true;
    input.mouse = compages::Vector2f(0.0f, 0.0f);
    rig.apply(world, camera, input, 0.0f);   // grab the target point
    input.mouse = compages::Vector2f(0.5f, 0.0f);
    rig.apply(world, camera, input, 0.0f);
    const compages::Vector3f moved = rig.target;
    // Dragging the mouse to the right drags the ground to the right: the
    // target goes to the left of the camera (-X at yaw 0).
    ASSERT_LT(moved.x, -0.1f);
    ASSERT_NEAR(moved.y, 0.0f, 1.0e-5f);
    // Holding the mouse still does not drift, while the camera catches up.
    for (int i = 0; i < 30; ++i)
    {
        rig.apply(world, camera, input, 1.0f / 60.0f);
    }
    ASSERT_NEAR(rig.target.x, moved.x, 1.0e-4f);
    ASSERT_NEAR(rig.target.z, moved.z, 1.0e-4f);
}

TEST(RigController, WheelZoomsByAFactor)
{
    compages::world::World world;
    const compages::world::EntityId camera = world.create("cam");
    compages::world::RigController rig;
    rig.distance = 10.0f;
    rig.snap(world, camera);
    compages::world::RigInput input;
    input.zoom_delta = 1.0f;
    rig.apply(world, camera, input, 0.0f);
    ASSERT_NEAR(rig.distance, 10.0f * std::exp(-rig.zoom_step), 1.0e-4f);
}
