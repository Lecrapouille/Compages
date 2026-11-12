// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/Renderer/Render/LightClusters.hpp"

#include <gtest/gtest.h>

#include <algorithm>

using compages::Vector3f;
using compages::renderer::LightClusters;
using compages::renderer::PointLightFrame;
using compages::renderer::RenderSnapshot;

namespace
{

//! \brief A camera at the origin looking down -z, 1 to 100 m, 1280 x 720.
RenderSnapshot cameraAtTheOrigin()
{
    RenderSnapshot snapshot;
    snapshot.camera.view = compages::Matrix44f(compages::matrix::Identity);
    snapshot.camera.projection =
        compages::perspective<float>(units::angle::radian_t(1.0), 16.0f / 9.0f, 1.0f, 100.0f);
    snapshot.camera.inverse_projection = compages::inverse(snapshot.camera.projection);
    snapshot.camera.viewport_width = 1280u;
    snapshot.camera.viewport_height = 720u;
    return snapshot;
}

PointLightFrame lamp(Vector3f p_at, float p_range)
{
    PointLightFrame light;
    light.position = p_at;
    light.range = p_range;
    return light;
}

} // namespace

// The slices span the near and far planes of the camera.
TEST(LightClusters, SlicesFollowTheCamera)
{
    LightClusters clusters;
    clusters.build(cameraAtTheOrigin(), 0u);
    EXPECT_NEAR(clusters.nearDepth(), 1.0f, 1.0e-3f);
    EXPECT_NEAR(clusters.farDepth(), 100.0f, 0.1f);
}

// A lamp in front of the camera is in the cluster of its centre, and not in
// a cluster far from it; a lamp behind the camera is in none. The first
// lamps of the snapshot (with uniforms of their own) are left out.
TEST(LightClusters, PutsEachLampInTheClustersItLights)
{
    RenderSnapshot snapshot = cameraAtTheOrigin();
    snapshot.point_lights = { lamp(Vector3f(0.0f, 0.0f, -10.0f), 2.0f),   // skipped: first
                              lamp(Vector3f(0.0f, 0.0f, -20.0f), 2.0f),   // in front, centre
                              lamp(Vector3f(0.0f, 0.0f, 30.0f), 5.0f) };  // behind
    LightClusters clusters;
    clusters.build(snapshot, 1u);
    EXPECT_EQ(clusters.lampCount(), 1u);
    const std::vector<std::uint32_t> centre = clusters.lampsOf(clusters.clusterOf(0.5f, 0.5f, 20.0f));
    ASSERT_EQ(centre.size(), 1u);
    EXPECT_EQ(centre.front(), 0u);
    EXPECT_TRUE(clusters.lampsOf(clusters.clusterOf(0.02f, 0.95f, 20.0f)).empty()); // a corner of the picture
    EXPECT_TRUE(clusters.lampsOf(clusters.clusterOf(0.5f, 0.5f, 60.0f)).empty());   // much deeper
}

// A lamp around the camera lights every tile of the slices it reaches.
TEST(LightClusters, ALampAroundTheCameraLightsTheWholePicture)
{
    RenderSnapshot snapshot = cameraAtTheOrigin();
    snapshot.point_lights = { lamp(Vector3f(0.0f, 0.0f, 0.0f), 5.0f) };
    LightClusters clusters;
    clusters.build(snapshot, 0u);
    for (float u : { 0.01f, 0.5f, 0.99f })
    {
        for (float v : { 0.01f, 0.5f, 0.99f })
        {
            EXPECT_EQ(clusters.lampsOf(clusters.clusterOf(u, v, 2.0f)).size(), 1u);
        }
    }
}
