// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/Renderer.hpp"
#include "Renderer/Render/Offer.hpp"

#include "Compages/GPU/Draw.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"

#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <array>
#include <span>
#include <string>

namespace compages::renderer
{

namespace
{

constexpr std::array<std::string_view,
                     compages::renderer::shaders::MAX_POINT_LIGHTS>
    kPointPosNames{ "pointLightPos0",
                    "pointLightPos1",
                    "pointLightPos2",
                    "pointLightPos3" };
constexpr std::array<std::string_view,
                     compages::renderer::shaders::MAX_POINT_LIGHTS>
    kPointColorNames{ "pointLightColor0",
                      "pointLightColor1",
                      "pointLightColor2",
                      "pointLightColor3" };
constexpr std::array<std::string_view,
                     compages::renderer::shaders::MAX_POINT_LIGHTS>
    kPointRangeNames{ "pointLightRange0",
                      "pointLightRange1",
                      "pointLightRange2",
                      "pointLightRange3" };

Status bindCamera(compages::renderer::Material& p_material,
                  RenderSnapshot const& p_snapshot)
{
    offer(p_material.program, "view", p_snapshot.camera.view);
    offer(p_material.program, "projection",
                           p_snapshot.camera.reversed_projection);
    return success();
}

Status bindLighting(compages::renderer::Material& p_material,
                    RenderSnapshot const& p_snapshot)
{
    COMPAGES_TRY(bindCamera(p_material, p_snapshot));

    auto const [light_direction, light_color] = sunOf(p_snapshot);

    offer(p_material.program, "lightDir", light_direction);
    offer(p_material.program, "lightColor", light_color);
    offer(p_material.program, "ambient", p_snapshot.environment.ambient);
    offer(p_material.program, "cameraPos", p_snapshot.camera.position);
    offer(p_material.program, "fogColor", p_snapshot.environment.fog_color);
    offer(p_material.program, "fogDensity", p_snapshot.environment.fog_density);
    offer(p_material.program, "exposure", p_snapshot.environment.exposure);
    offer(p_material.program, 
        "toneMapping", static_cast<int>(p_snapshot.environment.tone_mapping));

    const int count = static_cast<int>(
        std::min(p_snapshot.point_lights.size(),
                 static_cast<std::size_t>(
                     compages::renderer::shaders::MAX_POINT_LIGHTS)));
    offer(p_material.program, "pointLightCount", count);

    for (int i = 0; i < compages::renderer::shaders::MAX_POINT_LIGHTS; ++i)
    {
        if (i < count)
        {
            PointLightFrame const& light =
                p_snapshot.point_lights[static_cast<std::size_t>(i)];
            offer(p_material.program, kPointPosNames[static_cast<std::size_t>(i)],
                                   light.position);
            offer(p_material.program, 
                kPointColorNames[static_cast<std::size_t>(i)],
                light.color * light.intensity);
            offer(p_material.program, 
                kPointRangeNames[static_cast<std::size_t>(i)], light.range);
        }
        else
        {
            offer(p_material.program, kPointPosNames[static_cast<std::size_t>(i)],
                                   compages::Vector3f(0.0f, 0.0f, 0.0f));
            offer(p_material.program, 
                kPointColorNames[static_cast<std::size_t>(i)],
                compages::Vector3f(0.0f, 0.0f, 0.0f));
            offer(p_material.program, 
                kPointRangeNames[static_cast<std::size_t>(i)], 1.0f);
        }
    }
    return success();
}

Status bindMaterialFrame(compages::renderer::Material& p_material,
                         RenderSnapshot const& p_snapshot,
                         ShadowPass const& p_shadows)
{
    switch (p_material.family)
    {
        case compages::renderer::ShaderFamily::Lit:
        case compages::renderer::ShaderFamily::PbrMinimal:
            p_shadows.bind(p_material);
            return bindLighting(p_material, p_snapshot);
        case compages::renderer::ShaderFamily::Depth:
        case compages::renderer::ShaderFamily::Normals:
        case compages::renderer::ShaderFamily::Unlit:
        default:
            return bindCamera(p_material, p_snapshot);
    }
}

//! \brief Bind the textures of a material, set their samplers, bind its
//! storage buffers.
Status bindMaterialTextures(compages::renderer::Material& p_material,
                            compages::renderer::AssetManager& p_assets)
{
    if (p_material.textures.size() > compages::renderer::Material::MAX_TEXTURES)
    {
        return failure("the material '" + p_material.name + "' has " +
                       std::to_string(p_material.textures.size()) +
                       " textures, more than the " +
                       std::to_string(compages::renderer::Material::MAX_TEXTURES) +
                       " units the Renderer leaves free");
    }
    std::uint32_t unit = compages::renderer::Material::FIRST_TEXTURE_UNIT;
    for (compages::renderer::MaterialTexture const& texture : p_material.textures)
    {
        COMPAGES_TRY(p_assets.prepare(texture.texture));
        compages::renderer::TextureAsset const* asset =
            p_assets.texture(texture.texture);
        if (asset == nullptr)
        {
            return failure("the material '" + p_material.name +
                           "' lost its texture '" + texture.sampler + "'");
        }
        asset->texture.bind(unit);
        offer(p_material.program, texture.sampler, int(unit));
        ++unit;
    }
    for (compages::renderer::MaterialBuffer const& buffer : p_material.buffers)
    {
        COMPAGES_TRY(p_material.program.bind(buffer.block, buffer.buffer));
    }
    return success();
}

} // namespace

Status Renderer::render(RenderSnapshot const& p_snapshot,
                        compages::renderer::AssetManager& p_assets)
{
    if (!compages::gpu::inRenderPass())
    {
        return failure(
            "Renderer::render was called with no pass open. The pass says "
            "where to draw and what the target starts from");
    }

    m_queue.build(p_snapshot, p_assets);
    if (m_queue.empty())
    {
        return success();
    }
    COMPAGES_TRY(m_shadows.render(p_snapshot, p_assets));

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
            COMPAGES_TRY(bindMaterialFrame(*material, p_snapshot, m_shadows));
            COMPAGES_TRY(bindMaterialTextures(*material, p_assets));
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

        offer(material->program, "model", item.world_matrix);
        const int joint_count = static_cast<int>(std::min(
            static_cast<std::size_t>(item.joint_count),
            static_cast<std::size_t>(compages::renderer::shaders::MAX_JOINTS)));
        offer(material->program, "uJointCount", joint_count);
        if (joint_count > 0)
        {
            offer(material->program, 
                "uJoints",
                std::span<const compages::Matrix44f>(
                    p_snapshot.joint_palette.data() + item.joint_offset,
                    static_cast<std::size_t>(joint_count)));
        }
        switch (material->family)
        {
            case compages::renderer::ShaderFamily::PbrMinimal:
            {
                offer(material->program, "baseColorFactor",
                                      instance->base_color_factor);
                const bool has_map = instance->base_color_texture.valid();
                offer(material->program, "hasBaseColorMap", has_map);
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
                        COMPAGES_TRY_ASSIGN(
                            m_white, compages::gpu::Texture::create(white));
                        const std::array<std::byte, 4u> pixel{ std::byte{ 255 },
                                                               std::byte{ 255 },
                                                               std::byte{ 255 },
                                                               std::byte{
                                                                   255 } };
                        COMPAGES_TRY(m_white.write(pixel));
                    }
                    picture = &m_white;
                }
                picture->bind(0u);
                offer(material->program, "baseColorMap", 0);
                offer(material->program, "opacity", instance->opacity);
                break;
            }
            case compages::renderer::ShaderFamily::Depth:
                offer(material->program, "near", instance->depth_near);
                offer(material->program, "far", instance->depth_far);
                offer(material->program, "opacity", instance->opacity);
                break;
            case compages::renderer::ShaderFamily::Normals:
                offer(material->program, "opacity", instance->opacity);
                break;
            case compages::renderer::ShaderFamily::Unlit:
            case compages::renderer::ShaderFamily::Lit:
            default:
                offer(material->program, "color", instance->color);
                offer(material->program, "opacity", instance->opacity);
                break;
        }
        // A see-through instance blends over what is behind it; a material
        // made by hand without that pipeline is drawn solid.
        const bool blended = entry.see_through && material->see_through.valid();
        compages::gpu::drawIndexed(blended ? material->see_through
                                           : material->pipeline,
                                   mesh->vertices.handle(),
                                   mesh->indexBuffer(),
                                   mesh->index_type,
                                   mesh->index_count);
    }
    return success();
}

} // namespace compages::renderer
