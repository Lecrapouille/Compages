//=============================================================================
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
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

#include "Compages/Renderer/Render/SceneExtractor.hpp"
#include "Compages/World/Entity.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/Components/Camera.hpp"
#include "Compages/World/Components/Light.hpp"
#include "Compages/World/Components/SkinInstance.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cmath>

using namespace units::literals;

namespace compages::renderer
{

namespace
{

//------------------------------------------------------------------------------
void applyViewport(CameraFrame& p_frame,
                   compages::world::Camera::Viewport const& p_viewport,
                   std::uint32_t p_width,
                   std::uint32_t p_height)
{
    if ((p_width == 0u) || (p_height == 0u))
    {
        return;
    }
    const float x = std::clamp(p_viewport.x, 0.0f, 1.0f);
    const float y = std::clamp(p_viewport.y, 0.0f, 1.0f);
    const float w = std::clamp(p_viewport.width, 0.0f, 1.0f - x);
    const float h = std::clamp(p_viewport.height, 0.0f, 1.0f - y);
    p_frame.viewport_x = x * static_cast<float>(p_width);
    p_frame.viewport_y = y * static_cast<float>(p_height);
    p_frame.viewport_width =
        static_cast<std::uint32_t>(w * static_cast<float>(p_width) + 0.5f);
    p_frame.viewport_height =
        static_cast<std::uint32_t>(h * static_cast<float>(p_height) + 0.5f);
}

[[nodiscard]] float
aspectOf(compages::world::Camera::Viewport const& p_viewport,
         float p_fallback,
         std::uint32_t p_width,
         std::uint32_t p_height)
{
    if ((p_width == 0u) || (p_height == 0u))
    {
        return (p_fallback <= 0.0f) ? 1.0f : p_fallback;
    }
    const float pixel_w = p_viewport.width * static_cast<float>(p_width);
    const float pixel_h = p_viewport.height * static_cast<float>(p_height);
    if (pixel_h <= 1.0e-6f)
    {
        return (p_fallback <= 0.0f) ? 1.0f : p_fallback;
    }
    return pixel_w / pixel_h;
}

CameraFrame buildCameraFrame(compages::world::Camera const& p_camera,
                             compages::core::Matrix44f const& p_world_matrix,
                             float p_aspect,
                             std::uint32_t p_width,
                             std::uint32_t p_height)
{
    CameraFrame frame;
    frame.view = compages::core::inverse(p_world_matrix);
    frame.position = compages::core::Vector3f(p_world_matrix(0, 3),
                                              p_world_matrix(1, 3),
                                              p_world_matrix(2, 3));
    applyViewport(frame, p_camera.viewport, p_width, p_height);

    const float aspect = (p_aspect <= 0.0f) ? 1.0f : p_aspect;
    if (p_camera.projection == compages::world::Camera::Projection::Perspective)
    {
        frame.projection = compages::core::perspective(
            p_camera.fov, aspect, p_camera.near_plane, p_camera.far_plane);
    }
    else
    {
        const float half_h = p_camera.ortho_half_height;
        const float half_w = half_h * aspect;
        frame.projection = compages::core::ortho(-half_w,
                                                 half_w,
                                                 -half_h,
                                                 half_h,
                                                 p_camera.near_plane,
                                                 p_camera.far_plane);
    }
    // Column vectors: clip = projection * view * world, so VP = P * V.
    frame.view_projection = frame.projection * frame.view;
    frame.inverse_view = p_world_matrix;
    frame.inverse_projection = compages::core::inverse(frame.projection);
    frame.frustum =
        compages::core::Frustum::fromViewProjection(frame.view_projection);
    return frame;
}

//------------------------------------------------------------------------------
compages::core::Vector3f forwardOf(compages::core::Matrix44f const& p_world)
{
    // Column 2 is local +Z in world space; the camera looks down local -Z.
    return compages::core::Vector3f(-p_world(0, 2), -p_world(1, 2), -p_world(2, 2));
}

Result<RenderSnapshot> extractWithSize(compages::renderer::Scene const& p_scene,
                                       compages::world::EntityId p_camera,
                                       float p_aspect,
                                       std::uint32_t p_width,
                                       std::uint32_t p_height)
{
    compages::world::World const& world = p_scene.world();
    compages::renderer::AssetManager const& assets = p_scene.assets();

    const compages::world::EntityId camera_entity = p_camera;
    if (!camera_entity.valid() || !world.alive(camera_entity))
    {
        return failure(
            "no camera to draw from: the camera entity is empty or already "
            "destroyed. Scene::camera() makes one");
    }
    compages::world::Camera const* camera =
        world.tryGet<compages::world::Camera>(camera_entity);
    if (camera == nullptr)
    {
        return failure(
            "the entity drawn from has no compages::world::Camera component");
    }

    const float aspect =
        aspectOf(camera->viewport, p_aspect, p_width, p_height);

    RenderSnapshot snapshot;
    snapshot.environment = p_scene.environment();
    snapshot.camera = buildCameraFrame(
        *camera, world.worldMatrix(camera_entity), aspect, p_width, p_height);

    const bool cull = p_scene.renderSettings().frustum_culling;

    snapshot.items.reserve(world.living());
    world.each<compages::renderer::MeshRenderer>(
        [&](compages::world::EntityId entity,
            compages::renderer::MeshRenderer const& mr)
        {
            if (!world.enabledInHierarchy(entity))
            {
                return;
            }

            compages::renderer::MeshAsset const* mesh_asset =
                assets.mesh(mr.mesh);
            if (mesh_asset == nullptr)
            {
                return;
            }
            compages::renderer::MaterialInstance const* instance =
                assets.materialInstance(mr.material_instance);
            if (!mr.material_instance.valid() || (instance == nullptr) ||
                (assets.material(instance->material) == nullptr))
            {
                return;
            }

            RenderItem item;
            item.entity = entity;
            item.mesh = mr.mesh;
            item.material_instance = mr.material_instance;
            item.world_matrix = world.worldMatrix(entity);
            item.world_bounds =
                mesh_asset->local_bounds.transformed(item.world_matrix);
            item.flags = mr.flags;

            if (cull && !snapshot.camera.frustum.contains(item.world_bounds))
            {
                return;
            }

            compages::world::SkinInstance const* skin =
                world.tryGet<compages::world::SkinInstance>(entity);
            if ((skin != nullptr) && !skin->pose.empty())
            {
                item.joint_offset =
                    static_cast<std::uint32_t>(snapshot.joint_palette.size());
                item.joint_count = static_cast<std::uint16_t>(std::min(
                    skin->pose.size(), static_cast<std::size_t>(0xFFFFu)));
                snapshot.joint_palette.insert(snapshot.joint_palette.end(),
                                              skin->pose.begin(),
                                              skin->pose.begin() +
                                                  item.joint_count);
            }

            snapshot.items.emplace_back(item);
        });

    // Directional lights.
    world.each<compages::world::DirectionalLight>(
        [&](compages::world::EntityId entity,
            compages::world::DirectionalLight const& light)
        {
            if (!world.enabledInHierarchy(entity))
            {
                return;
            }
            DirectionalLightFrame frame;
            frame.direction = forwardOf(world.worldMatrix(entity));
            const float m = std::abs(frame.direction.x) +
                            std::abs(frame.direction.y) +
                            std::abs(frame.direction.z);
            if (m < 1.0e-6f)
            {
                frame.direction = p_scene.environment().default_light_direction;
            }
            frame.color = light.color;
            frame.intensity = light.intensity;
            snapshot.directional_lights.emplace_back(frame);
        });

    // Point lights.
    world.each<compages::world::PointLight>(
        [&](compages::world::EntityId entity,
            compages::world::PointLight const& light)
        {
            if (!world.enabledInHierarchy(entity))
            {
                return;
            }
            PointLightFrame frame;
            const compages::core::Matrix44f& world_matrix =
                world.worldMatrix(entity);
            frame.position = compages::core::translation(world_matrix);
            frame.color = light.color;
            frame.intensity = light.intensity;
            frame.range = light.range;
            snapshot.point_lights.emplace_back(frame);
        });

    // The shaders take a handful of lamps: the nearest ones to the camera,
    // so that walking down a corridor of torches lights the ones around.
    const compages::core::Vector3f eye = snapshot.camera.position;
    auto distance2 = [&eye](PointLightFrame const& p_light)
    {
        const compages::core::Vector3f d = p_light.position - eye;
        return (d.x * d.x) + (d.y * d.y) + (d.z * d.z);
    };
    std::stable_sort(
        snapshot.point_lights.begin(),
        snapshot.point_lights.end(),
        [&distance2](PointLightFrame const& p_a, PointLightFrame const& p_b)
        { return distance2(p_a) < distance2(p_b); });

    return snapshot;
}

} // namespace

//------------------------------------------------------------------------------
Result<RenderSnapshot>
SceneExtractor::extract(compages::renderer::Scene const& p_scene,
                        float p_aspect)
{
    return extractWithSize(p_scene, p_scene.activeCamera(), p_aspect, 0u, 0u);
}

//------------------------------------------------------------------------------
Result<RenderSnapshot>
SceneExtractor::extract(compages::renderer::Scene const& p_scene,
                        std::uint32_t p_width,
                        std::uint32_t p_height)
{
    return extract(p_scene, p_scene.activeCamera(), p_width, p_height);
}

Result<RenderSnapshot>
SceneExtractor::extract(compages::renderer::Scene const& p_scene,
                        compages::world::EntityId p_camera,
                        std::uint32_t p_width,
                        std::uint32_t p_height)
{
    const float aspect =
        (p_height == 0u)
            ? 1.0f
            : (static_cast<float>(p_width) / static_cast<float>(p_height));
    return extractWithSize(p_scene, p_camera, aspect, p_width, p_height);
}

} // namespace compages::renderer
