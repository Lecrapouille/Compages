// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Procedural/Primitives.hpp"
#include "Compages/Renderer/Render/Picker.hpp"
#include "Compages/Renderer/Render/Renderer.hpp"
#include "Compages/Renderer/Render/SceneExtractor.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/Renderer/Components/Helpers.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Components/ScreenEffects.hpp"
#include "Compages/World/World.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"

#include <gmock/gmock.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>




using namespace tests;

constexpr int WIDTH = 32;
constexpr int HEIGHT = 32;

class WorldDrawTest: public GPUTest
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
                                    compages::Vector3f(1.0f, 0.0f, 0.0f) });
    ASSERT_TRUE(bool(instance_id)) << instance_id.error();

    compages::world::World world;
    compages::renderer::Scene scene(world, assets);

    compages::world::EntityId cube = world.create("cube");
    world.add(cube,
              compages::renderer::MeshRenderer{ mesh_id.value(), instance_id.value() });

    compages::world::EntityId cam = world.create("camera");
    world.transform(cam).position = compages::Vector3f(0.0f, 0.0f, 3.0f);
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
    desc.color = compages::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
    desc.clear_depth = true;
    compages::gpu::RenderPass pass(desc);
    ASSERT_TRUE(pass.open()) << compages::gpu::takeFrameError();

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
    world.transform(cam).position = compages::Vector3f(0.0f, 0.0f, 6.0f);
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

TEST_F(WorldDrawTest, RendersTheSceneIntoAFile)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.0f, 0.0f, 0.0f);
    scene.camera().position(0.0f, 0.0f, 3.0f).lookAt(0.0f, 0.0f, 0.0f);
    scene.sun();
    scene.box("Cube", compages::renderer::color(1.0f, 1.0f, 1.0f));
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "compages_scene_test.png";
    auto written = scene.renderToFile(path, 64u, 48u);
    ASSERT_TRUE(bool(written)) << written.error();

    auto picture = compages::gpu::Texture::fromFile(path, { .mipmaps = false });
    ASSERT_TRUE(bool(picture)) << picture.error();
    EXPECT_EQ(picture.value().width(), 64u);
    EXPECT_EQ(picture.value().height(), 48u);
    auto pixels = picture.value().read();
    ASSERT_TRUE(bool(pixels)) << pixels.error();
    // The centre is the cube, not the black background.
    const std::size_t at = ((24u * 64u) + 32u) * 4u;
    EXPECT_GT(static_cast<int>(pixels.value()[at]), 0);
    std::filesystem::remove(path);
}

// An unlit shape keeps its colour whatever the light: white stays white with
// no sun and no ambient.
TEST_F(WorldDrawTest, AnUnlitShapeIsDrawnAsItsColour)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.0f, 0.0f, 0.0f).ambient(0.0f, 0.0f, 0.0f);
    scene.environment().default_light_color = compages::Vector3f(0.0f, 0.0f, 0.0f);
    scene.camera().position(0.0f, 0.0f, 3.0f).lookAt(0.0f, 0.0f, 0.0f);
    scene.box("Marker", compages::renderer::unlit(1.0f, 1.0f, 1.0f));
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "compages_unlit_test.png";
    ASSERT_TRUE(bool(scene.renderToFile(path, 32u, 32u)));
    auto picture = compages::gpu::Texture::fromFile(path, { .mipmaps = false });
    ASSERT_TRUE(bool(picture)) << picture.error();
    auto pixels = picture.value().read().take();
    const std::size_t at = ((16u * 32u) + 16u) * 4u;
    EXPECT_EQ(static_cast<int>(pixels[at]), 255);
    EXPECT_EQ(static_cast<int>(pixels[at + 1u]), 255);
    std::filesystem::remove(path);
}

// Helpers are components of entities: a grid and axes are entities of their
// own, the bounds a component of the shape, removed when hidden.
TEST_F(WorldDrawTest, HelpersAreComponentsOfEntities)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    compages::world::Entity grid = scene.grid(4.0f, 8u);
    compages::world::Entity axes = scene.axes(0.5f);
    compages::world::Entity cube = scene.box("Cube");
    scene.showBounds(cube.id(), true);

    EXPECT_FLOAT_EQ(grid.get<compages::renderer::Grid>().size, 4.0f);
    EXPECT_EQ(grid.get<compages::renderer::Grid>().divisions, 8u);
    EXPECT_FLOAT_EQ(axes.get<compages::renderer::Axes>().length, 0.5f);
    EXPECT_TRUE(cube.has<compages::renderer::ShowBounds>());
    scene.showBounds(cube.id(), false);
    EXPECT_FALSE(cube.has<compages::renderer::ShowBounds>());

    // Drawn with no frame error.
    ASSERT_TRUE(bool(scene.prepare()));
    scene.camera().position(0.0f, 2.0f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    scene.update(compages::world::ViewFrame{});
    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "compages_helpers_test.png";
    auto written = scene.renderToFile(path, 32u, 32u);
    EXPECT_TRUE(bool(written)) << written.error();
    std::filesystem::remove(path);
}

// A Scene draws into a picture of its own; readColor() gives the top row
// first, as an image file does: a white box above the middle is at the top.
// The depth read back in metres: a wall 7 m in front of the camera, nothing
// above it.
TEST_F(WorldDrawTest, ReadsTheDepthInMetres)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.box("Wall", compages::renderer::unlit(1.0f, 1.0f, 1.0f)).position(0.0f, -5.0f, -7.0f).scale(40.0f, 10.0f, 0.1f);
    compages::world::Entity eye = scene.camera();
    eye.position(0.0f, 0.0f, 0.0f);
    eye.get<compages::world::Camera>().near_plane = 0.1f;
    eye.get<compages::world::Camera>().far_plane = 500.0f;
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    auto target = compages::renderer::RenderTarget::create(32u, 32u);
    ASSERT_TRUE(bool(target));
    scene.render(eye.id(), target.value());
    auto metres = target.value().readDepth(eye.get<compages::world::Camera>());
    ASSERT_TRUE(bool(metres)) << metres.error();
    // The wall is in the lower half (top row first), the sky above.
    EXPECT_NEAR(metres.value()[(28u * 32u) + 16u], 6.95f, 0.01f);
    EXPECT_TRUE(std::isinf(metres.value()[(3u * 32u) + 16u]));
}

// A view with screen effects is drawn in linear light, then given the tone
// curve: a filter that changes nothing gives back the same picture (the
// background, an unlit colour, a lit face), and a filter is run.
TEST_F(WorldDrawTest, ScreenEffectsKeepThePictureAndRunTheFilters)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.2f, 0.4f, 0.6f);
    scene.box("Marker", compages::renderer::unlit(0.9f, 0.3f, 0.1f)).position(-1.2f, 0.0f, 0.0f);
    scene.box("Lit", compages::renderer::color(0.3f, 0.7f, 0.4f)).position(1.2f, 0.0f, 0.0f);
    scene.sun().position(1.0f, 2.0f, 3.0f).lookAt(0.0f, 0.0f, 0.0f);
    compages::world::Entity eye = scene.camera();
    eye.position(0.0f, 0.0f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    auto shoot = [&]()
    {
        auto target = compages::renderer::RenderTarget::create(64u, 32u);
        EXPECT_TRUE(bool(target));
        scene.render(eye.id(), target.value());
        EXPECT_FALSE(compages::gpu::hasFrameError()) << compages::gpu::takeFrameError();
        return target.value().readColor().value();
    };
    const std::vector<std::byte> plain = shoot();

    eye.set(compages::renderer::ScreenEffects{
        .filters = { { .name = "Same", .glsl = "vec3 effect(vec2 uv) { return texture(screenColor, uv).rgb; }" } } });
    const std::vector<std::byte> same = shoot();
    ASSERT_EQ(plain.size(), same.size());
    int worst = 0;
    for (std::size_t i = 0u; i < plain.size(); ++i)
    {
        worst = std::max(worst, std::abs(int(plain[i]) - int(same[i])));
    }
    EXPECT_LE(worst, 3) << "the linear picture and the tone curve change the colours";

    eye.get<compages::renderer::ScreenEffects>().filters = {
        { .name = "No red",
          .glsl = "uniform float keep;\nvec3 effect(vec2 uv) { return texture(screenColor, uv).rgb * vec3(keep, 1.0, 1.0); }",
          .values = { { "keep", 0.0f } } }
    };
    const std::vector<std::byte> cyan = shoot();
    auto reddest = [](std::vector<std::byte> const& p_pixels)
    {
        int red = 0;
        for (std::size_t i = 0u; i < p_pixels.size(); i += 4u)
        {
            red = std::max(red, int(p_pixels[i]));
        }
        return red;
    };
    EXPECT_GT(reddest(plain), 200);
    EXPECT_LT(reddest(cyan), 5);

    // A value for a uniform the text does not have is an error, not a silence.
    eye.get<compages::renderer::ScreenEffects>().filters[0].values = { { "kept", 0.0f } };
    auto target = compages::renderer::RenderTarget::create(64u, 32u);
    ASSERT_TRUE(bool(target));
    scene.render(eye.id(), target.value());
    ASSERT_TRUE(compages::gpu::hasFrameError());
    EXPECT_THAT(compages::gpu::takeFrameError(), ::testing::HasSubstr("kept"));
}

// The bloom spills the light of a white marker on the black around it.
TEST_F(WorldDrawTest, BloomSpillsBrightLight)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.0f, 0.0f, 0.0f);
    scene.box("Bulb", compages::renderer::unlit(1.0f, 1.0f, 1.0f)).scale(0.6f);
    compages::world::Entity eye = scene.camera();
    eye.position(0.0f, 0.0f, 5.0f).lookAt(0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    auto beside = [&]()
    {
        auto target = compages::renderer::RenderTarget::create(128u, 128u);
        EXPECT_TRUE(bool(target));
        scene.render(eye.id(), target.value());
        EXPECT_FALSE(compages::gpu::hasFrameError()) << compages::gpu::takeFrameError();
        return int(target.value().readColor().value()[(64u * 128u + 84u) * 4u]);
    };
    EXPECT_LT(beside(), 3);
    eye.set(compages::renderer::ScreenEffects{ .bloom = { .enabled = true, .threshold = 1.0f, .strength = 1.0f } });
    EXPECT_GT(beside(), 10);
}

TEST_F(WorldDrawTest, DrawsIntoARenderTargetReadTopRowFirst)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.0f, 0.0f, 0.0f);
    scene.box("Up", compages::renderer::unlit(1.0f, 1.0f, 1.0f)).position(0.0f, 1.0f, 0.0f).scale(4.0f, 1.0f, 1.0f);
    compages::world::Entity eye = scene.camera();
    eye.position(0.0f, 0.0f, 4.0f).lookAt(0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    auto target = compages::renderer::RenderTarget::create(32u, 32u);
    ASSERT_TRUE(bool(target)) << target.error();
    scene.render(eye.id(), target.value());
    ASSERT_FALSE(compages::gpu::hasFrameError()) << compages::gpu::takeFrameError();
    auto pixels = target.value().readColor();
    ASSERT_TRUE(bool(pixels)) << pixels.error();
    auto at = [&](std::size_t p_row) { return static_cast<int>(pixels.value()[(p_row * 32u + 16u) * 4u]); };
    EXPECT_GT(at(9u), 200) << "the box above the middle is not in the upper rows";
    EXPECT_LT(at(24u), 50);

    // The same size again keeps the pictures; another size makes new ones.
    EXPECT_TRUE(bool(target.value().resize(32u, 32u)));
    EXPECT_TRUE(bool(target.value().resize(16u, 8u)));
    EXPECT_EQ(target.value().width(), 16u);
    EXPECT_FALSE(bool(target.value().resize(0u, 8u)));
    EXPECT_FALSE(target.value().valid());
}

// A camera standing for a sensor (sees_helpers false) does not draw the
// meshes marked Helper: the eye of a robot is not fooled by a marker.
TEST_F(WorldDrawTest, ASensorCameraDoesNotSeeTheHelpers)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    compages::world::Entity marker = scene.box("Marker", compages::renderer::unlit(1.0f, 1.0f, 1.0f));
    marker.get<compages::renderer::MeshRenderer>().flags = compages::renderer::RenderFlags::Helper;
    scene.box("Cube").position(3.0f, 0.0f, 0.0f);
    compages::world::Entity eye = scene.camera();
    eye.position(0.0f, 0.0f, 6.0f).lookAt(0.0f, 0.0f, 0.0f);
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    auto seen = compages::renderer::SceneExtractor::extract(scene, 32u, 32u);
    ASSERT_TRUE(bool(seen)) << seen.error();
    EXPECT_EQ(seen.value().items.size(), 2u);

    eye.get<compages::world::Camera>().sees_helpers = false;
    auto sensed = compages::renderer::SceneExtractor::extract(scene, 32u, 32u);
    ASSERT_TRUE(bool(sensed)) << sensed.error();
    ASSERT_EQ(sensed.value().items.size(), 1u);
    EXPECT_NE(sensed.value().items.front().entity, marker.id());
}

// A camera drawing into the left half of a 64 x 32 target sees a square: its
// projection must not stretch X (the viewport was once applied twice).
TEST_F(WorldDrawTest, ACameraOnHalfThePictureKeepsItsProportions)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    compages::world::Entity eye = scene.camera().position(0.0f, 0.0f, 3.0f);
    eye.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 0.5f, 1.0f };
    scene.box("Cube");
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "compages_half_test.png";
    ASSERT_TRUE(bool(scene.renderToFile(path, 64u, 32u)));
    compages::Matrix44f const& projection = scene.lastCamera().projection;
    EXPECT_NEAR(projection[0][0], projection[1][1], 1e-4f);
    EXPECT_EQ(scene.lastCamera().viewport_width, 32u);
    std::filesystem::remove(path);
}

// On an odd width, two halves must fit: 667 pixels give 333 and 334, not two
// passes of 334 of which the right one leaves the target.
TEST_F(WorldDrawTest, TwoHalvesFitAnOddWidth)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    compages::world::Entity left = scene.camera("Left").position(0.0f, 0.0f, 3.0f);
    left.get<compages::world::Camera>().viewport = { 0.0f, 0.0f, 0.5f, 1.0f };
    compages::world::Entity right = scene.camera("Right").position(0.0f, 0.0f, 3.0f);
    right.get<compages::world::Camera>().viewport = { 0.5f, 0.0f, 0.5f, 1.0f };
    scene.box("Cube");
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    auto color = compages::gpu::Texture::create({ .width = 667u, .height = 31u }).take();
    auto target = compages::gpu::Framebuffer::create(color).take();
    auto drawn = compages::gpu::attempt(
        [&]
        {
            compages::gpu::RenderPass pass(target);
            scene.render(left.id());
            EXPECT_EQ(scene.lastCamera().viewport_width, 334u);
            scene.render(right.id());
            EXPECT_FLOAT_EQ(scene.lastCamera().viewport_x, 334.0f);
            EXPECT_EQ(scene.lastCamera().viewport_width, 333u);
        });
    EXPECT_TRUE(bool(drawn)) << drawn.error();
}

// The Scene draws with the depth reversed: the near plane is stored as 1 and
// the far one as 0, and a near face still hides a far one.
TEST_F(WorldDrawTest, DrawsWithTheDepthReversed)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.background(0.0f, 0.0f, 0.0f);
    compages::world::Entity eye = scene.camera().position(0.0f, 0.0f, 10.0f);
    compages::world::Camera& lens = eye.get<compages::world::Camera>();
    lens.near_plane = 1.0f;
    lens.far_plane = 1.0e9f;   // from a metre to past the Moon
    scene.box("Near", compages::renderer::unlit(1.0f, 0.0f, 0.0f)).position(0.0f, 0.0f, 0.0f);
    scene.box("Far", compages::renderer::unlit(0.0f, 0.0f, 1.0f))
        .position(0.0f, 0.0f, -1.0e6f)
        .scale(1.0e6f);
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});

    const std::filesystem::path path =
        std::filesystem::temp_directory_path() / "compages_reversed_test.png";
    ASSERT_TRUE(bool(scene.renderToFile(path, 32u, 32u)));

    // The reversed projection puts the near plane at 1 and the far one at 0.
    compages::Matrix44f const& p = scene.lastCamera().reversed_projection;
    auto depthAt = [&](float p_distance)
    {
        const float z = (p(2, 2) * -p_distance) + p(2, 3);
        const float w = (p(3, 2) * -p_distance) + p(3, 3);
        return z / w;
    };
    EXPECT_NEAR(depthAt(1.0f), 1.0f, 1e-5f);
    EXPECT_NEAR(depthAt(1.0e9f), 0.0f, 1e-5f);

    // The near red box hides the huge blue one behind it.
    auto picture = compages::gpu::Texture::fromFile(path, { .mipmaps = false });
    ASSERT_TRUE(bool(picture)) << picture.error();
    auto pixels = picture.value().read().take();
    const std::size_t at = ((16u * 32u) + 16u) * 4u;
    EXPECT_EQ(static_cast<int>(pixels[at]), 255);
    EXPECT_EQ(static_cast<int>(pixels[at + 2u]), 0);
    std::filesystem::remove(path);
}

// The picture keeps the proportions of its target: a 64 x 32 picture has a
// projection twice as strong vertically. (The size was once read through a
// reference into the stack of passes, after a new pass had been pushed.)
TEST_F(WorldDrawTest, KeepsTheProportionsOfTheTarget)
{
    compages::world::World world;
    compages::renderer::Scene scene(world);
    scene.camera().position(0.0f, 0.0f, 3.0f);
    scene.box("Cube");
    ASSERT_TRUE(bool(scene.prepare()));
    scene.update(compages::world::ViewFrame{});
    for (int i = 0; i < 8; ++i)   // a few passes deep, so that the stack grows
    {
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "compages_ratio_test.png";
        ASSERT_TRUE(bool(scene.renderToFile(path, 64u, 32u)));
        compages::Matrix44f const& p = scene.lastCamera().projection;
        EXPECT_NEAR(p(1, 1) / p(0, 0), 2.0f, 1e-4f);
        std::filesystem::remove(path);
    }
}
