// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/Renderer/Serialization/SceneSerializer.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/World.hpp"

#include <cstdio>
#include <string>

using namespace tests;

namespace
{

std::string tempScenePath()
{
    return std::string("/tmp/compages_scene_test_") + std::to_string(getpid()) +
           ".json";
}

} // namespace

class SceneSerializerTest: public GPUTest
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

TEST_F(SceneSerializerTest, RoundTripsAMeshRenderer)
{
    compages::renderer::AssetManager assets;
    auto cube = compages::renderer::makeCube();
    ASSERT_TRUE(bool(cube));
    auto mesh = assets.addMesh("cube", cube.take());
    ASSERT_TRUE(bool(mesh));
    auto lit = compages::renderer::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    auto instance = assets.addMaterialInstance(
        "red",
        compages::renderer::MaterialInstance{ material.value(), Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance));

    compages::world::World world;
    compages::world::EntityId root = world.create("Root");
    compages::world::EntityId box = world.create("Box");
    world.transform(box).position = Vector3f(1.0f, 2.0f, 3.0f);
    world.add(box, compages::renderer::MeshRenderer{ mesh.value(), instance.value() });
    world.setParent(box, root);
    world.update();

    const std::string path = tempScenePath();
    ASSERT_TRUE(bool(compages::renderer::save(world, assets, path)));

    compages::world::World loaded;
    auto roots = compages::renderer::load(loaded, assets, path);
    ASSERT_TRUE(bool(roots));
    ASSERT_EQ(roots.value().size(), 1u);
    compages::world::EntityId loaded_box = loaded.find(roots.value().front(), "Box");
    ASSERT_TRUE(loaded_box.valid());
    ASSERT_TRUE(loaded.has<compages::renderer::MeshRenderer>(loaded_box));
    ASSERT_EQ(assets.meshName(loaded.get<compages::renderer::MeshRenderer>(loaded_box).mesh),
              "cube");
    ASSERT_NEAR(loaded.transform(loaded_box).position.x, 1.0f, 1.0e-5f);

    std::remove(path.c_str());
}

TEST_F(SceneSerializerTest, SaveSceneRoundTripsPresentationAndPointLight)
{
    compages::renderer::AssetManager assets;
    auto cube = compages::renderer::makeCube();
    ASSERT_TRUE(bool(cube));
    auto mesh = assets.addMesh("cube", cube.take());
    ASSERT_TRUE(bool(mesh));
    auto lit = compages::renderer::makeLitMaterial();
    ASSERT_TRUE(bool(lit));
    auto material = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(material));
    auto instance = assets.addMaterialInstance(
        "red",
        compages::renderer::MaterialInstance{ material.value(), Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance));

    compages::world::World world;
    compages::renderer::Scene scene(world, assets);
    compages::world::EntityId lamp = world.create("Lamp");
    world.add(lamp, compages::world::PointLight{ Vector3f(1.0f, 0.5f, 0.2f), 2.0f, 15.0f });
    compages::world::EntityId box = world.create("Box");
    world.add(box, compages::renderer::MeshRenderer{ mesh.value(), instance.value() });
    compages::world::EntityId camera = world.create("Camera");
    scene.activeCamera(camera);
    scene.renderSettings().clear_color = Vector4f(0.1f, 0.2f, 0.3f, 1.0f);
    world.update();

    const std::string path = tempScenePath();
    ASSERT_TRUE(bool(compages::renderer::saveScene(scene, path)));

    compages::world::World loaded_world;
    compages::renderer::Scene loaded_scene(loaded_world, assets);
    auto roots = compages::renderer::loadScene(loaded_scene, path);
    ASSERT_TRUE(bool(roots));
    compages::world::EntityId loaded_lamp{};
    compages::world::EntityId loaded_camera{};
    for (compages::world::EntityId root : roots.value())
    {
        if (loaded_world.name(root) == "Lamp")
        {
            loaded_lamp = root;
        }
        if (loaded_world.name(root) == "Camera")
        {
            loaded_camera = root;
        }
    }
    ASSERT_TRUE(loaded_lamp.valid());
    ASSERT_TRUE(loaded_camera.valid());
    ASSERT_TRUE(loaded_world.has<compages::world::PointLight>(loaded_lamp));
    ASSERT_NEAR(loaded_scene.renderSettings().clear_color.y, 0.2f, 1.0e-5f);
    ASSERT_EQ(loaded_scene.activeCamera(), loaded_camera);

    std::remove(path.c_str());
}
