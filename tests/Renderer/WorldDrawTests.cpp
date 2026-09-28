// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"
#include "Compages/Renderer/Render/Picker.hpp"
#include "Compages/Renderer/Render/Renderer.hpp"
#include "Compages/Renderer/Render/SceneExtractor.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/World.hpp"

using namespace tests;

constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

class WorldDrawTest: public GPUTest
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

    [[nodiscard]] int targetWidth() const override
    {
        return WIDTH;
    }

    [[nodiscard]] int targetHeight() const override
    {
        return HEIGHT;
    }
};

// The whole chain in one test: register a mesh and a material in the
// AssetManager, put a MeshRenderer on an EntityId of the World, extract,
// render. The centre pixel of the framebuffer must be brighter than the clear
// colour.
TEST_F(WorldDrawTest, RendersACubeThroughTheWholePipeline)
{
    compages::renderer::AssetManager assets;

    auto mesh = compages::renderer::makeCube();
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    auto mesh_id = assets.addMesh("cube", mesh.take());
    ASSERT_TRUE(bool(mesh_id)) << mesh_id.error();

    auto lit = compages::renderer::makeLitMaterial();
    ASSERT_TRUE(bool(lit)) << lit.error();
    auto lit_id = assets.addMaterial("lit", lit.take());
    ASSERT_TRUE(bool(lit_id)) << lit_id.error();

    auto instance_id = assets.addMaterialInstance(
        "red",
        compages::renderer::MaterialInstance{ lit_id.value(),
                                    Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance_id)) << instance_id.error();

    compages::world::World world;
    compages::renderer::Scene scene(world, assets);

    compages::world::EntityId cube = world.create("cube");
    world.add(cube,
              compages::renderer::MeshRenderer{ mesh_id.value(), instance_id.value() });

    compages::world::EntityId cam = world.create("camera");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 3.0f);
    world.add(
        cam,
        compages::world::Camera{
            compages::world::Camera::Projection::Perspective,
            units::angle::degree_t(60.0),
            1.0f,
            0.1f,
            20.0f });
    scene.activeCamera(cam);

    world.update();
    auto snapshot = compages::renderer::SceneExtractor::extract(scene, WIDTH, HEIGHT);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_FALSE(snapshot.value().items.empty())
        << "extraction produced no items: the cube must have been culled or "
           "its ids were not resolved";

    compages::gpu::PassDesc desc;
    desc.width = WIDTH;
    desc.height = HEIGHT;
    desc.color = Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    desc.clear_depth = true;
    auto pass = compages::gpu::RenderPass::begin(desc);
    ASSERT_TRUE(bool(pass)) << pass.error();

    compages::renderer::Renderer renderer;
    auto drawn = renderer.render(snapshot.value(), assets);
    ASSERT_TRUE(bool(drawn)) << drawn.error();

    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();
    const std::size_t at =
        ((static_cast<std::size_t>(HEIGHT / 2) * WIDTH) + (WIDTH / 2)) * 4u;
    ASSERT_GT(static_cast<int>(picture.value()[at]), 20)
        << "the centre pixel is still black: the cube did not draw";
}

TEST_F(WorldDrawTest, PicksTheCubeUnderTheCentrePixel)
{
    compages::renderer::AssetManager assets;
    auto mesh = compages::renderer::makeCube();
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    auto mesh_id = assets.addMesh("cube", mesh.take());
    ASSERT_TRUE(bool(mesh_id)) << mesh_id.error();

    compages::world::World world;
    compages::renderer::Scene scene(world, assets);

    compages::world::EntityId box = world.create("box");
    world.add(box, compages::renderer::MeshRenderer{ mesh_id.value(), {} });

    compages::world::EntityId cam = world.create("cam");
    world.transform(cam).position = Vector3f(0.0f, 0.0f, 6.0f);
    world.add(cam, compages::world::Camera{});
    scene.activeCamera(cam);
    world.update();

    auto snapshot = compages::renderer::SceneExtractor::extract(scene, WIDTH, HEIGHT);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();

    const auto hit = compages::renderer::pickAt(scene,
                                      snapshot.value().camera,
                                      static_cast<float>(WIDTH) * 0.5f,
                                      static_cast<float>(HEIGHT) * 0.5f,
                                      static_cast<std::uint32_t>(WIDTH),
                                      static_cast<std::uint32_t>(HEIGHT));
    ASSERT_TRUE(hit.has_value());
    ASSERT_TRUE(hit->entity == box);
}
