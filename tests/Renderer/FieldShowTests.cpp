// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Components/FieldView.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"
#include "Compages/World/World.hpp"

#include <filesystem>

using namespace tests;

// ****************************************************************************
//! \brief A field of 4 x 4 points from -2 to 2, seen from above, on black.
// ****************************************************************************
class FieldShowTest: public GPUTest
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
        m_scene->camera().position(0.0f, 3.0f, 0.5f).lookAt(0.0f, 0.0f, 0.0f);
    }

    void TearDown() override
    {
        m_scene.reset();
        compages::gpu::shutdown();
        GPUTest::TearDown();
    }

    //! \brief The red of the centre of the picture.
    int centre()
    {
        const compages::Status prepared = m_scene->prepare();
        EXPECT_TRUE(bool(prepared)) << prepared.error();
        m_scene->update(compages::world::ViewFrame{});
        const std::filesystem::path path =
            std::filesystem::temp_directory_path() / "compages_field_test.png";
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

    const compages::Grid2D m_grid{ .columns = 4, .rows = 4, .origin = { -2.0f, -2.0f }, .spacing = 4.0f / 3.0f };
    const compages::renderer::FieldLook m_grey{ .colors = compages::renderer::ColorMap::Grey,
                                                .low = 0.0f,
                                                .high = 1.0f,
                                                .relief = 0.0f,
                                                .lit = false };
    compages::world::World m_world;
    std::unique_ptr<compages::renderer::Scene> m_scene;
};

// Grey from 0 to 1, unlit: the value is the brightness.
TEST_F(FieldShowTest, DrawsEachValueAsItsColour)
{
    compages::Field<float> field(m_grid, 1.0f);
    compages::world::Entity plate = m_scene->show(field, m_grey, "Plate");
    EXPECT_TRUE(plate.has<compages::renderer::FieldView>());
    EXPECT_EQ(centre(), 255);

    field.fill(0.5f);
    ASSERT_TRUE(bool(m_scene->show(plate, field, m_grey)));
    EXPECT_NEAR(centre(), 128, 2);

    // The range moves, not the values: 0.5 is now the top.
    compages::renderer::FieldLook half = m_grey;
    half.high = 0.5f;
    ASSERT_TRUE(bool(m_scene->show(plate, field, half)));
    EXPECT_EQ(centre(), 255);
}

TEST_F(FieldShowTest, RefusesAnotherSizeOrNoRange)
{
    compages::Field<float> field(m_grid, 0.0f);
    compages::world::Entity plate = m_scene->show(field, m_grey, "Plate");

    const compages::Grid2D bigger{ .columns = 5, .rows = 4, .origin = { 0.0f, 0.0f }, .spacing = 1.0f };
    auto other = m_scene->show(plate, compages::Field<float>(bigger, 0.0f), m_grey);
    ASSERT_FALSE(bool(other));
    EXPECT_THAT(other.error(), HasSubstr("5 x 4"));

    compages::renderer::FieldLook flat = m_grey;
    flat.high = flat.low;
    EXPECT_FALSE(bool(m_scene->show(plate, field, flat)));

    const compages::world::Entity camera = m_scene->activeCamera();
    EXPECT_FALSE(bool(m_scene->show(camera, field, m_grey)));
}

// The relief lifts the field toward the camera: still drawn, and the box
// that culls it grows with it.
TEST_F(FieldShowTest, RaisesTheValuesWithARelief)
{
    compages::Field<float> field(m_grid, 1.0f);
    compages::renderer::FieldLook raised = m_grey;
    raised.relief = 1.0f;
    m_scene->show(field, raised, "Raised");
    EXPECT_EQ(centre(), 255);
}

// Values already on the GPU (what a compute shader writes): no Field at all.
TEST_F(FieldShowTest, DrawsAStorageBufferOfValues)
{
    auto ones = compages::gpu::StorageBuffer<float>::from(std::vector<float>(16u, 1.0f),
                                                          compages::gpu::BufferUsage::Static);
    ASSERT_TRUE(bool(ones)) << ones.error();
    auto zeros = compages::gpu::StorageBuffer<float>::from(std::vector<float>(16u, 0.0f),
                                                           compages::gpu::BufferUsage::Static);
    ASSERT_TRUE(bool(zeros)) << zeros.error();

    compages::world::Entity plate = m_scene->show(m_grid, ones.value(), m_grey, "Plate");
    EXPECT_EQ(centre(), 255);
    ASSERT_TRUE(bool(m_scene->show(plate, zeros.value(), m_grey)));
    EXPECT_EQ(centre(), 0);

    auto few = compages::gpu::StorageBuffer<float>::from(std::vector<float>(3u, 0.0f),
                                                         compages::gpu::BufferUsage::Static);
    ASSERT_TRUE(bool(few)) << few.error();
    auto refused = m_scene->show(plate, few.value(), m_grey);
    ASSERT_FALSE(bool(refused));
    EXPECT_THAT(refused.error(), HasSubstr("16 points"));
}

// The centre pixel looks at the middle of the plate: the probe finds it, with
// its value; a field elsewhere is not found.
TEST_F(FieldShowTest, ProbesTheValueUnderAPixel)
{
    compages::Field<float> field(m_grid, 0.25f);
    m_scene->show(field, m_grey, "Plate");
    (void)centre(); // a picture, for the camera of the probe
    const std::optional<compages::renderer::FieldProbe> found =
        m_scene->probe(field, m_grey, compages::Vector2f(16.0f, 16.0f));
    ASSERT_TRUE(found.has_value());
    EXPECT_FLOAT_EQ(found->value, 0.25f);
    EXPECT_NEAR(found->point.y, 0.0f, 1.0e-3f);
    EXPECT_LT(std::abs(found->point.x), 0.5f);

    // A field far from what the camera sees: the ray misses it.
    compages::Grid2D far = m_grid;
    far.origin = compages::Vector2f(100.0f, 100.0f);
    EXPECT_FALSE(m_scene->probe(compages::Field<float>(far, 0.25f), m_grey,
                                compages::Vector2f(16.0f, 16.0f)).has_value());
}
