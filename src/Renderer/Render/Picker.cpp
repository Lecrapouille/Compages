// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/Picker.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Render/CameraFrame.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/World/World.hpp"

namespace compages::renderer
{

std::optional<compages::renderer::RayHit> pick(compages::renderer::Scene const& p_scene, Ray const& p_ray)
{
    compages::world::World const& world = p_scene.world();
    compages::renderer::AssetManager const& assets = p_scene.assets();
    return compages::renderer::raycast(
        world,
        p_ray,
        [&](compages::world::EntityId p_entity, compages::renderer::MeshRenderer const& p_renderer)
        {
            compages::renderer::MeshAsset const* mesh = assets.mesh(p_renderer.mesh);
            if (mesh == nullptr)
            {
                return AABB{};
            }
            return mesh->local_bounds.transformed(world.worldMatrix(p_entity));
        });
}

std::optional<compages::renderer::RayHit> pickAt(compages::renderer::Scene const& p_scene,
                                    CameraFrame const& p_camera,
                                    float p_x,
                                    float p_y,
                                    std::uint32_t p_width,
                                    std::uint32_t p_height)
{
    return pick(p_scene, p_camera.screenRay(p_x, p_y, p_width, p_height));
}

} // namespace compages::renderer
