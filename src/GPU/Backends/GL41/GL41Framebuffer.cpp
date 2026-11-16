// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <array>
#include <string>

namespace compages::gpu::backend
{

namespace
{

// ****************************************************************************
//! \brief Bind a framebuffer to configure it, and put back the one bound
//! before, so that a target made between two draws does not become theirs.
// ****************************************************************************
class Configuring
{
public:

    explicit Configuring(NativeId p_framebuffer)
    {
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_draw);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &m_read);
        glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(p_framebuffer));
    }

    ~Configuring()
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(m_draw));
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(m_read));
    }

    Configuring(Configuring const&) = delete;
    Configuring& operator=(Configuring const&) = delete;

private:

    GLint m_draw = 0;
    GLint m_read = 0;
};

void attach(GLenum p_point, NativeId p_texture, std::uint32_t p_level)
{
    const GLuint texture = detail::textureName(p_texture);
    const GLenum target = detail::textureTarget(p_texture);
    const auto level = static_cast<GLint>(p_level);

    if (target == GL_TEXTURE_2D)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, p_point, GL_TEXTURE_2D, texture, level);
        return;
    }
#if defined(COMPAGES_GLES3)
    // No layered attachment on ES 3.0: the first face or layer.
    if (target == GL_TEXTURE_CUBE_MAP)
    {
        glFramebufferTexture2D(GL_FRAMEBUFFER, p_point, GL_TEXTURE_CUBE_MAP_POSITIVE_X, texture, level);
        return;
    }
    glFramebufferTextureLayer(GL_FRAMEBUFFER, p_point, texture, level, 0);
#else
    // Every layer at once, as the GL45 backend does.
    glFramebufferTexture(GL_FRAMEBUFFER, p_point, texture, level);
#endif
}

} // namespace

Result<NativeId> createFramebuffer()
{
    GLuint name = 0u;
    glGenFramebuffers(1, &name);
    if (name == 0u)
    {
        return failure("the driver refused to create a framebuffer");
    }
    return static_cast<NativeId>(name);
}

void attachColor(NativeId p_framebuffer,
                 std::uint32_t p_index,
                 NativeId p_texture,
                 std::uint32_t p_level)
{
    Configuring bound(p_framebuffer);
    attach(GL_COLOR_ATTACHMENT0 + p_index, p_texture, p_level);
}

void attachDepth(NativeId p_framebuffer,
                 NativeId p_texture,
                 std::uint32_t p_level,
                 bool p_stencil)
{
    Configuring bound(p_framebuffer);
    attach(p_stencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT, p_texture, p_level);
}

void setColorCount(NativeId p_framebuffer, std::uint32_t p_count)
{
    Configuring bound(p_framebuffer);
    if (p_count == 0u)
    {
        // A depth-only target: no colour to write, none to read.
        const GLenum none = GL_NONE;
        glDrawBuffers(1, &none);
        glReadBuffer(GL_NONE);
        return;
    }

    std::array<GLenum, 8> points{};
    for (std::uint32_t i = 0u; i < p_count; ++i)
    {
        points[i] = GL_COLOR_ATTACHMENT0 + i;
    }
    glDrawBuffers(static_cast<GLsizei>(p_count), points.data());
    glReadBuffer(GL_COLOR_ATTACHMENT0);
}

Status checkFramebuffer(NativeId p_framebuffer)
{
    GLenum status = GL_FRAMEBUFFER_COMPLETE;
    {
        Configuring bound(p_framebuffer);
        status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    }
    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        return success();
    }

    const char* why = "for a reason the driver did not name";
    switch (status)
    {
        case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
            why = "an attachment is missing, is the wrong format, or is incomplete itself";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
            why = "nothing is attached";
            break;
        case GL_FRAMEBUFFER_UNSUPPORTED:
            why = "this combination of attachments is a combination the driver refuses (on OpenGL "
                  "ES, a float colour target needs EXT_color_buffer_float)";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
            why = "the attachments do not agree on how many samples they hold";
            break;
#if !defined(COMPAGES_GLES3)
        case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
            why = "a draw buffer points at an attachment that is not there";
            break;
        case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
            why = "the read buffer points at an attachment that is not there";
            break;
#endif
        default:
            break;
    }
    return failure("the framebuffer is incomplete: " + std::string(why));
}

void destroyFramebuffer(NativeId p_framebuffer)
{
    auto name = static_cast<GLuint>(p_framebuffer);
    glDeleteFramebuffers(1, &name);
}

} // namespace compages::gpu::backend
