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

#include "Compages/Renderer/Render/Renderer.hpp"

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/ShaderLib.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/RenderPass.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string>

namespace compages::renderer
{

namespace
{

constexpr std::array<std::string_view, compages::renderer::shaders::MAX_POINT_LIGHTS>
    kPointPosNames{
        "pointLightPos0", "pointLightPos1", "pointLightPos2", "pointLightPos3"
    };
constexpr std::array<std::string_view, compages::renderer::shaders::MAX_POINT_LIGHTS>
    kPointColorNames{ "pointLightColor0",
                      "pointLightColor1",
                      "pointLightColor2",
                      "pointLightColor3" };
constexpr std::array<std::string_view, compages::renderer::shaders::MAX_POINT_LIGHTS>
    kPointRangeNames{ "pointLightRange0",
                      "pointLightRange1",
                      "pointLightRange2",
                      "pointLightRange3" };

compages::Status bindCamera(compages::renderer::Material& p_material,
                       RenderSnapshot const& p_snapshot)
{
    p_material.program.set("view", p_snapshot.camera.view);
    p_material.program.set("projection",
                                   p_snapshot.camera.projection);
    return compages::success();
}

compages::Status bindLighting(compages::renderer::Material& p_material,
                         RenderSnapshot const& p_snapshot)
{
    COMPAGES_TRY(bindCamera(p_material, p_snapshot));

    Vector3f light_direction = p_snapshot.environment.default_light_direction;
    Vector3f light_color = p_snapshot.environment.default_light_color;
    if (!p_snapshot.directional_lights.empty())
    {
        DirectionalLightFrame const& sun = p_snapshot.directional_lights.front();
        light_direction = sun.direction;
        light_color = sun.color * sun.intensity;
    }

    p_material.program.set("lightDir", light_direction);
    p_material.program.set("lightColor", light_color);
    p_material.program.set("ambient", p_snapshot.environment.ambient);
    p_material.program.set("cameraPos", p_snapshot.camera.position);
    p_material.program.set("fogColor", p_snapshot.environment.fog_color);
    p_material.program.set("fogDensity", p_snapshot.environment.fog_density);

    const int count = static_cast<int>(std::min(
        p_snapshot.point_lights.size(),
        static_cast<std::size_t>(compages::renderer::shaders::MAX_POINT_LIGHTS)));
    p_material.program.set("pointLightCount", count);

    for (int i = 0; i < compages::renderer::shaders::MAX_POINT_LIGHTS; ++i)
    {
        if (i < count)
        {
            PointLightFrame const& light =
                p_snapshot.point_lights[static_cast<std::size_t>(i)];
            p_material.program.set(kPointPosNames[static_cast<std::size_t>(i)],
                                           light.position);
            p_material.program.set(
                kPointColorNames[static_cast<std::size_t>(i)],
                light.color * light.intensity);
            p_material.program.set(
                kPointRangeNames[static_cast<std::size_t>(i)], light.range);
        }
        else
        {
            p_material.program.set(
                kPointPosNames[static_cast<std::size_t>(i)],
                Vector3f(0.0f, 0.0f, 0.0f));
            p_material.program.set(
                kPointColorNames[static_cast<std::size_t>(i)],
                Vector3f(0.0f, 0.0f, 0.0f));
            p_material.program.set(kPointRangeNames[static_cast<std::size_t>(i)],
                                       1.0f);
        }
    }
    return compages::success();
}

compages::Status bindMaterialFrame(compages::renderer::Material& p_material,
                              RenderSnapshot const& p_snapshot)
{
    switch (p_material.family)
    {
        case compages::renderer::ShaderFamily::Lit:
        case compages::renderer::ShaderFamily::PbrMinimal:
            return bindLighting(p_material, p_snapshot);
        case compages::renderer::ShaderFamily::Depth:
        case compages::renderer::ShaderFamily::Normals:
        default:
            return bindCamera(p_material, p_snapshot);
    }
}

} // namespace

//------------------------------------------------------------------------------
compages::Status Renderer::render(RenderSnapshot const& p_snapshot,
                                  compages::renderer::AssetManager& p_assets)
{
    if (!compages::gpu::inRenderPass())
    {
        return compages::failure(
            "Renderer::render was called with no pass open. The pass says "
            "where to draw and what the target starts from");
    }

    m_queue.build(p_snapshot, p_assets);
    if (m_queue.empty())
    {
        return compages::success();
    }

    compages::renderer::MaterialId bound_material;
    compages::renderer::Material* material = nullptr;

    for (QueueEntry const& entry : m_queue.entries())
    {
        RenderItem const& item = p_snapshot.items[entry.item_index];

        compages::renderer::MaterialInstance const* instance =
            p_assets.materialInstance(item.material_instance);
        if (instance == nullptr)
        {
            continue;
        }

        if (!(bound_material == instance->material))
        {
            COMPAGES_TRY(p_assets.prepare(instance->material));
            material = p_assets.material(instance->material);
            if (material == nullptr)
            {
                continue;
            }
            bound_material = instance->material;
            COMPAGES_TRY(bindMaterialFrame(*material, p_snapshot));
        }

        if (material == nullptr)
        {
            continue;
        }

        COMPAGES_TRY(p_assets.prepare(item.mesh));
        compages::renderer::MeshAsset const* mesh = p_assets.mesh(item.mesh);
        if (mesh == nullptr)
        {
            continue;
        }

        material->program.set("model", item.world_matrix);
        const int joint_count = static_cast<int>(std::min(
            static_cast<std::size_t>(item.joint_count),
            static_cast<std::size_t>(compages::renderer::shaders::MAX_JOINTS)));
        material->program.set("uJointCount", joint_count);
        if (joint_count > 0)
        {
            material->program.set(
                "uJoints",
                std::span<const Matrix44f>(
                    p_snapshot.joint_palette.data() + item.joint_offset,
                    static_cast<std::size_t>(joint_count)));
        }
        switch (material->family)
        {
            case compages::renderer::ShaderFamily::PbrMinimal:
            {
                material->program.set("baseColorFactor",
                                              instance->base_color_factor);
                const bool has_map = instance->base_color_texture.valid();
                material->program.set("hasBaseColorMap", has_map);
                compages::gpu::Texture const* picture = nullptr;
                if (has_map)
                {
                    COMPAGES_TRY(
                        p_assets.prepare(instance->base_color_texture));
                    compages::renderer::TextureAsset const* texture =
                        p_assets.texture(instance->base_color_texture);
                    if (texture != nullptr)
                    {
                        picture = &texture->texture;
                    }
                }
                if (picture == nullptr)
                {
                    if (!m_white.valid())
                    {
                        compages::gpu::TextureDesc white;
                        white.width = 1u;
                        white.height = 1u;
                        COMPAGES_TRY_ASSIGN(m_white, compages::gpu::Texture::create(white));
                        const std::array<std::byte, 4u> pixel{
                            std::byte{ 255 }, std::byte{ 255 },
                            std::byte{ 255 }, std::byte{ 255 }
                        };
                        COMPAGES_TRY(m_white.write(pixel));
                    }
                    picture = &m_white;
                }
                picture->bind(0u);
                material->program.set("baseColorMap", 0);
                break;
            }
            case compages::renderer::ShaderFamily::Depth:
                material->program.set("near", instance->depth_near);
                material->program.set("far", instance->depth_far);
                material->program.set("opacity", instance->opacity);
                break;
            case compages::renderer::ShaderFamily::Normals:
                material->program.set("opacity", instance->opacity);
                break;
            case compages::renderer::ShaderFamily::Lit:
            default:
                material->program.set("color", instance->color);
                break;
        }
        compages::gpu::drawIndexed(material->pipeline,
                                 mesh->vertices.handle(),
                                 mesh->indexBuffer(),
                                 mesh->index_type,
                                 mesh->index_count);
    }
    return compages::success();
}

} // namespace compages::renderer
