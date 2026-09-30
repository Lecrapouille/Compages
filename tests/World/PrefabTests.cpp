// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"
#include "Compages/Renderer/Assets/Prefabs.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Components/PrefabInstance.hpp"
#include "Compages/Renderer/Prefab/PrefabInstantiate.hpp"
#include "Compages/World/World.hpp"




using namespace tests;

class PrefabTest: public GPUTest
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

TEST_F(PrefabTest, InstantiatesTheRobotHierarchy)
{
    compages::renderer::AssetManager assets;
    auto cube = compages::renderer::makeCube();
    ASSERT_TRUE(bool(cube));
    ASSERT_TRUE(bool(assets.addMesh("box", cube.take())));

    auto lit = compages::renderer::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "wood",
        compages::renderer::MaterialInstance{ material.value(), compages::core::Vector3f(0.6f, 0.4f, 0.2f) })));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "dark",
        compages::renderer::MaterialInstance{ material.value(), compages::core::Vector3f(0.3f, 0.2f, 0.1f) })));
    ASSERT_TRUE(bool(assets.addMaterialInstance(
        "light",
        compages::renderer::MaterialInstance{ material.value(), compages::core::Vector3f(0.9f, 0.9f, 0.8f) })));

    auto prefab_id = assets.addPrefab("robot", compages::renderer::makeRobotPrefab());
    ASSERT_TRUE(bool(prefab_id));

    compages::world::World world;
    const std::size_t before = world.living();
    auto root = compages::renderer::instantiate(world, assets, prefab_id.value());
    ASSERT_TRUE(bool(root));
    const compages::world::EntityId robot = root.take();
    ASSERT_TRUE(world.has<compages::renderer::PrefabInstance>(robot));
    ASSERT_GT(world.living(), before);
    ASSERT_TRUE(world.find(robot, "Body/Head/HeadMesh").valid());
    ASSERT_TRUE(
        world.has<compages::renderer::MeshRenderer>(world.find(robot, "Body/LeftLeg")));
}
