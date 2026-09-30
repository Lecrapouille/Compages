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

#include "Compages/GPU/Framebuffer.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Backends/Backend.hpp"
#include "GPU/Internal/Pools.hpp"

#include <algorithm>
#include <string>

namespace compages::gpu
{

namespace
{

constexpr std::uint32_t MAX_COLORS = 8u;

//------------------------------------------------------------------------------
//! \brief How large one mip level is, at least one pixel on a side.
//------------------------------------------------------------------------------
void sizeOfLevel(TextureDesc const& p_desc,
                 std::uint32_t p_level,
                 std::uint32_t& p_width,
                 std::uint32_t& p_height)
{
    p_width = std::max(1u, p_desc.width >> p_level);
    p_height = std::max(1u, p_desc.height >> p_level);
}

//------------------------------------------------------------------------------
Result<detail::TextureRecord const*>
checkedTexture(Attachment const& p_attachment, char const* p_role, bool p_depth)
{
    if (!p_attachment.texture)
    {
        return failure(std::string("the ") + p_role +
                       " attachment names no texture");
    }

    detail::TextureRecord const* record =
        detail::pools().textures.get(p_attachment.texture);
    if (record == nullptr)
    {
        return failure(std::string("the ") + p_role +
                       " attachment names a texture that has been released");
    }

    if (p_attachment.level >= record->desc.levels)
    {
        return failure(
            std::string("the ") + p_role + " attachment asks for mip level " +
            std::to_string(p_attachment.level) + " and the texture only has " +
            std::to_string(record->desc.levels));
    }

    if (record->desc.kind != TextureKind::Texture2D)
    {
        return failure(std::string("only a 2D texture can be attached for now, "
                                   "and the ") +
                       p_role + " attachment is a " +
                       toString(record->desc.kind));
    }

    const bool depth = isDepth(record->desc.format);
    if (p_depth && !depth)
    {
        return failure("the depth attachment holds " +
                       std::string(toString(record->desc.format)) +
                       ", which is a colour format. Use Depth16, Depth32F or "
                       "Depth24Stencil8");
    }
    if (!p_depth && depth)
    {
        return failure("a colour attachment holds " +
                       std::string(toString(record->desc.format)) +
                       ", which is a depth format. A colour attachment is what "
                       "the picture is written into");
    }

    return record;
}

} // namespace

//------------------------------------------------------------------------------
Result<Framebuffer> Framebuffer::create(std::span<const Attachment> p_colors,
                                        Attachment p_depth)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called");
    }
    if (p_colors.size() > MAX_COLORS)
    {
        return failure("a framebuffer can hold " + std::to_string(MAX_COLORS) +
                       " colour attachments, and " +
                       std::to_string(p_colors.size()) + " were given");
    }
    if (p_colors.empty() && !p_depth.texture)
    {
        return failure(
            "a framebuffer with no colour and no depth is not a target. Attach "
            "at least one texture");
    }

    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    std::vector<Attachment> colors;
    colors.reserve(p_colors.size());

    for (std::size_t i = 0u; i < p_colors.size(); ++i)
    {
        auto record_result = checkedTexture(p_colors[i], "colour", false);
        if (!record_result)
        {
            return failure(record_result.error());
        }
        auto record = record_result.take();
        std::uint32_t w = 0u;
        std::uint32_t h = 0u;
        sizeOfLevel(record->desc, p_colors[i].level, w, h);
        if (width == 0u)
        {
            width = w;
            height = h;
        }
        else if ((w != width) || (h != height))
        {
            return failure(
                "colour attachment " + std::to_string(i) + " is " +
                std::to_string(w) + " by " + std::to_string(h) +
                " and the first is " + std::to_string(width) + " by " +
                std::to_string(height) +
                ". Every attachment of a framebuffer has to be the same size");
        }
        colors.emplace_back(p_colors[i]);
    }

    Attachment depth;
    if (p_depth.texture)
    {
        auto record_result = checkedTexture(p_depth, "depth", true);
        if (!record_result)
        {
            return failure(record_result.error());
        }
        auto record = record_result.take();
        std::uint32_t w = 0u;
        std::uint32_t h = 0u;
        sizeOfLevel(record->desc, p_depth.level, w, h);
        if (width == 0u)
        {
            width = w;
            height = h;
        }
        else if ((w != width) || (h != height))
        {
            return failure(
                "the depth attachment is " + std::to_string(w) + " by " +
                std::to_string(h) + " and the colour attachments are " +
                std::to_string(width) + " by " + std::to_string(height));
        }
        depth = p_depth;
    }

    auto native_result = backend::createFramebuffer();
    if (!native_result)
    {
        return failure(native_result.error());
    }
    auto native = native_result.take();

    for (std::uint32_t i = 0u; i < colors.size(); ++i)
    {
        detail::TextureRecord const* record =
            detail::pools().textures.get(colors[i].texture);
        backend::attachColor(native, i, record->native, colors[i].level);
    }
    backend::setColorCount(native, static_cast<std::uint32_t>(colors.size()));

    if (depth.texture)
    {
        detail::TextureRecord const* record =
            detail::pools().textures.get(depth.texture);
        backend::attachDepth(native,
                             record->native,
                             depth.level,
                             hasStencil(record->desc.format));
    }

    if (auto ready = backend::checkFramebuffer(native); !ready)
    {
        backend::destroyFramebuffer(native);
        return failure(ready.error());
    }

    detail::FramebufferRecord stored;
    stored.native = native;
    stored.width = width;
    stored.height = height;
    stored.colors = std::move(colors);
    stored.depth = depth;

    auto added = detail::pools().framebuffers.add(std::move(stored));
    if (!added)
    {
        backend::destroyFramebuffer(native);
        return failure(added.error());
    }
    return Framebuffer(added.take());
}

//------------------------------------------------------------------------------
Result<Framebuffer> Framebuffer::create(Texture const& p_color)
{
    Attachment color{ p_color.handle(), 0u };
    return create(std::span<const Attachment>(&color, 1u));
}

//------------------------------------------------------------------------------
Result<Framebuffer> Framebuffer::create(Texture const& p_color,
                                        Texture const& p_depth)
{
    Attachment color{ p_color.handle(), 0u };
    return create(std::span<const Attachment>(&color, 1u),
                  Attachment{ p_depth.handle(), 0u });
}

//------------------------------------------------------------------------------
Status Framebuffer::attach(std::span<const Attachment> p_colors,
                           Attachment p_depth)
{
    COMPAGES_TRY_ASSIGN(*this, create(p_colors, p_depth));
    return success();
}

//------------------------------------------------------------------------------
Status Framebuffer::attach(Texture const& p_color)
{
    COMPAGES_TRY_ASSIGN(*this, create(p_color));
    return success();
}

//------------------------------------------------------------------------------
Status Framebuffer::attach(Texture const& p_color, Texture const& p_depth)
{
    COMPAGES_TRY_ASSIGN(*this, create(p_color, p_depth));
    return success();
}

//------------------------------------------------------------------------------
Framebuffer::Framebuffer(Framebuffer&& p_other) noexcept
    : m_handle(p_other.m_handle)
{
    p_other.m_handle = FramebufferHandle{};
}

//------------------------------------------------------------------------------
Framebuffer& Framebuffer::operator=(Framebuffer&& p_other) noexcept
{
    if (this != &p_other)
    {
        release();
        m_handle = p_other.m_handle;
        p_other.m_handle = FramebufferHandle{};
    }
    return *this;
}

//------------------------------------------------------------------------------
Framebuffer::~Framebuffer()
{
    release();
}

//------------------------------------------------------------------------------
void Framebuffer::release()
{
    if (!m_handle)
    {
        return;
    }
    if (detail::FramebufferRecord* record =
            detail::pools().framebuffers.get(m_handle))
    {
        backend::destroyFramebuffer(record->native);
        (void)detail::pools().framebuffers.remove(m_handle);
    }
    m_handle = FramebufferHandle{};
}

//------------------------------------------------------------------------------
bool Framebuffer::valid() const
{
    return detail::pools().framebuffers.get(m_handle) != nullptr;
}

//------------------------------------------------------------------------------
std::uint32_t Framebuffer::width() const
{
    detail::FramebufferRecord const* record =
        detail::pools().framebuffers.get(m_handle);
    return (record != nullptr) ? record->width : 0u;
}

//------------------------------------------------------------------------------
std::uint32_t Framebuffer::height() const
{
    detail::FramebufferRecord const* record =
        detail::pools().framebuffers.get(m_handle);
    return (record != nullptr) ? record->height : 0u;
}

//------------------------------------------------------------------------------
std::size_t Framebuffer::colorCount() const
{
    detail::FramebufferRecord const* record =
        detail::pools().framebuffers.get(m_handle);
    return (record != nullptr) ? record->colors.size() : 0u;
}

//------------------------------------------------------------------------------
TextureHandle Framebuffer::color(std::size_t p_index) const
{
    detail::FramebufferRecord const* record =
        detail::pools().framebuffers.get(m_handle);
    if ((record == nullptr) || (p_index >= record->colors.size()))
    {
        return {};
    }
    return record->colors[p_index].texture;
}

//------------------------------------------------------------------------------
TextureHandle Framebuffer::depth() const
{
    detail::FramebufferRecord const* record =
        detail::pools().framebuffers.get(m_handle);
    return (record != nullptr) ? record->depth.texture : TextureHandle{};
}

//------------------------------------------------------------------------------
bool Framebuffer::hasDepth() const
{
    return static_cast<bool>(depth());
}

//------------------------------------------------------------------------------
std::size_t liveFramebuffers()
{
    return detail::pools().framebuffers.size();
}

} // namespace compages::gpu
