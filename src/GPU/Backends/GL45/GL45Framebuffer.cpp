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

#include "GPU/Backends/GL45/GL45.hpp"

#include <array>
#include <string>

namespace compages::gpu::backend
{

//------------------------------------------------------------------------------
Result<NativeId> createFramebuffer()
{
    GLuint name = 0u;
    glCreateFramebuffers(1, &name);
    if (name == 0u)
    {
        return failure("the driver refused to create a framebuffer");
    }
    return static_cast<NativeId>(name);
}

//------------------------------------------------------------------------------
void attachColor(NativeId p_framebuffer,
                 std::uint32_t p_index,
                 NativeId p_texture,
                 std::uint32_t p_level)
{
    glNamedFramebufferTexture(static_cast<GLuint>(p_framebuffer),
                              GL_COLOR_ATTACHMENT0 + p_index,
                              static_cast<GLuint>(p_texture),
                              static_cast<GLint>(p_level));
}

//------------------------------------------------------------------------------
void attachDepth(NativeId p_framebuffer,
                 NativeId p_texture,
                 std::uint32_t p_level,
                 bool p_stencil)
{
    const GLenum point =
        p_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
    glNamedFramebufferTexture(static_cast<GLuint>(p_framebuffer),
                              point,
                              static_cast<GLuint>(p_texture),
                              static_cast<GLint>(p_level));
}

//------------------------------------------------------------------------------
void setColorCount(NativeId p_framebuffer, std::uint32_t p_count)
{
    if (p_count == 0u)
    {
        // A depth-only target writes no colour. The driver has to be told so,
        // or it considers the framebuffer incomplete.
        glNamedFramebufferDrawBuffer(static_cast<GLuint>(p_framebuffer),
                                     GL_NONE);
        glNamedFramebufferReadBuffer(static_cast<GLuint>(p_framebuffer),
                                     GL_NONE);
        return;
    }

    std::array<GLenum, 8> points{};
    for (std::uint32_t i = 0u; i < p_count; ++i)
    {
        points[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glNamedFramebufferDrawBuffers(static_cast<GLuint>(p_framebuffer),
                                  static_cast<GLsizei>(p_count),
                                  points.data());
}

//------------------------------------------------------------------------------
Status checkFramebuffer(NativeId p_framebuffer)
{
    const GLenum status = glCheckNamedFramebufferStatus(
        static_cast<GLuint>(p_framebuffer), GL_FRAMEBUFFER);
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        return success();
    }

    const char* why = "for a reason the driver did not name";
    switch (status)
    {
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        why = "an attachment is missing, is the wrong format, or is incomplete "
              "itself";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        why = "nothing is attached";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
        why = "a draw buffer points at an attachment that is not there";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
        why = "the read buffer points at an attachment that is not there";
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        why = "this combination of attachments is a combination the driver "
              "refuses";
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
        why = "the attachments do not agree on how many samples they hold";
        break;
    default:
        break;
    }

    return failure("the framebuffer is incomplete: " + std::string(why));
}

//------------------------------------------------------------------------------
void destroyFramebuffer(NativeId p_framebuffer)
{
    auto name = static_cast<GLuint>(p_framebuffer);
    glDeleteFramebuffers(1, &name);
}

} // namespace compages::gpu::backend
