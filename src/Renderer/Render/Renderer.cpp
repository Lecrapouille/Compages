// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/Renderer.hpp"

#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/ShaderLib.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/GPU/Core/Layout.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
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
    p_material.program.set("view", p_snapshot.camera.view);
    p_material.program.set("projection", p_snapshot.camera.projection);
    return success();
}

//! \brief Direction the light of the sun travels, and its colour: the first
//! directional light, else the default of the environment.
std::pair<compages::core::Vector3f, compages::core::Vector3f> sunOf(RenderSnapshot const& p_snapshot)
{
    if (!p_snapshot.directional_lights.empty())
    {
        DirectionalLightFrame const& sun = p_snapshot.directional_lights.front();
        return { sun.direction, sun.color * sun.intensity };
    }
    return { p_snapshot.environment.default_light_direction, p_snapshot.environment.default_light_color };
}

//! \brief The shadow uniforms; the shadow picture on texture unit 1.
void bindShadow(compages::renderer::Material& p_material,
                Renderer::Shadow const& p_shadow,
                compages::gpu::Texture const& p_depth)
{
    p_material.program.set("shadowEnabled", p_shadow.enabled ? 1 : 0);
    p_material.program.set("lightViewProj", p_shadow.light_view_projection);
    p_material.program.set("shadowTexel", p_shadow.texel);
    p_material.program.set("shadowBias", p_shadow.bias);
    if (p_shadow.enabled)
    {
        p_depth.bind(1u);
    }
    p_material.program.set("shadowMap", 1);
}

Status bindLighting(compages::renderer::Material& p_material,
                    RenderSnapshot const& p_snapshot)
{
    COMPAGES_TRY(bindCamera(p_material, p_snapshot));

    auto const [light_direction, light_color] = sunOf(p_snapshot);

    p_material.program.set("lightDir", light_direction);
    p_material.program.set("lightColor", light_color);
    p_material.program.set("ambient", p_snapshot.environment.ambient);
    p_material.program.set("cameraPos", p_snapshot.camera.position);
    p_material.program.set("fogColor", p_snapshot.environment.fog_color);
    p_material.program.set("fogDensity", p_snapshot.environment.fog_density);

    const int count = static_cast<int>(
        std::min(p_snapshot.point_lights.size(),
                 static_cast<std::size_t>(
                     compages::renderer::shaders::MAX_POINT_LIGHTS)));
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
            p_material.program.set(kPointPosNames[static_cast<std::size_t>(i)],
                                   compages::core::Vector3f(0.0f, 0.0f, 0.0f));
            p_material.program.set(
                kPointColorNames[static_cast<std::size_t>(i)],
                compages::core::Vector3f(0.0f, 0.0f, 0.0f));
            p_material.program.set(
                kPointRangeNames[static_cast<std::size_t>(i)], 1.0f);
        }
    }
    return success();
}

Status bindMaterialFrame(compages::renderer::Material& p_material,
                         RenderSnapshot const& p_snapshot,
                         Renderer::Shadow const& p_shadow,
                         compages::gpu::Texture const& p_depth)
{
    switch (p_material.family)
    {
        case compages::renderer::ShaderFamily::Lit:
        case compages::renderer::ShaderFamily::PbrMinimal:
            bindShadow(p_material, p_shadow, p_depth);
            return bindLighting(p_material, p_snapshot);
        case compages::renderer::ShaderFamily::Depth:
        case compages::renderer::ShaderFamily::Normals:
        default:
            return bindCamera(p_material, p_snapshot);
    }
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
    COMPAGES_TRY(renderShadow(p_snapshot, p_assets));

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
            COMPAGES_TRY(bindMaterialFrame(*material, p_snapshot, m_shadow, m_shadow_depth));
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
                std::span<const compages::core::Matrix44f>(
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
    return success();
}

Status Renderer::renderShadow(RenderSnapshot const& p_snapshot,
                              compages::renderer::AssetManager& p_assets)
{
    m_shadow.enabled = false;
    Environment const& environment = p_snapshot.environment;
    if (!environment.shadows || environment.shadow_resolution == 0u)
    {
        return success();
    }

    // What casts a shadow, and how big it is: the box of the sun fits it.
    compages::core::AABB casters;
    for (RenderItem const& item : p_snapshot.items)
    {
        if (!has(item.flags, RenderFlags::NoShadowCaster) && !has(item.flags, RenderFlags::Overlay) &&
            !item.world_bounds.empty())
        {
            casters = casters.merged(item.world_bounds);
        }
    }
    if (casters.empty())
    {
        return success();
    }

    // The picture and the program, made once (again when the size changes).
    const std::uint32_t size = environment.shadow_resolution;
    if (!m_shadow_depth.valid() || m_shadow_depth.width() != size)
    {
        compages::gpu::TextureDesc depth = compages::gpu::TextureDesc::image(size, size);
        depth.format = compages::gpu::PixelFormat::Depth32F;
        depth.filter(compages::gpu::Filter::Nearest);
        COMPAGES_TRY_ASSIGN(m_shadow_depth, compages::gpu::Texture::create(depth));
        const compages::gpu::Attachment none[1]{};
        COMPAGES_TRY_ASSIGN(m_shadow_target,
                            compages::gpu::Framebuffer::create(std::span<const compages::gpu::Attachment>(none, 0u),
                                                               compages::gpu::Attachment{ m_shadow_depth.handle() }));
    }
    if (!m_shadow_pipeline.valid())
    {
        COMPAGES_TRY_ASSIGN(m_shadow_program,
                            compages::gpu::Program::fromSources(shaders::shadowVertex(), shaders::shadowFragment()));
        compages::gpu::RenderState state;
        state.depth_test = true;
        state.cull = compages::gpu::CullMode::None;
        COMPAGES_TRY_ASSIGN(m_shadow_pipeline,
                            compages::gpu::Pipeline::create<MeshVertex>(
                                m_shadow_program, compages::gpu::VertexLayout::of<MeshVertex>(), state));
    }

    // The sun looks at the middle of the casters from beyond them.
    const compages::core::Vector3f center = (casters.min + casters.max) * 0.5f;
    const compages::core::Vector3f half = (casters.max - casters.min) * 0.5f;
    const float radius = std::max(compages::core::vector::norm(half), 0.01f);
    compages::core::Vector3f travel = sunOf(p_snapshot).first;
    if (compages::core::vector::norm(travel) < 1.0e-6f)
    {
        return success();
    }
    travel = compages::core::vector::normalize(travel);
    const compages::core::Vector3f up =
        std::abs(travel.y) > 0.99f ? compages::core::Vector3f(1.0f, 0.0f, 0.0f) : compages::core::Vector3f(0.0f, 1.0f, 0.0f);
    const compages::core::Matrix44f view = compages::core::lookAt(center - (travel * (2.0f * radius)), center, up);
    const compages::core::Matrix44f projection =
        compages::core::ortho(-radius, radius, -radius, radius, 0.01f * radius, 4.0f * radius);
    m_shadow.light_view_projection = projection * view;
    m_shadow.texel = (2.0f * radius) / static_cast<float>(size);
    // About one texel of depth, in the [0, 1] depth of the picture.
    m_shadow.bias = (m_shadow.texel / (4.0f * radius)) * 1.5f;

    compages::gpu::RenderPass const pass(m_shadow_target, compages::gpu::PassDesc{ .clear_color = false });
    m_shadow_program.set("lightViewProj", m_shadow.light_view_projection);
    for (RenderItem const& item : p_snapshot.items)
    {
        if (has(item.flags, RenderFlags::NoShadowCaster) || has(item.flags, RenderFlags::Overlay))
        {
            continue;
        }
        COMPAGES_TRY(p_assets.prepare(item.mesh));
        compages::renderer::MeshAsset const* mesh = p_assets.mesh(item.mesh);
        if (mesh == nullptr)
        {
            continue;
        }
        m_shadow_program.set("model", item.world_matrix);
        const int joints = static_cast<int>(std::min(static_cast<std::size_t>(item.joint_count),
                                                     static_cast<std::size_t>(shaders::MAX_JOINTS)));
        m_shadow_program.set("uJointCount", joints);
        if (joints > 0)
        {
            m_shadow_program.set("uJoints",
                                 std::span<const compages::core::Matrix44f>(
                                     p_snapshot.joint_palette.data() + item.joint_offset,
                                     static_cast<std::size_t>(joints)));
        }
        compages::gpu::drawIndexed(m_shadow_pipeline, mesh->vertices.handle(), mesh->indexBuffer(),
                                   mesh->index_type, mesh->index_count);
    }
    m_shadow.enabled = true;
    return success();
}

} // namespace compages::renderer
