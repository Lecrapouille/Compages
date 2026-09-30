//==============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//==============================================================================

#include "main.hpp"


#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Render/SceneExtractor.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Query/Raycast.hpp"
#include "Compages/World/World.hpp"



//------------------------------------------------------------------------------
TEST(Raycast, PicksTheCloserOfTwoBoxes)
{
    compages::world::World world;
    const compages::world::EntityId near = world.create("near");
    const compages::world::EntityId far = world.create("far");
    world.add(near, compages::renderer::MeshRenderer{});
    world.add(far, compages::renderer::MeshRenderer{});
    world.update();

    const compages::core::AABB near_box =
        compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f), compages::core::Vector3f(1.0f));
    const compages::core::AABB far_box =
        compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, -8.0f), compages::core::Vector3f(1.0f));

    const compages::core::Ray ray = compages::core::Ray::fromPoints(compages::core::Vector3f(0.0f, 0.0f, 10.0f),
                                    compages::core::Vector3f(0.0f, 0.0f, 0.0f));
    const auto hit = compages::renderer::raycast(
        world,
        ray,
        [&](compages::world::EntityId p_entity, compages::renderer::MeshRenderer const&)
        {
            return (p_entity == near) ? near_box : far_box;
        });
    ASSERT_TRUE(hit.has_value());
    ASSERT_TRUE(hit->entity == near);
}

//------------------------------------------------------------------------------
TEST(Raycast, SkipsADisabledEntity)
{
    compages::world::World world;
    const compages::world::EntityId hidden = world.create("hidden");
    world.add(hidden, compages::renderer::MeshRenderer{});
    world.setEnabled(hidden, false);
    world.update();

    const compages::core::Ray ray = compages::core::Ray::fromPoints(compages::core::Vector3f(0.0f, 0.0f, 10.0f),
                                    compages::core::Vector3f(0.0f, 0.0f, 0.0f));
    const auto hit = compages::renderer::raycast(
        world,
        ray,
        [&](compages::world::EntityId, compages::renderer::MeshRenderer const&)
        {
            return compages::core::AABB::fromCenterExtent(compages::core::Vector3f(0.0f, 0.0f, 0.0f),
                                          compages::core::Vector3f(1.0f));
        });
    ASSERT_FALSE(hit.has_value());
}

//------------------------------------------------------------------------------
TEST(WorldCamera, ScreenRayThroughTheCentreHitsTheOrigin)
{
    compages::world::World world;
    compages::renderer::AssetManager assets;
    compages::renderer::Scene scene(world, assets);

    compages::world::EntityId cam = world.create("cam");
    world.transform(cam).position = compages::core::Vector3f(0.0f, 0.0f, 10.0f);
    world.add(cam, compages::world::Camera{});
    scene.activeCamera(cam);
    world.update();

    auto snapshot = compages::renderer::SceneExtractor::extract(scene, 1.0f);
    ASSERT_TRUE(bool(snapshot)) << snapshot.error();

    const compages::core::Ray ray = snapshot.value().camera.screenRay(400.0f, 300.0f, 800u, 600u);
    // Centre of the view, looking along -Z: the ray should pass near the
    // origin and travel toward decreasing Z.
    ASSERT_NEAR(ray.direction.x, 0.0f, 0.05f);
    ASSERT_NEAR(ray.direction.y, 0.0f, 0.05f);
    ASSERT_LT(ray.direction.z, -0.9f);
}
