// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Entity.hpp"
#include "GPUContext.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/GltfLoader.hpp"
#include "Compages/Core/File.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/Renderer/Systems/AnimationSystem.hpp"
#include "Compages/Renderer/Components/Animator.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/World.hpp"

#include <cmath>

using namespace tests;

namespace
{

std::string soldierPath()
{
    for (const char* root :
         { "external/Compages-data/",
           "../external/Compages-data/",
           "external/Compages-data/",
           "../external/Compages-data/" })
    {
        const std::string path = std::string(root) + "Soldier.glb";
        if (File::exist(path))
        {
            return path;
        }
    }
    if (File::exist("/home/qq/three.js/examples/models/gltf/Soldier.glb"))
    {
        return "/home/qq/three.js/examples/models/gltf/Soldier.glb";
    }
    return {};
}

} // namespace

class GltfAnimationTest: public GPUTest
{
protected:

    void SetUp() override
    {
        GPUTest::SetUp();
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
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto loaded = assets.load(path);
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
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto imported = scene.load(path);
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
        for (Matrix44f const& joint : p_skin.pose)
        {
            const Vector3f t(joint[3].x, joint[3].y, joint[3].z);
            worst = std::max(worst, compages::vector::norm(t));
        }
    });
    EXPECT_LT(worst, 8.0f) << "bind-pose joint matrices translated too far";
}

TEST_F(GltfAnimationTest, WalkMovesTheMesh)
{
    const std::string path = soldierPath();
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    auto imported = scene.load(path);
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
    std::vector<Matrix44f> bind;
    auto const skins = world.view<compages::world::SkinInstance>();
    ASSERT_FALSE(skins.empty());
    auto const first_skin = *skins.begin();
    bind = skins.get<compages::world::SkinInstance>(first_skin).pose;

    ASSERT_TRUE(bool(compages::renderer::AnimationSystem::tick(world, assets, 0.35f)));

    float worst = 0.0f;
    auto const& after = skins.get<compages::world::SkinInstance>(first_skin).pose;
    ASSERT_EQ(after.size(), bind.size());
    for (std::size_t j = 0u; j < bind.size(); ++j)
    {
        const Vector3f d(
            after[j][3].x - bind[j][3].x,
            after[j][3].y - bind[j][3].y,
            after[j][3].z - bind[j][3].z);
        worst = std::max(worst, compages::vector::norm(d));
    }
    EXPECT_GT(worst, 0.05f) << "Walk clip left the bind pose unchanged";
}
