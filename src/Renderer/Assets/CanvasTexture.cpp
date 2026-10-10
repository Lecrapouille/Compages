// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Assets/CanvasTexture.hpp"

#include "Compages/Core/Matrix.hpp"
#include "Compages/GPU/Core/Enums.hpp"
#include "Compages/GPU/Texture.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/TextureAsset.hpp"
#include "Compages/Renderer/Scene.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/World/Entity.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <span>

namespace compages::renderer
{

Status CanvasTexture::create(std::uint32_t p_width,
                             std::uint32_t p_height,
                             Color p_fill)
{
    if ((p_width == 0u) || (p_height == 0u))
    {
        return failure("CanvasTexture size must be positive");
    }
    m_width = p_width;
    m_height = p_height;
    m_pixels.assign(size_t(m_width) * size_t(m_height) * 4u, 0u);
    clear(p_fill);
    m_texture_id = {};
    m_assets = nullptr;
    m_asset_name.clear();
    return success();
}

void CanvasTexture::setMapping(compages::core::AABB p_local_bounds,
                               bool p_flip_u,
                               bool p_flip_v)
{
    m_local_bounds = p_local_bounds;
    m_flip_u = p_flip_u;
    m_flip_v = p_flip_v;
}

void CanvasTexture::clear(Color p_fill)
{
    for (std::size_t i = 0u; i < m_pixels.size(); i += 4u)
    {
        m_pixels[i + 0u] = p_fill.r;
        m_pixels[i + 1u] = p_fill.g;
        m_pixels[i + 2u] = p_fill.b;
        m_pixels[i + 3u] = p_fill.a;
    }
    m_dirty = true;
    m_dirty_all = true;
}

Status CanvasTexture::bind(AssetManager& p_assets, std::string p_name)
{
    if ((m_width == 0u) || (m_height == 0u))
    {
        return failure("CanvasTexture::bind before create()");
    }

    TextureAsset picture;
    picture.name = p_name;
    picture.description =
        compages::gpu::TextureDesc::image(m_width, m_height)
            .filter(compages::gpu::Filter::Nearest)
            .wrap(compages::gpu::Wrap::ClampToEdge);
    picture.pixels.resize(m_pixels.size());
    std::memcpy(picture.pixels.data(), m_pixels.data(), m_pixels.size());

    const TextureAssetId existing = p_assets.findTexture(p_name);
    if (existing.valid())
    {
        TextureAsset* slot = p_assets.texture(existing);
        if (slot == nullptr)
        {
            return failure("CanvasTexture: stale texture name '" + p_name + "'");
        }
        if (!slot->texture.valid())
        {
            COMPAGES_TRY(slot->texture.allocate(picture.description));
        }
        COMPAGES_TRY(slot->texture.write(
            std::span<const std::byte>(reinterpret_cast<std::byte const*>(
                                           picture.pixels.data()),
                                       picture.pixels.size())));
        slot->pixels = std::move(picture.pixels);
        slot->description = picture.description;
        m_assets = &p_assets;
        m_texture_id = existing;
        m_asset_name = std::move(p_name);
        m_dirty = false;
        m_dirty_all = false;
        return success();
    }

    COMPAGES_TRY_ASSIGN(picture.texture,
                        compages::gpu::Texture::create(picture.description));
    COMPAGES_TRY(picture.texture.write(
        std::span<const std::byte>(
            reinterpret_cast<std::byte const*>(picture.pixels.data()),
            picture.pixels.size())));

    auto id = p_assets.addTexture(p_name, std::move(picture));
    if (!id)
    {
        return failure(id.error());
    }
    m_assets = &p_assets;
    m_texture_id = id.value();
    m_asset_name = std::move(p_name);
    m_dirty = false;
    m_dirty_all = false;
    return success();
}

bool CanvasTexture::localToPixel(compages::core::Vector3f p_local,
                                 int& p_px,
                                 int& p_py) const
{
    const float width = m_local_bounds.max.x - m_local_bounds.min.x;
    const float height = m_local_bounds.max.y - m_local_bounds.min.y;
    if ((width <= 0.0f) || (height <= 0.0f))
    {
        return false;
    }

    float u = (p_local.x - m_local_bounds.min.x) / width;
    float v = (p_local.y - m_local_bounds.min.y) / height;
    if (m_flip_u)
    {
        u = 1.0f - u;
    }
    if (m_flip_v)
    {
        v = 1.0f - v;
    }
    if ((u < 0.0f) || (u > 1.0f) || (v < 0.0f) || (v > 1.0f))
    {
        return false;
    }

    p_px = int(std::lround(u * float(m_width - 1u)));
    p_py = int(std::lround(v * float(m_height - 1u)));
    return true;
}

int CanvasTexture::worldRadiusToPixels(float p_radius_world,
                                       float p_world_extent_x) const
{
    if (p_world_extent_x <= 0.0f)
    {
        return 1;
    }
    const float pixels_per_world = float(m_width) / p_world_extent_x;
    return std::max(1, int(std::lround(p_radius_world * pixels_per_world)));
}

void CanvasTexture::stampDisk(int p_center_x,
                              int p_center_y,
                              int p_pixel_radius,
                              Color p_color)
{
    if (m_pixels.empty())
    {
        return;
    }

    const int r2 = p_pixel_radius * p_pixel_radius;
    const int x0 = std::max(0, p_center_x - p_pixel_radius);
    const int x1 = std::min(int(m_width) - 1, p_center_x + p_pixel_radius);
    const int y0 = std::max(0, p_center_y - p_pixel_radius);
    const int y1 = std::min(int(m_height) - 1, p_center_y + p_pixel_radius);

    for (int y = y0; y <= y1; ++y)
    {
        for (int x = x0; x <= x1; ++x)
        {
            const int dx = x - p_center_x;
            const int dy = y - p_center_y;
            if ((dx * dx + dy * dy) > r2)
            {
                continue;
            }
            const std::size_t offset = (std::size_t(y) * std::size_t(m_width) +
                                        std::size_t(x)) *
                                       4u;
            m_pixels[offset + 0u] = p_color.r;
            m_pixels[offset + 1u] = p_color.g;
            m_pixels[offset + 2u] = p_color.b;
            m_pixels[offset + 3u] = p_color.a;
        }
    }

    markDirty(std::uint32_t(x0), std::uint32_t(y0), std::uint32_t(x1),
              std::uint32_t(y1));
}

void CanvasTexture::markDirty(std::uint32_t p_x0,
                              std::uint32_t p_y0,
                              std::uint32_t p_x1,
                              std::uint32_t p_y1)
{
    if (m_dirty_all)
    {
        m_dirty = true;
        return;
    }
    if (!m_dirty)
    {
        m_dirty = true;
        m_dirty_x0 = p_x0;
        m_dirty_y0 = p_y0;
        m_dirty_x1 = p_x1;
        m_dirty_y1 = p_y1;
        return;
    }
    m_dirty_x0 = std::min(m_dirty_x0, p_x0);
    m_dirty_y0 = std::min(m_dirty_y0, p_y0);
    m_dirty_x1 = std::max(m_dirty_x1, p_x1);
    m_dirty_y1 = std::max(m_dirty_y1, p_y1);
}

void CanvasTexture::paintLocal(compages::core::Vector3f p_local_point,
                               Color p_color,
                               float p_brush_radius_local)
{
    int center_x = 0;
    int center_y = 0;
    if (!localToPixel(p_local_point, center_x, center_y))
    {
        return;
    }
    const float width = m_local_bounds.max.x - m_local_bounds.min.x;
    const int pixel_radius =
        worldRadiusToPixels(p_brush_radius_local, width);
    stampDisk(center_x, center_y, pixel_radius, p_color);
}

void CanvasTexture::paintWorld(compages::core::Vector3f p_world_point,
                               compages::core::Matrix44f const& p_canvas_world,
                               Color p_color,
                               float p_brush_radius_world)
{
    const compages::core::Matrix44f local_matrix =
        compages::core::inverse(p_canvas_world);
    paintLocal(compages::core::transformPoint(local_matrix, p_world_point),
               p_color,
               p_brush_radius_world);
}

void CanvasTexture::paintLineWorld(compages::core::Vector3f p_from,
                                   compages::core::Vector3f p_to,
                                   compages::core::Matrix44f const& p_canvas_world,
                                   Color p_color,
                                   float p_brush_radius_world)
{
    const float spacing = std::max(0.001f, p_brush_radius_world * 0.5f);
    const compages::core::Vector3f delta = p_to - p_from;
    const float span = std::sqrt(delta.x * delta.x + delta.y * delta.y +
                                 delta.z * delta.z);
    const int steps = std::max(1, int(std::ceil(span / spacing)));
    for (int i = 0; i <= steps; ++i)
    {
        const float t = float(i) / float(steps);
        const compages::core::Vector3f at{
            p_from.x + (p_to.x - p_from.x) * t,
            p_from.y + (p_to.y - p_from.y) * t,
            p_from.z + (p_to.z - p_from.z) * t,
        };
        paintWorld(at, p_canvas_world, p_color, p_brush_radius_world);
    }
}

Status CanvasTexture::sync()
{
    if (!m_dirty)
    {
        return success();
    }
    if ((m_assets == nullptr) || !m_texture_id.valid())
    {
        return failure("CanvasTexture::sync before bind()");
    }

    TextureAsset* asset = m_assets->texture(m_texture_id);
    if (asset == nullptr)
    {
        return failure("CanvasTexture: texture asset is gone");
    }
    if (!asset->texture.valid())
    {
        COMPAGES_TRY(asset->texture.allocate(asset->description));
    }

    if (m_dirty_all)
    {
        COMPAGES_TRY(asset->texture.write(
            std::span<const std::byte>(
                reinterpret_cast<std::byte const*>(m_pixels.data()),
                m_pixels.size())));
        if (asset->pixels.size() != m_pixels.size())
        {
            asset->pixels.resize(m_pixels.size());
        }
        std::memcpy(asset->pixels.data(), m_pixels.data(), m_pixels.size());
        m_dirty = false;
        m_dirty_all = false;
        return success();
    }

    const std::uint32_t width = m_dirty_x1 - m_dirty_x0 + 1u;
    const std::uint32_t height = m_dirty_y1 - m_dirty_y0 + 1u;
    std::vector<std::byte> region(size_t(width) * size_t(height) * 4u);
    for (std::uint32_t y = 0u; y < height; ++y)
    {
        const std::size_t src = (std::size_t(m_dirty_y0 + y) *
                                 std::size_t(m_width) + std::size_t(m_dirty_x0)) *
                                4u;
        const std::size_t dst = std::size_t(y) * std::size_t(width) * 4u;
        std::memcpy(region.data() + dst, m_pixels.data() + src,
                    std::size_t(width) * 4u);
    }

    COMPAGES_TRY(asset->texture.write(m_dirty_x0,
                                      m_dirty_y0,
                                      0u,
                                      width,
                                      height,
                                      1u,
                                      region));
    if (asset->pixels.size() == m_pixels.size())
    {
        for (std::uint32_t y = 0u; y < height; ++y)
        {
            const std::size_t src = std::size_t(y) * std::size_t(width) * 4u;
            const std::size_t dst = (std::size_t(m_dirty_y0 + y) *
                                     std::size_t(m_width) +
                                     std::size_t(m_dirty_x0)) *
                                    4u;
            std::memcpy(asset->pixels.data() + dst, region.data() + src,
                        std::size_t(width) * 4u);
        }
    }

    m_dirty = false;
    m_dirty_all = false;
    return success();
}

void CanvasTexture::applyLook(Scene& p_scene, compages::world::EntityId p_entity)
{
    p_scene.look(p_entity, m_texture_id);
}

} // namespace compages::renderer
