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

#include "GPU/Internal/Pools.hpp"

#include <string>

namespace compages::gpu::detail
{

//------------------------------------------------------------------------------
Pools& pools()
{
    static Pools instance;
    return instance;
}

//------------------------------------------------------------------------------
namespace
{

//------------------------------------------------------------------------------
//! \brief Say what had been forgotten, in words naming what to look for.
//------------------------------------------------------------------------------
void reportLeak(std::size_t p_count, const char* p_what)
{
    if (p_count == 0u)
    {
        return;
    }
    log(LogLevel::Warning,
        std::to_string(p_count) + " " + p_what +
            " were still alive when the device shut down. They have been freed, "
            "but something held on to them: an object that outlives "
            "compages::gpu::shutdown(), or one stored in a container that is never "
            "cleared");
}

} // namespace

//------------------------------------------------------------------------------
void releaseAllResources()
{
    Pools& all = pools();

    reportLeak(all.buffers.size(), "buffer(s)");
    reportLeak(all.shaders.size(), "compiled shader(s)");
    reportLeak(all.programs.size(), "program(s)");
    reportLeak(all.textures.size(), "texture(s)");
    reportLeak(all.pipelines.size(), "pipeline(s)");
    reportLeak(all.framebuffers.size(), "framebuffer(s)");

    // Pipelines go first, because each holds a share of a way of reading a
    // vertex, and the backend only lets go of one when its last holder does.
    all.pipelines.forEach([](PipelineHandle, PipelineRecord const& p_record) {
        backend::releaseVertexReader(p_record.reader);
    });
    all.pipelines.clear();

    all.framebuffers.forEach(
        [](FramebufferHandle, FramebufferRecord const& p_record) {
            backend::destroyFramebuffer(p_record.native);
        });
    all.framebuffers.clear();

    all.textures.forEach([](TextureHandle, TextureRecord const& p_record) {
        backend::destroyTexture(p_record.native);
    });
    all.textures.clear();

    all.programs.forEach([](ProgramHandle, ProgramRecord const& p_record) {
        backend::destroyProgram(p_record.native);
    });
    all.programs.clear();

    all.shaders.forEach([](ShaderHandle, ShaderRecord const& p_record) {
        backend::destroyShader(p_record.native);
    });
    all.shaders.clear();

    all.buffers.forEach([](BufferHandle, BufferRecord const& p_record) {
        backend::destroyBuffer(p_record.native);
    });
    all.buffers.clear();
}

} // namespace compages::gpu::detail
