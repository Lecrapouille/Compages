// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPUContext.hpp"

#include "Compages/GPU/GPU.hpp"
#include "Compages/Renderer/Assets/Primitives.hpp"

using namespace tests;

class PrimitivesTest: public GPUTest
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

TEST_F(PrimitivesTest, SphereFitsInsideItsBounds)
{
    auto mesh = compages::renderer::makeSphere(0.5f, 8u, 12u);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    const compages::renderer::MeshAsset built = mesh.take();
    ASSERT_FALSE(built.local_bounds.empty());
    const compages::core::Vector3f extent = built.local_bounds.extent();
    ASSERT_NEAR(extent.x, 0.5f, 1.0e-3f);
    ASSERT_NEAR(extent.y, 0.5f, 1.0e-3f);
    ASSERT_NEAR(extent.z, 0.5f, 1.0e-3f);
    ASSERT_GT(built.index_count, 0u);
}

TEST_F(PrimitivesTest, RejectsAnInvalidSphere)
{
    const auto mesh = compages::renderer::makeSphere(0.0f, 8u, 12u);
    ASSERT_FALSE(mesh);
}

TEST_F(PrimitivesTest, CylinderHasClosedSidesAndTwoCaps)
{
    constexpr std::uint32_t slices = 12u;
    auto mesh = compages::renderer::makeCylinder(0.5f, 2.0f, slices);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    const compages::renderer::MeshAsset built = mesh.take();
    // 2 triangles per side + 1 triangle per slice on each cap.
    ASSERT_EQ(built.index_count, (6u * slices) + (2u * 3u * slices));
    const compages::core::Vector3f extent = built.local_bounds.extent();
    EXPECT_NEAR(extent.x, 0.5f, 1.0e-3f);
    EXPECT_NEAR(extent.y, 0.5f, 1.0e-3f);
    EXPECT_NEAR(extent.z, 1.0f, 1.0e-3f);
}

TEST_F(PrimitivesTest, ConeTipSitsOnNegativeZAndHasABaseCapOnly)
{
    constexpr std::uint32_t slices = 16u;
    auto mesh = compages::renderer::makeCone(0.8f, 0.0f, 2.0f, slices);
    ASSERT_TRUE(bool(mesh)) << mesh.error();
    const compages::renderer::MeshAsset built = mesh.take();
    // Sides + the base disk. The collapsed tip is not capped.
    ASSERT_EQ(built.index_count, (6u * slices) + (3u * slices));
    EXPECT_NEAR(built.local_bounds.min.z, -1.0f, 1.0e-3f);
    EXPECT_NEAR(built.local_bounds.max.z, 1.0f, 1.0e-3f);
    EXPECT_NEAR(built.local_bounds.extent().x, 0.8f, 1.0e-3f);
}

//! \brief Every triangle winds counter-clockwise seen from the side its
//! normals face, so that back-face culling keeps the visible side.
TEST_F(PrimitivesTest, TrianglesFaceTheirNormals)
{
    struct Named
    {
        char const* name;
        compages::Result<compages::renderer::MeshAsset> mesh;
    };
    Named shapes[] = {
        { "cube", compages::renderer::makeCube() },
        { "plane", compages::renderer::makePlane(2.0f, 1.0f, 3u, 2u) },
        { "sphere", compages::renderer::makeSphere(0.5f, 8u, 12u) },
        { "cylinder", compages::renderer::makeCylinder(0.5f, 1.0f, 12u) },
        { "cone", compages::renderer::makeCone(0.5f, 0.0f, 1.0f, 12u) },
        { "tube", compages::renderer::makeTube(0.3f, 0.5f, 1.0f, 12u) },
        { "pyramid", compages::renderer::makePyramid(0.5f, 1.0f) },
    };
    for (Named& shape : shapes)
    {
        ASSERT_TRUE(bool(shape.mesh))
            << shape.name << ": " << shape.mesh.error();
        compages::renderer::MeshAsset const& mesh = shape.mesh.value();
        std::size_t wrong = 0u;
        for (std::size_t i = 0u; i + 2u < mesh.source_indices.size(); i += 3u)
        {
            compages::renderer::MeshVertex const& a =
                mesh.source_vertices[mesh.source_indices[i]];
            compages::renderer::MeshVertex const& b =
                mesh.source_vertices[mesh.source_indices[i + 1u]];
            compages::renderer::MeshVertex const& c =
                mesh.source_vertices[mesh.source_indices[i + 2u]];
            const compages::core::Vector3f face = compages::core::vector::cross(
                b.position - a.position, c.position - a.position);
            const compages::core::Vector3f normal =
                a.normal + b.normal + c.normal;
            // The pole of a sphere folds whole triangles onto a point: no area,
            // no side, nothing drawn.
            if (compages::core::vector::dot(face, face) < 1.0e-10f)
            {
                continue;
            }
            if (compages::core::vector::dot(face, normal) < 0.0f)
            {
                ++wrong;
            }
        }
        EXPECT_EQ(wrong, 0u)
            << shape.name
            << " has triangles facing away from its normals, out of "
            << (mesh.source_indices.size() / 3u);
    }
}

//! \brief Sixteen-bit indices on the GPU below 65536 vertices, thirty-two
//! above, from the same 32-bit CPU indices.
TEST_F(PrimitivesTest, UploadPicksTheIndexSizeFromTheVertexCount)
{
    auto meshOf = [](std::size_t p_vertices)
    {
        compages::renderer::MeshAsset mesh;
        mesh.source_vertices.resize(p_vertices);
        const auto last = static_cast<std::uint32_t>(p_vertices - 1u);
        mesh.source_indices = { 0u, last / 2u, last };
        return mesh;
    };

    compages::renderer::MeshAsset small = meshOf(65536u);
    ASSERT_TRUE(bool(small.upload()));
    EXPECT_EQ(small.index_type, compages::gpu::IndexType::UInt16);
    EXPECT_EQ(small.index_count, 3u);
    EXPECT_TRUE(small.uploaded());

    compages::renderer::MeshAsset large = meshOf(70000u);
    ASSERT_TRUE(bool(large.upload()));
    EXPECT_EQ(large.index_type, compages::gpu::IndexType::UInt32);
    EXPECT_EQ(large.index_count, 3u);
    EXPECT_TRUE(large.uploaded());
    EXPECT_EQ(large.indexBuffer(), large.long_indices.handle());
}
