// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "main.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Render/RenderQueue.hpp"
#include "Compages/Renderer/Scene/Looks.hpp"

namespace
{

//! \brief An item of the cube mesh at x along the X axis.
compages::renderer::RenderItem itemAt(compages::renderer::MeshAssetId p_mesh,
                                      compages::renderer::MaterialInstanceId p_look,
                                      float p_x)
{
    compages::renderer::RenderItem item;
    item.mesh = p_mesh;
    item.material_instance = p_look;
    item.world_matrix = compages::translate(
        compages::Matrix44f(compages::matrix::Identity), { p_x, 0.0f, 0.0f });
    return item;
}

} // namespace

// The solid items first; then the see-through ones, the farthest first, so
// that each one blends over what is already behind it.
TEST(RenderQueue, DrawsSeeThroughItemsLastAndFarthestFirst)
{
    compages::renderer::AssetManager assets;
    // Nothing goes to the GPU: the queue only reads ids and positions.
    auto mesh = assets.addMesh("cube", compages::renderer::MeshAsset{}).take();
    compages::renderer::Material lit;
    auto material = assets.addMaterial("lit", std::move(lit)).take();
    compages::renderer::MaterialInstance solid;
    solid.material = material;
    compages::renderer::MaterialInstance glass = solid;
    glass.opacity = 0.5f;
    auto solid_id = assets.addMaterialInstance("solid", solid).take();
    auto glass_id = assets.addMaterialInstance("glass", glass).take();

    compages::renderer::RenderSnapshot snapshot;
    snapshot.camera.position = compages::Vector3f(0.0f, 0.0f, 0.0f);
    snapshot.items.push_back(itemAt(mesh, glass_id, 2.0f));   // 0: near glass
    snapshot.items.push_back(itemAt(mesh, solid_id, 9.0f));   // 1: solid
    snapshot.items.push_back(itemAt(mesh, glass_id, 6.0f));   // 2: far glass

    compages::renderer::RenderQueue queue;
    queue.build(snapshot, assets);
    ASSERT_EQ(queue.size(), 3u);
    EXPECT_EQ(queue.entries()[0].item_index, 1u);
    EXPECT_FALSE(queue.entries()[0].see_through);
    EXPECT_EQ(queue.entries()[1].item_index, 2u);
    EXPECT_EQ(queue.entries()[2].item_index, 0u);
    EXPECT_TRUE(queue.entries()[2].see_through);
}

// A colour picked by a person is sRGB; the shaders need linear light.
TEST(Looks, ConvertsAPickedColourToLinearLight)
{
    EXPECT_FLOAT_EQ(compages::renderer::srgbToLinear(0.0f), 0.0f);
    EXPECT_FLOAT_EQ(compages::renderer::srgbToLinear(1.0f), 1.0f);
    EXPECT_NEAR(compages::renderer::srgbToLinear(0.5f), 0.214f, 1e-3f);
    EXPECT_NEAR(compages::renderer::srgbToLinear(0.02f), 0.02f / 12.92f, 1e-6f);
}
