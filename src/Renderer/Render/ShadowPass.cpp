// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/ShadowPass.hpp"
#include "Renderer/Render/Offer.hpp"
#include "Renderer/Render/MaterialResources.hpp"

#include "Compages/Core/Transformation.hpp"
#include "Compages/GPU/VertexLayout.hpp"
#include "Compages/GPU/Draw.hpp"
#include "Compages/GPU/RenderPass.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Procedural/ShaderLib.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <span>

namespace compages::renderer
{

namespace
{

//! \brief Texture unit of the shadow atlas of the lamps. Far from the units
//! the materials use (0 colour, 1 sun), so that a custom material derived
//! from the PBR one keeps a few for its own maps.
constexpr std::uint32_t LAMP_SHADOW_UNIT = 7u;

//! \brief Draw one item into a shadow picture, skinned if it has joints.
Status drawCaster(compages::gpu::Program& p_program,
                  compages::gpu::Pipeline const& p_pipeline,
                  RenderItem const& p_item,
                  RenderSnapshot const& p_snapshot,
                  AssetManager& p_assets)
{
    COMPAGES_TRY(p_assets.prepare(p_item.mesh));
    compages::renderer::MeshAsset const* mesh = p_assets.mesh(p_item.mesh);
    if (mesh == nullptr)
    {
        return success();
    }
    p_program.set("model", p_item.world_matrix);
    const int joints = static_cast<int>(std::min(static_cast<std::size_t>(p_item.joint_count),
                                                 static_cast<std::size_t>(shaders::MAX_JOINTS)));
    p_program.set("uJointCount", joints);
    if (joints > 0)
    {
        p_program.set("uJoints",
                      std::span<const compages::Matrix44f>(
                          p_snapshot.joint_palette.data() + p_item.joint_offset,
                          static_cast<std::size_t>(joints)));
    }
    compages::gpu::drawIndexed(p_pipeline, mesh->vertices.handle(), mesh->indexBuffer(),
                               mesh->index_type, mesh->index_count);
    return success();
}

//! \brief The material of an item when it draws its own shadow (its hook
//! "deform" moves the corners), else nullptr.
compages::renderer::Material* ownShadow(RenderItem const& p_item, AssetManager& p_assets)
{
    compages::renderer::MaterialInstance const* instance =
        p_assets.materialInstance(p_item.material_instance);
    if (instance == nullptr)
    {
        return nullptr;
    }
    compages::renderer::Material* material = p_assets.material(instance->material);
    return ((material != nullptr) && material->casts_shadow.valid()) ? material : nullptr;
}

//! \brief Draw an item whose material draws its own shadow: its own program,
//! seen from the sun (no view, the projection of the sun), its textures and
//! buffers, without the shadows it would read (the picture being drawn).
Status drawOwnCaster(compages::renderer::Material& p_material,
                     compages::Matrix44f const& p_light_view_projection,
                     RenderItem const& p_item,
                     RenderSnapshot const& p_snapshot,
                     AssetManager& p_assets)
{
    offer(p_material.program, "view", compages::Matrix44f(compages::matrix::Identity));
    offer(p_material.program, "projection", p_light_view_projection);
    offer(p_material.program, "shadowEnabled", 0);
    offer(p_material.program, "lampShadows", compages::Vector4f(0.0f, 0.0f, 0.0f, 0.0f));
    COMPAGES_TRY(bindMaterialResources(p_material, p_assets));
    return drawCaster(p_material.program, p_material.casts_shadow, p_item, p_snapshot, p_assets);
}

} // namespace

Status ShadowPass::render(RenderSnapshot const& p_snapshot, AssetManager& p_assets)
{
    COMPAGES_TRY(renderSun(p_snapshot, p_assets));
    return renderLamps(p_snapshot, p_assets);
}

//! \brief The shadow uniforms; the shadow picture on texture unit 1, the
//! atlas of the lamps on LAMP_SHADOW_UNIT.
void ShadowPass::bind(compages::renderer::Material& p_material) const
{
    offer(p_material.program, "lampShadows", m_lamps.enabled);
    offer(p_material.program, "lampFaces",
                           std::span<const compages::Matrix44f>(m_lamps.faces.data(), m_lamps.faces.size()));
    if (m_lamps.any)
    {
        m_lamp_depth.bind(LAMP_SHADOW_UNIT);
    }
    offer(p_material.program, "lampShadowMap", int(LAMP_SHADOW_UNIT));

    offer(p_material.program, "shadowEnabled", m_sun.enabled ? 1 : 0);
    offer(p_material.program, "lightViewProj", m_sun.light_view_projection);
    offer(p_material.program, "shadowTexel", m_sun.texel);
    offer(p_material.program, "shadowBias", m_sun.bias);
    if (m_sun.enabled)
    {
        m_sun_depth.bind(1u);
    }
    offer(p_material.program, "shadowMap", 1);
}

Status ShadowPass::renderSun(RenderSnapshot const& p_snapshot,
                              compages::renderer::AssetManager& p_assets)
{
    m_sun.enabled = false;
    Environment const& environment = p_snapshot.environment;
    if (!environment.shadows || environment.shadow_resolution == 0u)
    {
        return success();
    }

    // What casts a shadow in the picture, and how big it is: the box of the
    // sun fits it. Casters out of the picture only push the sun back.
    compages::AABB casters;
    for (RenderItem const& item : p_snapshot.items)
    {
        if (item.in_view && !has(item.flags, RenderFlags::NoShadowCaster) &&
            !has(item.flags, RenderFlags::Overlay) && !item.world_bounds.empty())
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
    if (!m_sun_depth.valid() || m_sun_depth.width() != size)
    {
        compages::gpu::TextureDesc depth = compages::gpu::TextureDesc::image(size, size);
        depth.format = compages::gpu::PixelFormat::Depth32F;
        depth.filter(compages::gpu::Filter::Nearest);
        COMPAGES_TRY_ASSIGN(m_sun_depth, compages::gpu::Texture::create(depth));
        const compages::gpu::Attachment none[1]{};
        COMPAGES_TRY_ASSIGN(m_sun_target,
                            compages::gpu::Framebuffer::create(std::span<const compages::gpu::Attachment>(none, 0u),
                                                               compages::gpu::Attachment{ m_sun_depth.handle() }));
    }
    if (!m_sun_pipeline.valid())
    {
        COMPAGES_TRY(m_sun_program.load(shaders::shadow().vertex(), shaders::shadow().fragment()));
        compages::gpu::RenderState state;
        state.depth_test = true;
        state.cull = compages::gpu::CullMode::None;
        COMPAGES_TRY_ASSIGN(m_sun_pipeline,
                            compages::gpu::Pipeline::create<MeshVertex>(
                                m_sun_program, compages::gpu::VertexLayout::of<MeshVertex>(), state));
    }

    // The sun looks at the middle of the casters from beyond them.
    const compages::Vector3f center = (casters.min + casters.max) * 0.5f;
    const compages::Vector3f half = (casters.max - casters.min) * 0.5f;
    const float radius = std::max(compages::vector::norm(half), 0.01f);
    compages::Vector3f travel = sunOf(p_snapshot).first;
    if (compages::vector::norm(travel) < 1.0e-6f)
    {
        return success();
    }
    travel = compages::vector::normalize(travel);
    const compages::Vector3f up =
        std::abs(travel.y) > 0.99f ? compages::Vector3f(1.0f, 0.0f, 0.0f) : compages::Vector3f(0.0f, 1.0f, 0.0f);
    // How far toward the sun a caster out of the picture is: the sun stands
    // beyond it, so that it is drawn into the shadow picture.
    float back = 2.0f * radius;
    for (RenderItem const& item : p_snapshot.items)
    {
        if (item.in_view || item.world_bounds.empty())
        {
            continue;
        }
        compages::AABB const& b = item.world_bounds;
        for (int corner = 0; corner < 8; ++corner)
        {
            const compages::Vector3f p((corner & 1) ? b.max.x : b.min.x,
                                       (corner & 2) ? b.max.y : b.min.y,
                                       (corner & 4) ? b.max.z : b.min.z);
            const compages::Vector3f d = p - center;
            back = std::max(back, -((d.x * travel.x) + (d.y * travel.y) + (d.z * travel.z)) + (0.05f * radius));
        }
    }
    const compages::Matrix44f view = compages::lookAt(center - (travel * back), center, up);
    const compages::Matrix44f projection =
        compages::ortho(-radius, radius, -radius, radius, 0.01f * radius, back + (2.0f * radius));
    m_sun.light_view_projection = projection * view;
    m_sun.texel = (2.0f * radius) / static_cast<float>(size);
    // About one texel of depth, in the [0, 1] depth of the picture.
    m_sun.bias = (m_sun.texel / (back + (2.0f * radius))) * 1.5f;

    compages::gpu::RenderPass const pass(m_sun_target, compages::gpu::PassDesc{ .clear_color = false });
    m_sun_program.set("lightViewProj", m_sun.light_view_projection);
    for (RenderItem const& item : p_snapshot.items)
    {
        if (has(item.flags, RenderFlags::NoShadowCaster) || has(item.flags, RenderFlags::Overlay))
        {
            continue;
        }
        if (compages::renderer::Material* own = ownShadow(item, p_assets); own != nullptr)
        {
            COMPAGES_TRY(drawOwnCaster(*own, m_sun.light_view_projection, item, p_snapshot, p_assets));
            continue;
        }
        COMPAGES_TRY(drawCaster(m_sun_program, m_sun_pipeline, item, p_snapshot, p_assets));
    }
    m_sun.enabled = true;
    return success();
}

Status ShadowPass::renderLamps(RenderSnapshot const& p_snapshot,
                                   compages::renderer::AssetManager& p_assets)
{
    m_lamps = Lamps{};
    const std::uint32_t size = p_snapshot.environment.lamp_shadow_resolution;
    const std::size_t lamps =
        std::min(p_snapshot.point_lights.size(), static_cast<std::size_t>(shaders::MAX_POINT_LIGHTS));
    bool wanted = false;
    for (std::size_t i = 0u; i < lamps; ++i)
    {
        wanted = wanted || p_snapshot.point_lights[i].shadows;
    }
    if (!wanted || (size == 0u))
    {
        return success();
    }

    // One depth picture for every lamp: a row per lamp, a column per face.
    if (!m_lamp_depth.valid() || (m_lamp_depth.width() != 6u * size))
    {
        compages::gpu::TextureDesc depth = compages::gpu::TextureDesc::image(
            6u * size, static_cast<std::uint32_t>(shaders::MAX_POINT_LIGHTS) * size);
        depth.format = compages::gpu::PixelFormat::Depth32F;
        depth.filter(compages::gpu::Filter::Nearest);
        COMPAGES_TRY_ASSIGN(m_lamp_depth, compages::gpu::Texture::create(depth));
        const compages::gpu::Attachment none[1]{};
        COMPAGES_TRY_ASSIGN(m_lamp_target,
                            compages::gpu::Framebuffer::create(std::span<const compages::gpu::Attachment>(none, 0u),
                                                               compages::gpu::Attachment{ m_lamp_depth.handle() }));
    }
    if (!m_lamp_pipeline.valid())
    {
        COMPAGES_TRY(m_lamp_program.load(shaders::lampShadow().vertex(), shaders::lampShadow().fragment()));
        compages::gpu::RenderState state;
        state.depth_test = true;
        state.cull = compages::gpu::CullMode::None;
        COMPAGES_TRY_ASSIGN(m_lamp_pipeline,
                            compages::gpu::Pipeline::create<MeshVertex>(
                                m_lamp_program, compages::gpu::VertexLayout::of<MeshVertex>(), state));
    }

    // The six faces of a cube, as a camera of 90 degrees looking along each axis.
    using compages::Vector3f;
    const std::array<Vector3f, 6u> along{ Vector3f(1.0f, 0.0f, 0.0f), Vector3f(-1.0f, 0.0f, 0.0f),
                                          Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, -1.0f, 0.0f),
                                          Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, -1.0f) };
    const std::array<Vector3f, 6u> up{ Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f),
                                       Vector3f(0.0f, 0.0f, 1.0f), Vector3f(0.0f, 0.0f, 1.0f),
                                       Vector3f(0.0f, 1.0f, 0.0f), Vector3f(0.0f, 1.0f, 0.0f) };

    for (std::size_t i = 0u; i < lamps; ++i)
    {
        PointLightFrame const& lamp = p_snapshot.point_lights[i];
        if (!lamp.shadows || (lamp.range <= 0.0f))
        {
            continue;
        }
        const float near = std::max(0.01f, lamp.range * 0.002f);
        const compages::Matrix44f projection =
            compages::perspective(units::angle::radian_t(0.5f * 3.14159265f), 1.0f, near, lamp.range);
        m_lamp_program.set("lampPos", lamp.position);
        m_lamp_program.set("lampRange", lamp.range);
        for (std::size_t face = 0u; face < 6u; ++face)
        {
            const compages::Matrix44f view_projection =
                projection * compages::lookAt(lamp.position, lamp.position + along[face], up[face]);
            m_lamps.faces[(6u * i) + face] = view_projection;

            compages::gpu::RenderPass const pass(m_lamp_target,
                                                 compages::gpu::PassDesc{ .x = std::uint32_t(face) * size,
                                                                          .y = std::uint32_t(i) * size,
                                                                          .width = size,
                                                                          .height = size,
                                                                          .clear_color = false });
            m_lamp_program.set("lightViewProj", view_projection);
            for (RenderItem const& item : p_snapshot.items)
            {
                // A material that bends its mesh casts no shadow of a lamp:
                // the lamp program would draw the mesh unbent.
                if (has(item.flags, RenderFlags::NoShadowCaster) || has(item.flags, RenderFlags::Overlay) ||
                    (ownShadow(item, p_assets) != nullptr))
                {
                    continue;
                }
                // Beyond the range nothing is lit: nothing there casts a shadow.
                if (!item.world_bounds.empty())
                {
                    const Vector3f nearest(std::clamp(lamp.position.x, item.world_bounds.min.x, item.world_bounds.max.x),
                                           std::clamp(lamp.position.y, item.world_bounds.min.y, item.world_bounds.max.y),
                                           std::clamp(lamp.position.z, item.world_bounds.min.z, item.world_bounds.max.z));
                    if (compages::vector::norm(nearest - lamp.position) > lamp.range)
                    {
                        continue;
                    }
                }
                COMPAGES_TRY(drawCaster(m_lamp_program, m_lamp_pipeline, item, p_snapshot, p_assets));
            }
        }
        float* enabled[4] = { &m_lamps.enabled.x, &m_lamps.enabled.y, &m_lamps.enabled.z, &m_lamps.enabled.w };
        *enabled[i] = 1.0f;
        m_lamps.any = true;
    }
    return success();
}

} // namespace compages::renderer
