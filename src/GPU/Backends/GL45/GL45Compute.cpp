// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL45/GL45.hpp"

namespace compages::gpu::backend
{

void bindStorageBlock(NativeId p_program, int p_block_index, int p_binding)
{
    glShaderStorageBlockBinding(static_cast<GLuint>(p_program),
                                static_cast<GLuint>(p_block_index),
                                static_cast<GLuint>(p_binding));
}

void dispatch(std::uint32_t p_groups_x,
              std::uint32_t p_groups_y,
              std::uint32_t p_groups_z)
{
    glDispatchCompute(p_groups_x, p_groups_y, p_groups_z);
}

void memoryBarrier(Barrier p_what)
{
    if (p_what == Barrier::All)
    {
        glMemoryBarrier(GL_ALL_BARRIER_BITS);
        return;
    }

    GLbitfield bits = 0u;
    const auto has = [p_what](Barrier p_one) {
        return (static_cast<std::uint32_t>(p_what) &
                static_cast<std::uint32_t>(p_one)) != 0u;
    };

    if (has(Barrier::VertexAttrib))
    {
        bits |= GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT;
    }
    if (has(Barrier::Index))
    {
        bits |= GL_ELEMENT_ARRAY_BARRIER_BIT;
    }
    if (has(Barrier::Uniform))
    {
        bits |= GL_UNIFORM_BARRIER_BIT;
    }
    if (has(Barrier::TextureFetch))
    {
        bits |= GL_TEXTURE_FETCH_BARRIER_BIT;
    }
    if (has(Barrier::Image))
    {
        bits |= GL_SHADER_IMAGE_ACCESS_BARRIER_BIT;
    }
    if (has(Barrier::Storage))
    {
        bits |= GL_SHADER_STORAGE_BARRIER_BIT;
    }
    if (has(Barrier::Framebuffer))
    {
        bits |= GL_FRAMEBUFFER_BARRIER_BIT;
    }
    if (has(Barrier::Command))
    {
        bits |= GL_COMMAND_BARRIER_BIT;
    }

    if (bits != 0u)
    {
        glMemoryBarrier(bits);
    }
}

} // namespace compages::gpu::backend
