// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/Renderer/Render/RenderTarget.hpp"

#include <algorithm>
#include <cstring>
#include <limits>
#include <string>

namespace compages::renderer
{

// ----------------------------------------------------------------------------
Result<RenderTarget> RenderTarget::create(std::uint32_t p_width, std::uint32_t p_height)
{
    RenderTarget target;
    COMPAGES_TRY(target.resize(p_width, p_height));
    return target;
}

// ----------------------------------------------------------------------------
Status RenderTarget::resize(std::uint32_t p_width, std::uint32_t p_height)
{
    if (valid() && (p_width == m_width) && (p_height == m_height))
    {
        return success();
    }
    *this = RenderTarget{};
    if ((p_width == 0u) || (p_height == 0u))
    {
        return failure("a RenderTarget needs at least one pixel, not " + std::to_string(p_width) + " x " +
                       std::to_string(p_height));
    }
    // Depth32F: the Scene draws with the depth reversed, precise from a
    // metre to the Moon.
    COMPAGES_TRY_ASSIGN(m_color, compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::RGBA8,
                                                                  .width = p_width,
                                                                  .height = p_height }));
    COMPAGES_TRY_ASSIGN(m_depth, compages::gpu::Texture::create({ .format = compages::gpu::PixelFormat::Depth32F,
                                                                  .width = p_width,
                                                                  .height = p_height,
                                                                  .magnify = compages::gpu::Filter::Nearest,
                                                                  .minify = compages::gpu::Filter::Nearest }));
    COMPAGES_TRY_ASSIGN(m_framebuffer, compages::gpu::Framebuffer::create(m_color, m_depth));
    m_width = p_width;
    m_height = p_height;
    return success();
}

// ----------------------------------------------------------------------------
//! The GPU gives the bottom row first: the rows are turned over here, once.
// ----------------------------------------------------------------------------
Result<std::vector<std::byte>> RenderTarget::readColor() const
{
    if (!valid())
    {
        return failure("readColor() on an empty RenderTarget: create() it first");
    }
    std::vector<std::byte> pixels;
    COMPAGES_TRY_ASSIGN(pixels, m_color.read());
    const std::size_t row = std::size_t(m_width) * 4u;
    for (std::size_t top = 0u, bottom = std::size_t(m_height) - 1u; top < bottom; ++top, --bottom)
    {
        std::swap_ranges(pixels.begin() + std::ptrdiff_t(top * row), pixels.begin() + std::ptrdiff_t((top + 1u) * row),
                         pixels.begin() + std::ptrdiff_t(bottom * row));
    }
    return pixels;
}

// ----------------------------------------------------------------------------
//! The Scene stores n (f - d) / ((f - n) d) for a perspective lens (1 at the
//! near plane, 0 at the far one) and (f - d) / (f - n) for an orthographic
//! one: solved here for d, the distance along the axis of the camera.
// ----------------------------------------------------------------------------
Result<std::vector<float>> RenderTarget::readDepth(compages::world::Camera const& p_lens) const
{
    if (!valid())
    {
        return failure("readDepth() on an empty RenderTarget: create() it first");
    }
    std::vector<std::byte> bytes;
    COMPAGES_TRY_ASSIGN(bytes, m_depth.read());
    const std::size_t count = std::size_t(m_width) * m_height;
    if (bytes.size() < count * sizeof(float))
    {
        return failure("readDepth(): the GPU gave " + std::to_string(bytes.size()) + " bytes for " +
                       std::to_string(count) + " pixels");
    }
    const float n = p_lens.near_plane;
    const float f = p_lens.far_plane;
    const bool perspective = (p_lens.projection == compages::world::Camera::Projection::Perspective);
    std::vector<float> metres(count);
    for (std::size_t row = 0u; row < m_height; ++row)
    {
        // The GPU gives the bottom row first.
        const std::size_t from = (std::size_t(m_height) - 1u - row) * m_width;
        for (std::size_t x = 0u; x < m_width; ++x)
        {
            float stored = 0.0f;
            std::memcpy(&stored, bytes.data() + ((from + x) * sizeof(float)), sizeof(float));
            float& out = metres[(row * m_width) + x];
            if (stored <= 0.0f)
            {
                out = std::numeric_limits<float>::infinity();
            }
            else
            {
                out = perspective ? ((n * f) / ((stored * (f - n)) + n)) : (f - (stored * (f - n)));
            }
        }
    }
    return metres;
}

// ----------------------------------------------------------------------------
Status RenderTarget::saveColor(std::filesystem::path const& p_path) const
{
    if (!valid())
    {
        return failure("saveColor() on an empty RenderTarget: create() it first");
    }
    return m_color.saveFile(p_path);
}

} // namespace compages::renderer
