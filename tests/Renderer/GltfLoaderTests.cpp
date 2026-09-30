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
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/GPU/GPU.hpp"
#include "Compages/Core/AABB.hpp"
#include "Compages/GPU/Statistics.hpp"
#include "Compages/Renderer/Render/SceneExtractor.hpp"
#include "Compages/Renderer/Render/Renderer.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/World.hpp"

#include "Compages/Core/File.hpp"

#include <algorithm>
#include <cmath>




using namespace tests;

namespace
{

std::string dataPath(std::string const& p_name)
{
    for (const char* root :
         { "external/Compages-data/",
           "../external/Compages-data/",
           "external/Compages-data/",
           "../external/Compages-data/" })
    {
        if (compages::core::File::exist(root + p_name))
        {
            return root + p_name;
        }
    }
    const std::string three_js =
        "/home/qq/three.js/examples/models/gltf/" + p_name;
    if (compages::core::File::exist(three_js))
    {
        return three_js;
    }
    return {};
}

} // namespace

TEST(GltfLoaderHeadless, LoadsReusableAssetsWithoutAWorldOrDevice)
{
    const std::string path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing from Compages-data";
    }
    ASSERT_FALSE(compages::gpu::initialized());

    compages::renderer::AssetManager assets;
    auto loaded = assets.load(path);
    ASSERT_TRUE(bool(loaded)) << loaded.error();
    ASSERT_NE(assets.prefab(loaded.value()), nullptr);
    ASSERT_GT(assets.meshCount(), 0u);
    ASSERT_FALSE(compages::gpu::initialized());
}

class GltfLoaderTest: public GPUTest
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
        return 128;
    }

    [[nodiscard]] int targetHeight() const override
    {
        return 128;
    }
};

TEST_F(GltfLoaderTest, SeparatesLoadingFromInstantiation)
{
    const std::string path = dataPath("Soldier.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Soldier.glb is missing from external/Compages-data";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene scene(world, assets);

    auto loaded = assets.load(path);
    ASSERT_TRUE(bool(loaded)) << loaded.error();
    ASSERT_EQ(world.living(), 0u);

    auto first = scene.instantiate(loaded.value());
    ASSERT_TRUE(bool(first)) << first.error();
    const std::size_t one_instance = world.living();
    ASSERT_GT(one_instance, 0u);

    auto second = scene.instantiate(loaded.value());
    ASSERT_TRUE(bool(second)) << second.error();
    ASSERT_EQ(world.living(), one_instance * 2u);

    compages::renderer::AssetManager shortcut_assets;
    compages::world::World shortcut_world;
    compages::renderer::Scene shortcut(shortcut_world, shortcut_assets);
    auto direct = shortcut.load(path);
    ASSERT_TRUE(bool(direct)) << direct.error();
    ASSERT_GT(shortcut_world.living(), 0u);
}

TEST_F(GltfLoaderTest, LoadsTheKhronosDuck)
{
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Duck.glb is missing from external/Compages-data";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene view(world, assets);
    auto imported = view.load(path);
    ASSERT_TRUE(bool(imported)) << imported.error();
    ASSERT_TRUE(imported.value().id().valid());
    ASSERT_GT(assets.meshCount(), 0u);
    ASSERT_GT(world.living(), 0u);
    ASSERT_GT(assets.meshCount(), 0u);

    auto const renderers = world.view<compages::renderer::MeshRenderer>();
    ASSERT_GT(renderers.size(), 0u);
    {
        auto const first = *renderers.begin();
        compages::renderer::MeshRenderer const& renderer =
            renderers.get<compages::renderer::MeshRenderer>(first);
        compages::renderer::MaterialInstance const* instance =
            assets.materialInstance(renderer.material_instance);
        ASSERT_NE(instance, nullptr);
        ASSERT_TRUE(instance->material.valid())
            << "scene.load stored an empty MaterialId on the instance";
        ASSERT_NE(assets.material(instance->material), nullptr)
            << "the PBR Material was never registered with the AssetManager";
    }

    compages::core::AABB bounds;
    std::size_t mesh_index = 0u;
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId p_entity, compages::renderer::MeshRenderer const& p_renderer) {
        compages::renderer::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
        ASSERT_NE(mesh, nullptr);
        EXPECT_FALSE(mesh->local_bounds.empty()) << "mesh " << mesh_index;
        bounds = bounds.merged(
            mesh->local_bounds.transformed(world.worldMatrix(p_entity)));
        ++mesh_index;
    });
    ASSERT_FALSE(bounds.empty());
    const compages::core::Vector3f extent = bounds.extent();
    EXPECT_TRUE(std::isfinite(bounds.center().x));
    EXPECT_TRUE(std::isfinite(bounds.center().y));
    EXPECT_TRUE(std::isfinite(bounds.center().z));
    // Duck.glb vertices are in centimetres; the root scale is 0.01, so the
    // world duck is about a metre tall — not 100 m and not a millimetre.
    EXPECT_GT(extent.y, 0.3f);
    EXPECT_LT(extent.y, 2.0f);

    compages::world::EntityId camera = world.create("Camera");
    compages::world::Camera lens;
    lens.projection = compages::world::Camera::Projection::Perspective;
    lens.fov = units::angle::degree_t(45.0);
    lens.near_plane = 0.05f;
    lens.far_plane = 50.0f;
    world.add(camera, lens);
    world.transform(camera).position =
        bounds.center() + compages::core::Vector3f(0.0f, extent.y * 0.3f, extent.y * 4.0f);
    world.update();

    view.activeCamera(camera);
    view.renderSettings().frustum_culling = false;
    auto uncull = compages::renderer::SceneExtractor::extract(view, 1280u, 720u);
    ASSERT_TRUE(bool(uncull)) << uncull.error();
    EXPECT_FALSE(uncull.value().items.empty())
        << "imported mesh never reached extract";

    view.renderSettings().frustum_culling = true;
    auto culled = compages::renderer::SceneExtractor::extract(view, 1280u, 720u);
    ASSERT_TRUE(bool(culled)) << culled.error();
    EXPECT_FALSE(culled.value().items.empty())
        << "Duck was frustum-culled despite sitting in front of the camera";
}

TEST_F(GltfLoaderTest, RendersTheKhronosDuck)
{
    const std::string path = dataPath("Duck.glb");
    if (path.empty())
    {
        GTEST_SKIP() << "Duck.glb is missing from external/Compages-data";
    }

    compages::renderer::AssetManager assets;
    compages::world::World world;
    compages::renderer::Scene view(world, assets);
    auto imported = view.load(path);
    ASSERT_TRUE(bool(imported)) << imported.error();

    compages::core::AABB bounds;
    auto const renderers = world.view<compages::renderer::MeshRenderer>();
    ASSERT_GT(renderers.size(), 0u);
    auto const first = *renderers.begin();
    compages::renderer::MeshAsset const* mesh =
        assets.mesh(renderers.get<compages::renderer::MeshRenderer>(first).mesh);
    ASSERT_NE(mesh, nullptr);
    ASSERT_GT(mesh->index_count, 0u);
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId p_entity, compages::renderer::MeshRenderer const& p_renderer) {
        compages::renderer::MeshAsset const* one = assets.mesh(p_renderer.mesh);
        ASSERT_NE(one, nullptr);
        bounds = bounds.merged(
            one->local_bounds.transformed(world.worldMatrix(p_entity)));
    });

    const compages::core::Vector3f center = bounds.center();
    const compages::core::Vector3f extent = bounds.extent();

    compages::world::EntityId camera = world.create("Camera");
    compages::world::Camera lens;
    lens.projection = compages::world::Camera::Projection::Perspective;
    lens.fov = units::angle::degree_t(45.0);
    lens.near_plane = std::max(extent.y * 0.01f, 0.01f);
    lens.far_plane = std::max(extent.y * 40.0f, 20.0f);
    world.add(camera, lens);
    world.transform(camera).position =
        center + compages::core::Vector3f(0.0f, extent.y * 0.3f, extent.y * 4.0f);
    world.update();

    view.activeCamera(camera);
    view.renderSettings().frustum_culling = false;
    view.environment().ambient = compages::core::Vector3f(0.4f, 0.4f, 0.4f);

    constexpr std::uint32_t width = 128u;
    constexpr std::uint32_t height = 128u;
    auto snapshot = compages::renderer::SceneExtractor::extract(view, width, height);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();
    ASSERT_FALSE(snapshot.value().items.empty());

    compages::gpu::PassDesc desc;
    desc.width = width;
    desc.height = height;
    desc.color = compages::core::Vector4f(0.12f, 0.14f, 0.18f, 1.0f);
    desc.clear_depth = true;
    auto pass = compages::gpu::RenderPass::begin(desc);
    ASSERT_TRUE(bool(pass)) << pass.error();

    compages::renderer::Renderer renderer;
    auto drawn = renderer.render(snapshot.value(), assets);
    ASSERT_TRUE(bool(drawn)) << drawn.error();
    ASSERT_GE(compages::gpu::frameStatistics().draw_calls, 1u)
        << "Renderer accepted the snapshot but issued no draw";

    auto picture = compages::gpu::readPixels();
    ASSERT_TRUE(bool(picture)) << picture.error();

    std::size_t lit = 0u;
    std::size_t orange = 0u;
    const auto& pixels = picture.value();
    for (std::size_t i = 0u; i + 3u < pixels.size(); i += 4u)
    {
        const int r = static_cast<int>(pixels[i]);
        const int g = static_cast<int>(pixels[i + 1u]);
        const int b = static_cast<int>(pixels[i + 2u]);
        if ((r > 40) || (g > 40) || (b > 50))
        {
            ++lit;
        }
        // The beak atlas cell is orange. Flipping the embedded PNG (the
        // OpenGL-file convention) maps that cell onto the body and the eye
        // onto the beak, so orange disappears from the frame.
        if ((r > 140) && (g < 180) && (b < 80) && (r > g + 20))
        {
            ++orange;
        }
    }

    const compages::core::Vector3f cam_pos = world.transform(camera).position;
    ASSERT_GT(lit, 80u)
        << "Duck produced a draw but almost no visible pixels"
        << " items=" << snapshot.value().items.size()
        << " draws=" << compages::gpu::frameStatistics().draw_calls
        << " indices=" << mesh->index_count
        << " bounds=[" << bounds.min.x << "," << bounds.min.y << ","
        << bounds.min.z << "]-[" << bounds.max.x << "," << bounds.max.y
        << "," << bounds.max.z << "]"
        << " cam=(" << cam_pos.x << "," << cam_pos.y << "," << cam_pos.z
        << ")"
        << " lit_pixels=" << lit
        << " orange_pixels=" << orange;
    ASSERT_GT(orange, 8u)
        << "Duck albedo is vertically flipped: the orange beak is missing"
        << " orange_pixels=" << orange
        << " lit_pixels=" << lit;
}
