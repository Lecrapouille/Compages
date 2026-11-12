// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"
#include "Compages/World/World.hpp"

#include <filesystem>
#include <memory>

using namespace tests;

// ****************************************************************************
//! \brief A black room with a sun, a dark box off to the side, and the
//! camera looking at nothing: what the centre of the picture shows is the
//! air.
// ****************************************************************************
class VolumetricFogTest: public GPUTest
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
        m_scene->background(0.0f, 0.0f, 0.0f).ambient(0.0f, 0.0f, 0.0f);
        m_scene->sun("Sun", { 1.0f, 1.0f, 1.0f }, 2.0f);
        m_scene->camera().position(0.0f, 1.0f, 5.0f).lookAt(0.0f, 1.0f, 0.0f);
        m_scene->box("Box", compages::renderer::color(0.1f, 0.1f, 0.1f)).position(3.0f, 0.5f, 0.0f);
    }

    void TearDown() override
    {
        m_scene.reset();
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }

    //! \brief The brightness of the centre of the picture.
    int centre()
    {
        const compages::Status prepared = m_scene->prepare();
        EXPECT_TRUE(bool(prepared)) << prepared.error();
        m_scene->update(compages::world::ViewFrame{});
        const std::filesystem::path path = std::filesystem::temp_directory_path() / "compages_fog_test.png";
        const compages::Status rendered = m_scene->renderToFile(path, 32u, 32u);
        EXPECT_TRUE(bool(rendered)) << rendered.error();
        auto picture = compages::gpu::Texture::fromFile(path, { .mipmaps = false });
        if (!picture)
        {
            ADD_FAILURE() << picture.error();
            return -1;
        }
        auto pixels = picture.value().read().take();
        std::filesystem::remove(path);
        return int(pixels[((16u * 32u) + 16u) * 4u]);
    }

    compages::world::World m_world;
    std::unique_ptr<compages::renderer::Scene> m_scene;
};

// Off, the air is the black background; on, the fog lit by the sun glows,
// and thicker fog glows more.
TEST_F(VolumetricFogTest, TheAirGlowsWhereTheLightCrossesIt)
{
    EXPECT_LT(centre(), 5);
    compages::renderer::VolumetricFog& fog = m_scene->environment().volumetric;
    fog.enabled = true;
    fog.noise = 0.0f;
    fog.falloff = 0.0f;
    fog.density = 0.01f;
    const int thin = centre();
    EXPECT_GT(thin, 5);
    fog.density = 0.05f;
    EXPECT_GT(centre(), thin);
}
