// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/RenderPass.hpp"
#include "GPU/Backends/Backend.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Internal/Pools.hpp"
#include "GPU/Internal/Statistics.hpp"
#include "Compages/GPU/Errors.hpp"

#include <vector>

namespace compages::gpu
{

namespace
{

//! \brief One pass open, and the target it draws into.
struct OpenPass
{
    PassDesc desc;
    backend::NativeId target = 0u;
};

//! \brief The passes open, innermost last.
//!
//! The library knows which pass is drawing. That is what lets a draw call say
//! "no pass is open" rather than drawing into whatever the driver happened to
//! have bound, which is how a frame ends up in the wrong place with no error at
//! all.
std::vector<OpenPass> g_open;

//! \brief Bind the innermost pass again, with the clears p_desc asks for.
void rebind(PassDesc const& p_desc)
{
    backend::beginPass(p_desc, g_open.back().target);
}

} // namespace

Result<RenderPass> RenderPass::begin(PassDesc const& p_given)
{
    PassDesc p_desc = p_given;
    if (!p_desc.target && !g_open.empty())
    {
        p_desc.target = g_open.back().desc.target;
    }

    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is nothing to "
                       "draw into");
    }
    if ((p_desc.width == 0u) || (p_desc.height == 0u))
    {
        return failure(
            "a pass of " + std::to_string(p_desc.width) + " by " +
            std::to_string(p_desc.height) +
            " pixels draws nothing at all. A size of zero usually means the "
            "window size was read before the window was shown, or that the window "
            "is minimised, in which case the frame is worth skipping entirely");
    }

    backend::NativeId target = 0u;
    if (p_desc.target)
    {
        detail::FramebufferRecord const* record =
            detail::pools().framebuffers.get(p_desc.target);
        if (record == nullptr)
        {
            return failure(
                "the pass names a framebuffer that has been released, so there "
                "is nowhere to draw. The empty handle is the window; a stale "
                "one is not");
        }
        if ((p_desc.x + p_desc.width > record->width) ||
            (p_desc.y + p_desc.height > record->height))
        {
            return failure(
                "the pass is " + std::to_string(p_desc.width) + " by " +
                std::to_string(p_desc.height) + " at " +
                std::to_string(p_desc.x) + ", " + std::to_string(p_desc.y) +
                " and the framebuffer is " + std::to_string(record->width) +
                " by " + std::to_string(record->height));
        }
        target = record->native;
    }

    backend::beginPass(p_desc, target);

    g_open.emplace_back(OpenPass{ p_desc, target });
    detail::countPass();

    return RenderPass(p_desc, g_open.size());
}

RenderPass::RenderPass(PassDesc const& p_desc)
{
    auto opened = begin(p_desc);
    if (!opened)
    {
        reportError(opened.error());
        return;
    }
    *this = opened.take();
}

RenderPass::RenderPass(Framebuffer const& p_target, PassDesc p_desc)
{
    p_desc.target = p_target.handle();
    if (p_desc.width == 0u)
    {
        p_desc.width = p_target.width() - p_desc.x;
    }
    if (p_desc.height == 0u)
    {
        p_desc.height = p_target.height() - p_desc.y;
    }
    auto opened = begin(p_desc);
    if (!opened)
    {
        reportError(opened.error());
        return;
    }
    *this = opened.take();
}

RenderPass::RenderPass(RenderPass&& p_other) noexcept
    : m_desc(p_other.m_desc), m_depth(p_other.m_depth), m_open(p_other.m_open)
{
    p_other.m_open = false;
}

RenderPass& RenderPass::operator=(RenderPass&& p_other) noexcept
{
    if (this != &p_other)
    {
        end();
        m_desc = p_other.m_desc;
        m_depth = p_other.m_depth;
        m_open = p_other.m_open;
        p_other.m_open = false;
    }
    return *this;
}

RenderPass::~RenderPass()
{
    end();
}

void RenderPass::end()
{
    if (!m_open)
    {
        return;
    }
    m_open = false;

    // Passes opened over this one and still open are closed with it: drawing
    // into them after this point would be drawing into a target nobody holds.
    if (g_open.size() >= m_depth)
    {
        g_open.resize(m_depth - 1u);
    }
    if (!initialized())
    {
        g_open.clear();
        return;
    }
    backend::endPass();

    if (!g_open.empty())
    {
        PassDesc resumed = g_open.back().desc;
        resumed.clear_color = false;
        resumed.clear_depth = false;
        rebind(resumed);
    }
}

bool inRenderPass()
{
    return !g_open.empty();
}

PassDesc const& currentPass()
{
    static const PassDesc nothing;
    return g_open.empty() ? nothing : g_open.back().desc;
}

void clear(Vector4f const& p_color)
{
    if (g_open.empty())
    {
        return reportError("compages::gpu::clear() with no pass open: there is no "
                           "target to clear");
    }
    PassDesc desc = g_open.back().desc;
    desc.clear_color = true;
    desc.color = p_color;
    desc.clear_depth = false;
    rebind(desc);
}

void clear(Vector3f const& p_color)
{
    clear(Vector4f(p_color[0], p_color[1], p_color[2], 1.0f));
}

void clear(std::initializer_list<float> p_color)
{
    if ((p_color.size() != 3u) && (p_color.size() != 4u))
    {
        return reportError("compages::gpu::clear() takes a colour of three numbers, or "
                           "four with the alpha");
    }
    const float* c = p_color.begin();
    clear(Vector4f(c[0], c[1], c[2], (p_color.size() == 4u) ? c[3] : 1.0f));
}

void clearDepth(float p_depth)
{
    if (g_open.empty())
    {
        return reportError("compages::gpu::clearDepth() with no pass open: there is no "
                           "target to clear");
    }
    PassDesc desc = g_open.back().desc;
    desc.clear_color = false;
    desc.clear_depth = true;
    desc.depth = p_depth;
    rebind(desc);
}

void clear(Vector3f const& p_color, float p_depth)
{
    clear(p_color);
    clearDepth(p_depth);
}

Result<std::vector<std::byte>> readPixels(std::uint32_t p_x,
                                          std::uint32_t p_y,
                                          std::uint32_t p_width,
                                          std::uint32_t p_height)
{
    if (g_open.empty())
    {
        return failure(
            "there is no pass open, so there is no target to read from. Read the "
            "picture before the pass goes out of scope");
    }

    PassDesc const& current = g_open.back().desc;
    const std::uint32_t width = (p_width == 0u) ? current.width : p_width;
    const std::uint32_t height = (p_height == 0u) ? current.height : p_height;

    if ((p_x + width > current.width) || (p_y + height > current.height))
    {
        return failure("the region being read runs outside the pass: " +
                       std::to_string(width) + " by " + std::to_string(height) +
                       " at " + std::to_string(p_x) + ", " + std::to_string(p_y) +
                       " does not fit in " + std::to_string(current.width) +
                       " by " + std::to_string(current.height));
    }

    std::vector<std::byte> pixels(static_cast<std::size_t>(width) * height * 4u);
    backend::readTargetPixels(
        current.x + p_x, current.y + p_y, width, height, pixels.data());
    return pixels;
}

} // namespace compages::gpu
