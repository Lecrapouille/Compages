// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Internal/Pools.hpp"
#include "GPU/Internal/Statistics.hpp"

namespace compages::gpu
{

namespace
{

FrameStatistics g_frame;

//! \brief A byte count as a human reads it.
std::string readableBytes(std::size_t p_bytes)
{
    if (p_bytes < 1024u)
    {
        return std::to_string(p_bytes) + " B";
    }
    if (p_bytes < (1024u * 1024u))
    {
        return std::to_string(p_bytes / 1024u) + " kiB";
    }
    return std::to_string(p_bytes / (1024u * 1024u)) + " MiB";
}

} // namespace

namespace detail
{

void countDraw(std::size_t p_vertices, std::size_t p_instances)
{
    ++g_frame.draw_calls;
    g_frame.vertices += p_vertices;
    g_frame.instances += p_instances;
}

void countPass()
{
    ++g_frame.passes;
}

void countDispatch()
{
    ++g_frame.dispatches;
}

} // namespace detail

FrameStatistics const& frameStatistics()
{
    return g_frame;
}

void resetFrameStatistics()
{
    g_frame = FrameStatistics{};
}

ResourceStatistics resourceStatistics()
{
    ResourceStatistics counted;
    detail::Pools const& all = detail::pools();

    counted.buffers = all.buffers.size();
    counted.textures = all.textures.size();
    counted.shaders = all.shaders.size();
    counted.programs = all.programs.size();
    counted.pipelines = all.pipelines.size();
    counted.framebuffers = all.framebuffers.size();
    counted.vertex_readers = vertexReadersHeld();
    counted.buffer_bytes = detail::bufferMemory();
    counted.texture_bytes = textureMemory();

    return counted;
}

std::string ResourceStatistics::toString() const
{
    std::string text;

    text += std::to_string(buffers) + " buffer(s), " +
            readableBytes(buffer_bytes) + "\n";
    text += std::to_string(textures) + " texture(s), " +
            readableBytes(texture_bytes) + "\n";
    text += std::to_string(shaders) + " compiled shader(s)\n";
    text += std::to_string(programs) + " program(s)\n";
    text += std::to_string(pipelines) + " pipeline(s), sharing " +
            std::to_string(vertex_readers) + " way(s) of reading a vertex\n";
    text += std::to_string(framebuffers) + " framebuffer(s)";

    return text;
}

} // namespace compages::gpu
