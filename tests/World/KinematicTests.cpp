// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "main.hpp"


#include "Compages/World/Components/PoseDriver.hpp"
#include "Compages/World/World.hpp"

#include <cmath>
#include <limits>



using namespace units::literals;

namespace
{

constexpr float EPSILON = 1.0e-5f;

void expectNear(compages::core::Vector3f const& p_actual, compages::core::Vector3f const& p_expected)
{
    EXPECT_NEAR(p_actual.x, p_expected.x, EPSILON);
    EXPECT_NEAR(p_actual.y, p_expected.y, EPSILON);
    EXPECT_NEAR(p_actual.z, p_expected.z, EPSILON);
}

} // namespace

TEST(Kinematics, BoundedClampsItsValue)
{
    compages::world::Bounded<units::angle::radian_t> bounded;
    bounded.value = 3.0_rad;
    EXPECT_DOUBLE_EQ(bounded.applied().to<double>(), 3.0);
    bounded.min = -1.0_rad;
    bounded.max = 1.0_rad;
    EXPECT_DOUBLE_EQ(bounded.applied().to<double>(), 1.0);
    bounded.value = -2.0_rad;
    EXPECT_DOUBLE_EQ(bounded.applied().to<double>(), -1.0);
}

TEST(Kinematics, JointStateIsUnboundedByDefault)
{
    const compages::world::RevoluteJoint joint;
    EXPECT_TRUE(std::isinf(joint.state.position.min.to<double>()));
    EXPECT_TRUE(std::isinf(joint.state.velocity.max.to<double>()));
    EXPECT_TRUE(std::isinf(joint.state.acceleration.max.to<double>()));
    EXPECT_DOUBLE_EQ(joint.state.position.value.to<double>(), 0.0);
}

TEST(Kinematics, RevoluteJointTurnsItsLink)
{
    compages::world::World world;
    compages::world::Entity base = world.entity("Base");
    compages::world::Entity arm = base.child("Arm").position(0, 0, 1).revolute({ 0, 0, 1 });
    compages::world::Entity tip = arm.child("Tip").position(1, 0, 0);

    world.update();
    expectNear(tip.worldPosition(), compages::core::Vector3f(1, 0, 1));

    arm.joint().angle(90.0_deg);
    world.update();
    expectNear(tip.worldPosition(), compages::core::Vector3f(0, 1, 1));
    EXPECT_NEAR(arm.joint().angle().to<double>(), M_PI / 2.0, 1.0e-9);
}

TEST(Kinematics, RevoluteJointStaysWithinItsLimits)
{
    compages::world::World world;
    compages::world::Entity arm = world.entity("Arm").revolute({ 0, 0, 1 }, -45.0_deg, 45.0_deg);
    compages::world::Entity tip = arm.child("Tip").position(1, 0, 0);

    arm.joint().angle(90.0_deg);
    world.update();
    const float half = std::sqrt(0.5f);
    expectNear(tip.worldPosition(), compages::core::Vector3f(half, half, 0));
    // The command is kept; the applied angle is the one inside the bounds.
    EXPECT_NEAR(units::angle::degree_t(arm.joint().commandedAngle()).to<double>(), 90.0, 1.0e-9);
    EXPECT_NEAR(units::angle::degree_t(arm.joint().angle()).to<double>(), 45.0, 1.0e-9);
}

TEST(Kinematics, PrismaticJointSlidesItsLink)
{
    compages::world::World world;
    compages::world::Entity slider =
        world.entity("Slider").position(1, 0, 0).prismatic({ 0, 0, 2 }, 0.0_m, 0.5_m);

    slider.joint().offset(0.3_m);
    world.update();
    expectNear(slider.worldPosition(), compages::core::Vector3f(1, 0, 0.3f));

    slider.joint().offset(2.0_m);
    world.update();
    expectNear(slider.worldPosition(), compages::core::Vector3f(1, 0, 0.5f));
}

TEST(Kinematics, PrismaticAxisFollowsTheJointOrientation)
{
    compages::world::World world;
    compages::world::Entity slider = world.entity("Slider")
                               .rotation(0.5f * float(M_PI), compages::core::Vector3f(0, 0, 1))
                               .prismatic({ 1, 0, 0 });
    slider.joint().offset(2.0_m);
    world.update();
    expectNear(slider.worldPosition(), compages::core::Vector3f(0, 2, 0));
}

TEST(Kinematics, PositionSetsTheJointOriginWhateverTheOrder)
{
    compages::world::World world;
    compages::world::Entity before = world.entity("Before").position(0, 2, 0).revolute({ 0, 0, 1 });
    compages::world::Entity after = world.entity("After").revolute({ 0, 0, 1 }).position(0, 2, 0);

    ASSERT_TRUE(before.has<compages::world::RevoluteJoint>());
    ASSERT_TRUE(after.has<compages::world::RevoluteJoint>());
    expectNear(before.get<compages::world::RevoluteJoint>().origin.position, compages::core::Vector3f(0, 2, 0));
    expectNear(after.get<compages::world::RevoluteJoint>().origin.position, compages::core::Vector3f(0, 2, 0));
    expectNear(after.position(), compages::core::Vector3f(0, 2, 0));

    before.joint().angle(30.0_deg);
    after.joint().angle(30.0_deg);
    world.update();
    expectNear(before.worldPosition(), compages::core::Vector3f(0, 2, 0));
    expectNear(after.worldPosition(), compages::core::Vector3f(0, 2, 0));
}

TEST(Kinematics, ReplacingAJointKeepsItsOrigin)
{
    compages::world::World world;
    compages::world::Entity link = world.entity("Link").revolute({ 0, 0, 1 }).position(3, 0, 0);
    link.prismatic({ 0, 1, 0 });
    EXPECT_FALSE(link.has<compages::world::RevoluteJoint>());
    ASSERT_TRUE(link.has<compages::world::PrismaticJoint>());
    expectNear(link.get<compages::world::PrismaticJoint>().origin.position, compages::core::Vector3f(3, 0, 0));
}

TEST(Kinematics, TwoLinkChainForwardKinematics)
{
    // Planar arm: two unit links turning around z.
    compages::world::World world;
    compages::world::Entity shoulder = world.entity("Shoulder").revolute({ 0, 0, 1 });
    compages::world::Entity elbow = shoulder.child("Elbow").position(1, 0, 0).revolute({ 0, 0, 1 });
    compages::world::Entity hand = elbow.child("Hand").position(1, 0, 0);

    shoulder.joint().angle(90.0_deg);
    elbow.joint().angle(90.0_deg);
    world.update();
    expectNear(elbow.worldPosition(), compages::core::Vector3f(0, 1, 0));
    expectNear(hand.worldPosition(), compages::core::Vector3f(-1, 1, 0));

    // Any angles: the classic x = cos(a) + cos(a + b), y = sin(a) + sin(a + b).
    const double a = 0.3;
    const double b = -1.1;
    shoulder.joint().angle(units::angle::radian_t(a));
    elbow.joint().angle(units::angle::radian_t(b));
    world.update();
    expectNear(hand.worldPosition(),
               compages::core::Vector3f(float(std::cos(a) + std::cos(a + b)),
                        float(std::sin(a) + std::sin(a + b)), 0.0f));
}

TEST(Kinematics, UnchangedJointsLeaveTheirTransformsClean)
{
    entt::registry registry;
    compages::world::TransformStore transforms;
    const compages::world::EntityId arm(registry.create());
    transforms.allocate(arm);
    registry.emplace<compages::world::RevoluteJoint>(arm.native()).state.position.value = 10.0_deg;

    const compages::world::KinematicSystem system;
    system.update(registry, transforms);
    ASSERT_TRUE(transforms.isDirty(arm));
    transforms.markClean(arm);

    // Same angle: nothing to write, so the subtree is not recomputed.
    system.update(registry, transforms);
    EXPECT_FALSE(transforms.isDirty(arm));

    registry.get<compages::world::RevoluteJoint>(arm.native()).state.position.value = 20.0_deg;
    system.update(registry, transforms);
    EXPECT_TRUE(transforms.isDirty(arm));
}

TEST(Kinematics, ZeroAxisJointDoesNotMoveItsLink)
{
    compages::world::World world;
    compages::world::Entity arm = world.entity("Arm").position(1, 2, 3).revolute({ 0, 0, 0 });
    compages::world::Entity slider = world.entity("Slider").position(1, 0, 0).prismatic({ 0, 0, 0 });

    arm.joint().angle(90.0_deg);
    slider.joint().offset(0.5_m);
    world.update();
    expectNear(arm.worldPosition(), compages::core::Vector3f(1, 2, 3));
    expectNear(slider.worldPosition(), compages::core::Vector3f(1, 0, 0));
    EXPECT_TRUE(std::isfinite(arm.worldMatrix()(0, 0)));
}

// A physics engine marks its bodies External: the joint no longer writes the
// pose, so the one written from outside stays.
TEST(Kinematics, ExternalPoseDriverKeepsThePoseWrittenFromOutside)
{
    compages::world::World world;
    compages::world::Entity arm = world.entity("Arm").revolute({ 0, 0, 1 });
    compages::world::Entity tip = arm.child("Tip").position(1, 0, 0);
    arm.set(compages::world::PoseDriver{ compages::world::PoseDriver::External });

    arm.joint().angle(90.0_deg);
    arm.transform().position = compages::core::Vector3f(0, 0, 5);
    world.transforms().markDirty(arm.id());
    world.update();
    expectNear(arm.worldPosition(), compages::core::Vector3f(0, 0, 5));
    expectNear(tip.worldPosition(), compages::core::Vector3f(1, 0, 5));

    // Back to the joint: it writes the pose again.
    arm.set(compages::world::PoseDriver{ compages::world::PoseDriver::Kinematic });
    world.update();
    expectNear(tip.worldPosition(), compages::core::Vector3f(0, 1, 0));
}
