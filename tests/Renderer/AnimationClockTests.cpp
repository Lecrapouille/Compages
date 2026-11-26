// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/Renderer/Animation/AnimationSystem.hpp"
#include "Compages/Renderer/Animation/Animator.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/World/Entity.hpp"
#include "Compages/World/World.hpp"

using namespace compages::si::literals;

namespace
{

//! \brief Counts the steps it is given: it must see none from the clips.
struct Steps : compages::world::Routine
{
    int count = 0;
    void update(compages::si::Seconds) override
    {
        ++count;
    }
};

//! \brief A clip of two seconds that moves its node from x = 0 to x = 2.
compages::renderer::AnimationClip slide()
{
    compages::renderer::AnimationChannel channel;
    channel.target = 0u;
    channel.path = compages::renderer::AnimationChannel::Path::Translation;
    channel.times = { 0.0f, 2.0f };
    channel.values = { 0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f };
    compages::renderer::AnimationClip clip;
    clip.name = "Slide";
    clip.duration = 2.0_s;
    clip.channels.push_back(channel);
    return clip;
}

} // namespace

// A host with its own clock (RobotIK and its history slider) sets the time of
// the clip, then poses: the same time gives the same pose, back or forth, and
// no routine runs.
TEST(AnimationClock, PosesAtTheTimeTheHostGivesWithoutRunningRoutines)
{
    compages::world::World world;
    compages::renderer::AssetManager assets;
    auto clip = assets.addAnimation("Slide", slide());
    ASSERT_TRUE(bool(clip)) << clip.error();

    compages::world::Entity node = world.entity("Node").add<Steps>();
    world.entity("Model").set(compages::renderer::Animator{
        .clip = clip.value(), .clips = {}, .targets = { node.id() }, .loop = false });
    auto& animator = world.get<compages::renderer::Animator>(world.lookup("Model").id());

    animator.time = 1.5_s;
    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.0_s)));
    ASSERT_NEAR(node.worldPosition().x, 1.5f, 1.0e-5f);

    animator.time = 0.5_s;
    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.0_s)));
    ASSERT_NEAR(node.worldPosition().x, 0.5f, 1.0e-5f);

    ASSERT_EQ(node.find<Steps>()->count, 0);
}

namespace
{

//! \brief A still clip that holds its node at x = p_x.
compages::renderer::AnimationClip hold(char const* p_name, float p_x)
{
    compages::renderer::AnimationChannel channel;
    channel.target = 0u;
    channel.path = compages::renderer::AnimationChannel::Path::Translation;
    channel.times = { 0.0f, 1.0f };
    channel.values = { p_x, 0.0f, 0.0f, p_x, 0.0f, 0.0f };
    compages::renderer::AnimationClip clip;
    clip.name = p_name;
    clip.duration = 1.0_s;
    clip.channels.push_back(channel);
    return clip;
}

} // namespace

// A walker that stops goes from Walk to Idle: during the fade the pose is a
// mix of both, then the new clip alone.
TEST(AnimationClock, FadesFromTheOldClipToTheNewOne)
{
    compages::world::World world;
    compages::renderer::AssetManager assets;
    auto left = assets.addAnimation("Left", hold("Left", 0.0f));
    auto right = assets.addAnimation("Right", hold("Right", 4.0f));
    ASSERT_TRUE(bool(left) && bool(right));

    compages::world::Entity node = world.entity("Node");
    const compages::world::EntityId model = world.entity("Model").id();
    world.entity(model).set(compages::renderer::Animator{
        .clip = left.value(), .clips = { left.value(), right.value() }, .targets = { node.id() } });
    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.1_s)));
    ASSERT_NEAR(node.worldPosition().x, 0.0f, 1.0e-5f);

    ASSERT_TRUE(compages::renderer::AnimationSystem::play(world, assets, model, "Right", 0.4_s));
    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.2_s)));
    EXPECT_NEAR(node.worldPosition().x, 2.0f, 1.0e-4f) << "half way through the fade";

    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.3_s)));
    EXPECT_NEAR(node.worldPosition().x, 4.0f, 1.0e-5f) << "after the fade";
    EXPECT_FALSE(world.get<compages::renderer::Animator>(model).previous.valid());
}

// Without a fade, the new clip shows at once.
TEST(AnimationClock, JumpsToTheNewClipWithoutFade)
{
    compages::world::World world;
    compages::renderer::AssetManager assets;
    auto left = assets.addAnimation("Left", hold("Left", 0.0f));
    auto right = assets.addAnimation("Right", hold("Right", 4.0f));
    ASSERT_TRUE(bool(left) && bool(right));

    compages::world::Entity node = world.entity("Node");
    const compages::world::EntityId model = world.entity("Model").id();
    world.entity(model).set(compages::renderer::Animator{
        .clip = left.value(), .clips = { left.value(), right.value() }, .targets = { node.id() } });
    ASSERT_TRUE(compages::renderer::AnimationSystem::play(world, assets, model, "Right"));
    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.1_s)));
    EXPECT_NEAR(node.worldPosition().x, 4.0f, 1.0e-5f);
}
