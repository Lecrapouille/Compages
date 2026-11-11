// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Procedural/Primitives.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"
#include "Compages/World/World.hpp"

#include <array>
#include <filesystem>

using namespace tests;

namespace
{

//! \brief A main() that shows the texture of the material, unlit.
constexpr const char* SHOW_TINT = R"(uniform sampler2D tint;
void main()
{
    oColor = vec4(texture(tint, vec2(0.5)).rgb, 1.0);
}
)";

//! \brief Moves every vertex far to the side.
constexpr const char* AWAY = R"(void deform(inout vec4 world, inout vec3 normal)
{
    world.x += 100.0;
}
)";

} // namespace

// ****************************************************************************
//! \brief A Scene drawing one box, seen from the front, on black.
// ****************************************************************************
class MaterialTest: public GPUTest
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
        m_scene = std::make_unique<compages::renderer::Scene>(m_world);
        m_scene->background(0.0f, 0.0f, 0.0f);
        m_scene->camera().position(0.0f, 0.0f, 3.0f).lookAt(0.0f, 0.0f, 0.0f);
        m_box = m_scene->box("Box");
    }

    void TearDown() override
    {
        m_scene.reset();
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }

    //! \brief A one-pixel red picture in the catalog.
    compages::renderer::TextureAssetId red()
    {
        compages::renderer::TextureAsset picture;
        picture.description.width = 1u;
        picture.description.height = 1u;
        picture.pixels = { std::byte{ 255 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 255 } };
        auto id = m_scene->assets().addTexture("red", std::move(picture));
        EXPECT_TRUE(bool(id)) << id.error();
        return id ? id.value() : compages::renderer::TextureAssetId{};
    }

    //! \brief A PBR material whose shaders were changed by p_change.
    template <typename Change>
    compages::renderer::MaterialId material(Change p_change)
    {
        compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
        p_change(shaders);
        auto made = compages::renderer::Material::create(
            "Test",
            { .family = compages::renderer::ShaderFamily::PbrMinimal,
              .shaders = std::move(shaders),
              .state = { .depth_test = true, .cull = compages::gpu::CullMode::Back },
              .see_through = std::nullopt });
        EXPECT_TRUE(bool(made)) << made.error();
        if (!made)
        {
            return {};
        }
        compages::renderer::Material material = made.take();
        material.textures = { { .sampler = "tint", .texture = red() } };
        auto id = m_scene->assets().addMaterial("Test", std::move(material));
        EXPECT_TRUE(bool(id)) << id.error();
        return id ? id.value() : compages::renderer::MaterialId{};
    }

    //! \brief The colour of the centre of the picture.
    std::array<int, 3> centre()
    {
        const compages::Status prepared = m_scene->prepare();
        EXPECT_TRUE(bool(prepared)) << prepared.error();
        m_scene->update(compages::world::ViewFrame{});
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "compages_material_test.png";
        const compages::Status rendered = m_scene->renderToFile(path, 32u, 32u);
        EXPECT_TRUE(bool(rendered)) << rendered.error();
        auto picture = compages::gpu::Texture::fromFile(path, { .mipmaps = false });
        if (!picture)
        {
            ADD_FAILURE() << picture.error();
            return { -1, -1, -1 };
        }
        auto pixels = picture.value().read().take();
        std::filesystem::remove(path);
        const std::size_t at = ((16u * 32u) + 16u) * 4u;
        return { int(pixels[at]), int(pixels[at + 1u]), int(pixels[at + 2u]) };
    }

    compages::world::World m_world;
    std::unique_ptr<compages::renderer::Scene> m_scene;
    compages::world::Entity m_box;
};

// The hook main, a texture of the material bound by its sampler name, and
// look() putting the material on the box.
TEST_F(MaterialTest, DrawsWithItsMainAndItsTexture)
{
    const compages::renderer::MaterialId id = material(
        [](compages::renderer::shaders::ShaderPair& p_shaders)
        { p_shaders.fragment().hook("main", SHOW_TINT); });
    m_scene->look(m_box, id);

    EXPECT_EQ(centre(), (std::array<int, 3>{ 255, 0, 0 }));
}

TEST_F(MaterialTest, TheHookDeformMovesTheVertices)
{
    const compages::renderer::MaterialId id = material(
        [](compages::renderer::shaders::ShaderPair& p_shaders)
        {
            p_shaders.vertex().hook("deform", AWAY);
            p_shaders.fragment().hook("main", SHOW_TINT);
        });
    m_scene->look(m_box, id);

    EXPECT_EQ(centre(), (std::array<int, 3>{ 0, 0, 0 }));
}

TEST_F(MaterialTest, RefusesAHookThatIsNotThere)
{
    compages::renderer::shaders::ShaderPair shaders = compages::renderer::shaders::pbr();
    shaders.fragment().hook("deform", AWAY); // deform is a hook of the vertex stage
    auto made = compages::renderer::Material::create(
        "Wrong",
        { .family = compages::renderer::ShaderFamily::PbrMinimal,
          .shaders = std::move(shaders),
          .state = {},
          .see_through = std::nullopt });
    ASSERT_FALSE(bool(made));
    EXPECT_THAT(made.error(), HasSubstr("known: main"));
}

// reshape() changes the mesh in place: what was drawn at the centre is gone.
TEST_F(MaterialTest, ReshapeGivesNewGeometry)
{
    m_scene->look(m_box, compages::renderer::unlit(1.0f, 1.0f, 1.0f));
    EXPECT_EQ(centre()[0], 255);

    auto small = compages::renderer::makeSphere(0.01f, 8u, 8u);
    ASSERT_TRUE(bool(small)) << small.error();
    compages::renderer::MeshAsset mesh = small.take();
    for (compages::renderer::MeshVertex& corner : mesh.source_vertices)
    {
        corner.position.x += 1.0f;
    }
    mesh.releaseGpu();
    ASSERT_TRUE(bool(m_scene->reshape(m_box, std::move(mesh))));
    EXPECT_EQ(centre()[0], 0);

    const compages::world::Entity camera = m_scene->activeCamera();
    EXPECT_FALSE(bool(m_scene->reshape(camera, compages::renderer::MeshAsset{})));
}

// The shadow has the shape of the picture: a box moved away by its hook
// "deform" leaves no shadow where it was. Sun straight above a floor.
TEST_F(MaterialTest, TheShadowFollowsTheHookDeform)
{
    m_scene->environment().shadows = true;
    // Above and in front: the box is in the picture (what the camera does not
    // see casts no shadow), its centre is the floor under the box.
    m_scene->activeCamera().position(0.0f, 8.0f, 6.0f).lookAt(0.0f, -1.0f, 0.0f);
    m_scene->sun("Sun", { 1.0f, 1.0f, 1.0f }, 1.0f).position(0.5f, 10.0f, 0.3f).lookAt(0.0f, 0.0f, 0.0f);
    m_scene->plane("Floor", compages::renderer::color(1.0f, 1.0f, 1.0f))
        .position(0.0f, -1.0f, 0.0f)
        .scale(20.0f, 1.0f, 20.0f);
    // The box flat above the floor, between the sun and the floor.
    m_box.position(0.0f, 2.0f, 0.0f).scale(1.5f, 0.1f, 1.5f);
    m_scene->look(m_box, compages::renderer::color(1.0f, 1.0f, 1.0f));
    const int shaded = centre()[0];

    const compages::renderer::MaterialId away = material(
        [](compages::renderer::shaders::ShaderPair& p_shaders)
        { p_shaders.vertex().hook("deform", AWAY); });
    m_scene->look(m_box, away);
    const int moved = centre()[0];
    m_scene->environment().shadows = false;
    const int unshaded = centre()[0];
    // The plain box shades the floor; the moved one leaves it as without
    // any shadow.
    EXPECT_LT(shaded, unshaded - 5) << "shadow " << shaded << ", no shadow " << unshaded;
    EXPECT_NEAR(moved, unshaded, 2) << "moved " << moved << ", no shadow " << unshaded;
}
