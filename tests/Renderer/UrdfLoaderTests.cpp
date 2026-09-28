// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "main.hpp"

#include "Compages/Renderer/Assets/UrdfLoader.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

using namespace units::literals;

namespace
{

//! \brief The kinematics of the ABB IRB 2400, without its meshes.
constexpr char const* IRB2400 = R"(<?xml version="1.0" ?>
<robot name="irb2400">
  <material name="abb_orange"><color rgba="1 0.43 0 1"/></material>
  <link name="base_link"/>
  <link name="link_1"/>
  <link name="link_2"/>
  <link name="link_3"/>
  <link name="link_4"/>
  <link name="link_5"/>
  <link name="link_6"/>
  <link name="tool0"/>
  <joint name="joint_1" type="revolute">
    <origin rpy="0 0 0" xyz="0 0 0"/>
    <parent link="base_link"/><child link="link_1"/>
    <axis xyz="0 0 1"/>
    <limit effort="0" lower="-3.1416" upper="3.1416" velocity="2.618"/>
  </joint>
  <joint name="joint_2" type="revolute">
    <origin rpy="0 0 0" xyz="0.1 0 0.615"/>
    <parent link="link_1"/><child link="link_2"/>
    <axis xyz="0 1 0"/>
    <limit effort="0" lower="-1.7453" upper="1.9199" velocity="2.618"/>
  </joint>
  <joint name="joint_3" type="revolute">
    <origin rpy="0 0 0" xyz="0 0 0.705"/>
    <parent link="link_2"/><child link="link_3"/>
    <axis xyz="0 1 0"/>
    <limit effort="0" lower="-1.0472" upper="1.1345" velocity="2.618"/>
  </joint>
  <joint name="joint_4" type="revolute">
    <origin rpy="0 0 0" xyz="0.258 0 0.135"/>
    <parent link="link_3"/><child link="link_4"/>
    <axis xyz="1 0 0"/>
    <limit effort="0" lower="-3.49" upper="3.49" velocity="6.2832"/>
  </joint>
  <joint name="joint_5" type="revolute">
    <origin rpy="0 0 0" xyz="0.497 0 0"/>
    <parent link="link_4"/><child link="link_5"/>
    <axis xyz="0 1 0"/>
    <limit effort="0" lower="-2.0944" upper="2.0944" velocity="6.2832"/>
  </joint>
  <joint name="joint_6" type="revolute">
    <origin rpy="0 0 0" xyz="0.085 0 0"/>
    <parent link="link_5"/><child link="link_6"/>
    <axis xyz="1 0 0"/>
    <limit effort="0" lower="-6.9813" upper="6.9813" velocity="7.854"/>
  </joint>
  <joint name="joint_6-tool0" type="fixed">
    <parent link="link_6"/><child link="tool0"/>
    <origin rpy="0 0 0" xyz="0 0 0"/>
  </joint>
</robot>
)";

constexpr char const* TOOL0 = "base_link/link_1/link_2/link_3/link_4/link_5/link_6/tool0";

// ****************************************************************************
//! \brief A URDF text written to a temporary file for the time of a test.
// ****************************************************************************
class UrdfFile
{
public:

    explicit UrdfFile(std::string const& p_text)
        : m_path(std::filesystem::temp_directory_path() /
                 ("compages_" + std::to_string(s_count++) + ".urdf"))
    {
        std::ofstream file(m_path);
        file << p_text;
    }

    ~UrdfFile()
    {
        std::filesystem::remove(m_path);
    }

    [[nodiscard]] std::string path() const
    {
        return m_path.string();
    }

private:

    static inline int s_count = 0;
    std::filesystem::path m_path;
};

void expectNear(Vector3f const& p_actual, Vector3f const& p_expected)
{
    EXPECT_NEAR(p_actual.x, p_expected.x, 1.0e-4f);
    EXPECT_NEAR(p_actual.y, p_expected.y, 1.0e-4f);
    EXPECT_NEAR(p_actual.z, p_expected.z, 1.0e-4f);
}

//! \brief URDF is Z-up and the World Y-up: (x, y, z) becomes (x, z, -y).
Vector3f yUp(float p_x, float p_y, float p_z)
{
    return Vector3f(p_x, p_z, -p_y);
}

} // namespace

TEST(UrdfLoader, BuildsTheChainOfLinks)
{
    const UrdfFile file(IRB2400);
    compages::world::World world;
    auto robot = compages::renderer::loadUrdf(world, file.path());
    ASSERT_TRUE(bool(robot)) << robot.error();

    EXPECT_EQ(robot.value().name(), "irb2400");
    EXPECT_EQ(world.living(), 9u);
    for (char const* path : { "base_link/link_1", "base_link/link_1/link_2/link_3/link_4/link_5/link_6" })
    {
        compages::world::Entity link = robot.value().lookup(path);
        ASSERT_TRUE(bool(link)) << path;
        EXPECT_TRUE(link.has<compages::world::RevoluteJoint>()) << path;
    }
    compages::world::Entity tool = robot.value().lookup(TOOL0);
    ASSERT_TRUE(bool(tool));
    EXPECT_FALSE(tool.has<compages::world::RevoluteJoint>());
    EXPECT_FALSE(robot.value().lookup("base_link").has<compages::world::RevoluteJoint>());
}

TEST(UrdfLoader, ReadsTheLimitsOfTheJoints)
{
    const UrdfFile file(IRB2400);
    compages::world::World world;
    auto robot = compages::renderer::loadUrdf(world, file.path());
    ASSERT_TRUE(bool(robot)) << robot.error();

    compages::world::Entity link_2 = robot.value().lookup("base_link/link_1/link_2");
    auto const& joint = link_2.get<compages::world::RevoluteJoint>();
    EXPECT_NEAR(joint.state.position.min.to<double>(), -1.7453, 1.0e-9);
    EXPECT_NEAR(joint.state.position.max.to<double>(), 1.9199, 1.0e-9);
    EXPECT_NEAR(joint.state.velocity.min.to<double>(), -2.618, 1.0e-9);
    EXPECT_NEAR(joint.state.velocity.max.to<double>(), 2.618, 1.0e-9);
    EXPECT_TRUE(std::isinf(joint.state.acceleration.max.to<double>()));
    expectNear(joint.axis, Vector3f(0, 1, 0));
    expectNear(joint.origin.position, Vector3f(0.1f, 0.0f, 0.615f));
}

TEST(UrdfLoader, ForwardKinematicsOfTheIrb2400)
{
    const UrdfFile file(IRB2400);
    compages::world::World world;
    auto robot = compages::renderer::loadUrdf(world, file.path());
    ASSERT_TRUE(bool(robot)) << robot.error();
    compages::world::Entity tool = robot.value().lookup(TOOL0);

    // At zero, the tool is the sum of the joint offsets.
    world.update();
    expectNear(tool.worldPosition(),
               yUp(0.1f + 0.258f + 0.497f + 0.085f, 0.0f, 0.615f + 0.705f + 0.135f));

    // Turning the base a quarter turn around z swings the arm onto y.
    robot.value().lookup("base_link/link_1").angle(90.0_deg);
    world.update();
    expectNear(tool.worldPosition(), yUp(0.0f, 0.94f, 1.455f));

    // Folding the shoulder forward a quarter turn around y lays the upper arm
    // along x: the elbow ends up 0.705 ahead of the shoulder.
    robot.value().lookup("base_link/link_1").angle(0.0_deg);
    robot.value().lookup("base_link/link_1/link_2").angle(90.0_deg);
    world.update();
    compages::world::Entity elbow = robot.value().lookup("base_link/link_1/link_2/link_3");
    expectNear(elbow.worldPosition(), yUp(0.1f + 0.705f, 0.0f, 0.615f));
}

TEST(UrdfLoader, ReadsRollPitchYawAndOtherJointTypes)
{
    const UrdfFile file(R"(<robot name="bot">
  <link name="base"/><link name="arm"/><link name="tip"/><link name="wheel"/>
  <joint name="j1" type="fixed">
    <origin xyz="1 0 0" rpy="0 0 1.5707963"/>
    <parent link="base"/><child link="arm"/>
  </joint>
  <joint name="j2" type="prismatic">
    <origin xyz="1 0 0"/>
    <parent link="arm"/><child link="tip"/>
    <axis xyz="0 0 1"/>
    <limit lower="0" upper="0.2" velocity="0.5"/>
  </joint>
  <joint name="j3" type="continuous">
    <parent link="base"/><child link="wheel"/>
    <axis xyz="0 1 0"/>
  </joint>
</robot>)");
    compages::world::World world;
    auto robot = compages::renderer::loadUrdf(world, file.path());
    ASSERT_TRUE(bool(robot)) << robot.error();

    compages::world::Entity tip = robot.value().lookup("base/arm/tip");
    ASSERT_TRUE(tip.has<compages::world::PrismaticJoint>());
    world.update();
    // The yaw turns the x offset of the tip onto y.
    expectNear(tip.worldPosition(), yUp(1.0f, 1.0f, 0.0f));
    tip.offset(1.0_m);
    world.update();
    expectNear(tip.worldPosition(), yUp(1.0f, 1.0f, 0.2f));

    compages::world::Entity wheel = robot.value().lookup("base/wheel");
    ASSERT_TRUE(wheel.has<compages::world::RevoluteJoint>());
    EXPECT_TRUE(std::isinf(wheel.get<compages::world::RevoluteJoint>().state.position.max.to<double>()));
}

TEST(UrdfLoader, HangsUnderAParent)
{
    const UrdfFile file(IRB2400);
    compages::world::World world;
    compages::world::Entity cell = world.entity("Cell").position(5, 0, 0);
    auto robot = compages::renderer::loadUrdf(world, file.path(), cell);
    ASSERT_TRUE(bool(robot)) << robot.error();
    EXPECT_EQ(robot.value().parent(), cell);
    world.update();
    expectNear(robot.value().lookup(TOOL0).worldPosition(),
               Vector3f(5.0f, 0.0f, 0.0f) + yUp(0.94f, 0.0f, 1.455f));
}

TEST(UrdfLoader, RefusesBrokenFilesWithoutLeavingEntities)
{
    compages::world::World world;

    auto missing = compages::renderer::loadUrdf(world, "/does/not/exist.urdf");
    EXPECT_FALSE(missing);

    const UrdfFile floating(R"(<robot name="r"><link name="a"/><link name="b"/>
  <joint name="j" type="floating"><parent link="a"/><child link="b"/></joint>
</robot>)");
    auto unsupported = compages::renderer::loadUrdf(world, floating.path());
    ASSERT_FALSE(unsupported);
    EXPECT_NE(unsupported.error().find("floating"), std::string::npos);

    const UrdfFile two_roots(R"(<robot name="r"><link name="a"/><link name="b"/></robot>)");
    EXPECT_FALSE(compages::renderer::loadUrdf(world, two_roots.path()));

    const UrdfFile unknown(R"(<robot name="r"><link name="a"/>
  <joint name="j" type="fixed"><parent link="a"/><child link="ghost"/></joint>
</robot>)");
    EXPECT_FALSE(compages::renderer::loadUrdf(world, unknown.path()));

    const UrdfFile not_robot(R"(<scene/>)");
    EXPECT_FALSE(compages::renderer::loadUrdf(world, not_robot.path()));

    EXPECT_EQ(world.living(), 0u);
}
