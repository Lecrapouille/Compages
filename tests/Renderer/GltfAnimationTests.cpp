// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "DataPath.hpp"
#include "GPUContext.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Import/GltfLoader.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/Renderer/Animation/AnimationSystem.hpp"
#include "Compages/Renderer/Animation/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/World.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>




using namespace tests;

class GltfAnimationTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
        if (IsSkipped())
        {
            return;
        }
        auto ready = compages::gpu::init(GPUContext::procAddress());
        ASSERT_TRUE(bool(ready)) << ready.error();
    }

    void TearDown() override
    {
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }
};

TEST_F(GltfAnimationTest, LoadsSoldierClipsAndSkins)
{
    const std::filesystem::path path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto loaded = assets.loadFile(path);
    ASSERT_TRUE(bool(loaded)) << loaded.error();
    compages::renderer::Prefab const* prefab = assets.prefab(loaded.value());
    ASSERT_NE(prefab, nullptr);
    EXPECT_GE(prefab->animations.size(), 3u);
    auto imported = scene.instantiate(loaded.value());
    ASSERT_TRUE(bool(imported)) << imported.error();
    EXPECT_TRUE(world.has<compages::renderer::Animator>(imported.value().id()));

    bool found_skin = false;
    auto const skins = world.view<compages::world::SkinInstance>();
    EXPECT_GE(skins.size(), 1u);
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId, compages::renderer::MeshRenderer const& p_renderer) {
        compages::renderer::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
        ASSERT_NE(mesh, nullptr);
        if (!mesh->rest_pose.empty())
        {
            found_skin = true;
            EXPECT_EQ(mesh->joint_indices.size(), mesh->rest_pose.size() * 4u);
        }
    });
    EXPECT_TRUE(found_skin);
}

TEST_F(GltfAnimationTest, BindPoseKeepsRestVertices)
{
    const std::filesystem::path path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto imported = scene.loadFile(path);
    ASSERT_TRUE(bool(imported)) << imported.error();

    if (world.has<compages::renderer::Animator>(imported.value().id()))
    {
        world.get<compages::renderer::Animator>(imported.value().id()).playing = false;
    }
    world.update();
    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::pose(world, assets)));

    float worst = 0.0f;
    world.each<compages::world::SkinInstance>(
        [&](compages::world::EntityId, compages::world::SkinInstance const& p_skin) {
        for (compages::Matrix44f const& joint : p_skin.pose)
        {
            const compages::Vector3f t = compages::translation(joint);
            worst = std::max(worst, compages::vector::norm(t));
        }
    });
    EXPECT_LT(worst, 8.0f) << "bind-pose joint matrices translated too far";
}

TEST_F(GltfAnimationTest, WalkMovesTheMesh)
{
    const std::filesystem::path path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto imported = scene.loadFile(path);
    ASSERT_TRUE(bool(imported)) << imported.error();

    compages::renderer::MeshAsset const* body = nullptr;
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId, compages::renderer::MeshRenderer const& p_renderer) {
        if (body != nullptr)
        {
            return;
        }
        compages::renderer::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
        if ((mesh != nullptr) && (mesh->rest_pose.size() > 100u))
        {
            body = mesh;
        }
    });
    ASSERT_NE(body, nullptr);

    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::pose(world, assets)));
    std::vector<compages::Matrix44f> bind;
    auto const skins = world.view<compages::world::SkinInstance>();
    ASSERT_FALSE(skins.empty());
    auto const first_skin = *skins.begin();
    bind = skins.get<compages::world::SkinInstance>(first_skin).pose;

    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, compages::si::Seconds(0.35))));

    float worst = 0.0f;
    auto const& after = skins.get<compages::world::SkinInstance>(first_skin).pose;
    ASSERT_EQ(after.size(), bind.size());
    for (std::size_t j = 0u; j < bind.size(); ++j)
    {
        const compages::Vector3f d = compages::translation(after[j]) -
                                           compages::translation(bind[j]);
        worst = std::max(worst, compages::vector::norm(d));
    }
    EXPECT_GT(worst, 0.05f) << "Walk clip left the bind pose unchanged";
}

// The feet of a walker do not slide when it moves at the speed its clip
// steps: Run steps faster than Walk, and Idle does not step.
TEST_F(GltfAnimationTest, MeasuresTheGroundSpeedOfAClipMadeInPlace)
{
    const std::filesystem::path path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }
    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto soldier = scene.loadFile(path);
    ASSERT_TRUE(bool(soldier)) << soldier.error();
    ASSERT_TRUE(scene.play(soldier.value(), "Idle"));

    const double walk = scene.groundSpeed(soldier.value(), "Walk").value();
    const double run = scene.groundSpeed(soldier.value(), "Run").value();
    const double idle = scene.groundSpeed(soldier.value(), "Idle").value();
    // A man walks at about 1.4 m/s and runs at 3 to 5 m/s.
    EXPECT_GT(walk, 0.8);
    EXPECT_LT(walk, 2.2);
    EXPECT_GT(run, walk * 1.5);
    EXPECT_LT(idle, 0.4);
    std::cout << "ground speed: walk " << walk << " run " << run << " idle " << idle << " m/s\n";
    // The model plays what it played before.
    EXPECT_EQ(scene.playing(soldier.value()), "Idle");
}

// Two files exported by the same tool share the names of their materials
// and skins ("Human.body", "Human.rig"): each file keeps its own, or the
// last one loaded would dress and bend the others.
TEST_F(GltfAnimationTest, TwoFilesKeepTheirOwnMaterialsAndSkins)
{
    const std::filesystem::path path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }
    const std::filesystem::path copy =
        std::filesystem::temp_directory_path() / "compages_second_soldier.glb";
    std::filesystem::copy_file(path, copy, std::filesystem::copy_options::overwrite_existing);

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto first = scene.loadFile(path);
    auto second = scene.loadFile(copy);
    ASSERT_TRUE(bool(first) && bool(second));

    std::vector<std::uint32_t> materials;
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId, compages::renderer::MeshRenderer const& p_renderer)
        { materials.push_back(p_renderer.material_instance.index()); });
    std::ranges::sort(materials);
    const auto repeated = std::ranges::unique(materials);
    EXPECT_TRUE(repeated.empty()) << "a material instance is shared by the two files";

    std::vector<compages::renderer::MeshAsset const*> skinned;
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId, compages::renderer::MeshRenderer const& p_renderer)
        {
            compages::renderer::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
            if ((mesh != nullptr) && mesh->skin.valid())
            {
                skinned.push_back(mesh);
            }
        });
    ASSERT_GE(skinned.size(), 2u);
    EXPECT_NE(skinned.front()->skin.index(), skinned.back()->skin.index());
    std::filesystem::remove(copy);
}
