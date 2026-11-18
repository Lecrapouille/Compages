// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/Backend.hpp"
#include "GPU/Internal/Pools.hpp"
#include "GPU/Internal/Statistics.hpp"

#include <array>
#include <cstdint>

namespace compages::gpu
{

namespace
{

FrameStatistics g_frame;

//! \brief The GPU timer: a ring of frames, two timestamps each (begin, end),
//! read when the GPU has passed them.
struct GpuTimer
{
    static constexpr std::uint32_t FRAMES = backend::TIMESTAMP_SLOTS / 2u;
    std::array<bool, FRAMES> pending{};
    std::uint32_t current = 0u;
    bool open = false;
    double last = -1.0;
};
GpuTimer g_timer;

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

void beginGpuTimer()
{
    if (g_timer.open)
    {
        return;
    }
    // Read the frames the GPU has finished, oldest first.
    for (std::uint32_t k = 1u; k <= GpuTimer::FRAMES; ++k)
    {
        const std::uint32_t frame = (g_timer.current + k) % GpuTimer::FRAMES;
        std::uint64_t begin = 0u;
        std::uint64_t end = 0u;
        if (g_timer.pending[frame] && backend::readTimestamp(2u * frame + 1u, end) &&
            backend::readTimestamp(2u * frame, begin))
        {
            g_timer.last = double(end - begin) * 1.0e-6;
            g_timer.pending[frame] = false;
        }
    }
    // The GPU is a whole ring behind: this frame is not measured again.
    g_timer.current = (g_timer.current + 1u) % GpuTimer::FRAMES;
    g_timer.pending[g_timer.current] = false;
    backend::writeTimestamp(2u * g_timer.current);
    g_timer.open = true;
}

void endGpuTimer()
{
    if (!g_timer.open)
    {
        return;
    }
    backend::writeTimestamp(2u * g_timer.current + 1u);
    g_timer.pending[g_timer.current] = true;
    g_timer.open = false;
}

double gpuMilliseconds()
{
    return g_timer.last;
}

namespace detail
{

void forgetGpuTimer()
{
    g_timer = GpuTimer{};
}

} // namespace detail

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
    counted.reads = all.reads.size();
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
    text += std::to_string(framebuffers) + " framebuffer(s)\n";
    text += std::to_string(reads) + " read(s) on their way to the CPU";

    return text;
}

} // namespace compages::gpu
