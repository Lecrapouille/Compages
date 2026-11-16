// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <algorithm>

namespace compages::gpu::backend
{

namespace
{

struct GLFormat
{
    GLenum internal_format;
    GLenum layout;
    GLenum type;
};

GLFormat toGL(PixelFormat p_format)
{
    switch (p_format)
    {
        case PixelFormat::R8:
            return { GL_R8, GL_RED, GL_UNSIGNED_BYTE };
        case PixelFormat::RG8:
            return { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };
        case PixelFormat::RGB8:
            return { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE };
        case PixelFormat::RGBA8:
            return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
        case PixelFormat::SRGB8:
            return { GL_SRGB8, GL_RGB, GL_UNSIGNED_BYTE };
        case PixelFormat::SRGB8A8:
            return { GL_SRGB8_ALPHA8, GL_RGBA, GL_UNSIGNED_BYTE };
        case PixelFormat::R8UI:
            return { GL_R8UI, GL_RED_INTEGER, GL_UNSIGNED_BYTE };
        case PixelFormat::R32I:
            return { GL_R32I, GL_RED_INTEGER, GL_INT };
        case PixelFormat::R32UI:
            return { GL_R32UI, GL_RED_INTEGER, GL_UNSIGNED_INT };
        case PixelFormat::R16F:
            return { GL_R16F, GL_RED, GL_HALF_FLOAT };
        case PixelFormat::RG16F:
            return { GL_RG16F, GL_RG, GL_HALF_FLOAT };
        case PixelFormat::RGBA16F:
            return { GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT };
        case PixelFormat::R32F:
            return { GL_R32F, GL_RED, GL_FLOAT };
        case PixelFormat::RG32F:
            return { GL_RG32F, GL_RG, GL_FLOAT };
        case PixelFormat::RGBA32F:
            return { GL_RGBA32F, GL_RGBA, GL_FLOAT };
        case PixelFormat::Depth16:
            return { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT };
        case PixelFormat::Depth32F:
            return { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT };
        case PixelFormat::Depth24Stencil8:
            return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8 };
    }
    return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
}

//! \brief The target, or 0 when this backend has none for the kind.
GLenum toGL(TextureKind p_kind)
{
    switch (p_kind)
    {
        case TextureKind::Texture1D:
#if defined(COMPAGES_GLES3)
            return 0u;
#else
            return GL_TEXTURE_1D;
#endif
        case TextureKind::Texture2D:
            return GL_TEXTURE_2D;
        case TextureKind::Texture3D:
            return GL_TEXTURE_3D;
        case TextureKind::TextureCube:
            return GL_TEXTURE_CUBE_MAP;
        case TextureKind::Texture2DArray:
            return GL_TEXTURE_2D_ARRAY;
    }
    return GL_TEXTURE_2D;
}

GLenum toGL(Wrap p_wrap)
{
    switch (p_wrap)
    {
        case Wrap::Repeat:
            return GL_REPEAT;
        case Wrap::MirroredRepeat:
            return GL_MIRRORED_REPEAT;
        case Wrap::ClampToEdge:
            return GL_CLAMP_TO_EDGE;
    }
    return GL_CLAMP_TO_EDGE;
}

GLint magnifyToGL(Filter p_filter)
{
    return (p_filter == Filter::Linear) ? GL_LINEAR : GL_NEAREST;
}

GLint minifyToGL(Filter p_filter, bool p_has_mipmaps)
{
    if (!p_has_mipmaps)
    {
        return (p_filter == Filter::Linear) ? GL_LINEAR : GL_NEAREST;
    }
    return (p_filter == Filter::Linear) ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
}

//! \brief Bind a texture on the scratch unit, to configure it.
GLenum bindToConfigure(NativeId p_texture)
{
    const GLenum target = detail::textureTarget(p_texture);
    glActiveTexture(GL_TEXTURE0 + detail::SCRATCH_UNIT);
    glBindTexture(target, detail::textureName(p_texture));
    return target;
}

GLsizei atLevel(std::uint32_t p_size, GLint p_level)
{
    return static_cast<GLsizei>(std::max(1u, p_size >> static_cast<std::uint32_t>(p_level)));
}

#if !defined(COMPAGES_GLES3)
//! \brief Every level of every face, empty. glTexStorage (immutable sizes)
//! is OpenGL 4.2, which macOS never had: the levels are made one by one,
//! and GL_TEXTURE_MAX_LEVEL says how many there are.
void allocateLevels(GLenum p_target, TextureDesc const& p_desc, GLFormat const& p_format)
{
    const auto internal = static_cast<GLint>(p_format.internal_format);
    for (GLint level = 0; level < static_cast<GLint>(p_desc.levels); ++level)
    {
        const GLsizei width = atLevel(p_desc.width, level);
        const GLsizei height = atLevel(p_desc.height, level);
        switch (p_desc.kind)
        {
            case TextureKind::Texture1D:
                glTexImage1D(p_target, level, internal, width, 0, p_format.layout, p_format.type, nullptr);
                break;
            case TextureKind::Texture2D:
                glTexImage2D(p_target, level, internal, width, height, 0, p_format.layout, p_format.type, nullptr);
                break;
            case TextureKind::TextureCube:
                for (GLenum face = 0u; face < 6u; ++face)
                {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, level, internal, width, height, 0,
                                 p_format.layout, p_format.type, nullptr);
                }
                break;
            case TextureKind::Texture3D:
                glTexImage3D(p_target, level, internal, width, height, atLevel(p_desc.depth, level), 0,
                             p_format.layout, p_format.type, nullptr);
                break;
            case TextureKind::Texture2DArray:
                glTexImage3D(p_target, level, internal, width, height, static_cast<GLsizei>(p_desc.depth), 0,
                             p_format.layout, p_format.type, nullptr);
                break;
        }
    }
    glTexParameteri(p_target, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(p_target, GL_TEXTURE_MAX_LEVEL, static_cast<GLint>(p_desc.levels) - 1);
}
#endif

} // namespace

Result<NativeId> createTexture(TextureDesc const& p_desc)
{
    const GLenum target = toGL(p_desc.kind);
    if (target == 0u)
    {
        return failure(std::string(name()) + " has no 1D textures: use a Texture2D one pixel high");
    }
    const GLFormat format = toGL(p_desc.format);

    GLuint texture = 0u;
    glGenTextures(1, &texture);
    if (texture == 0u)
    {
        return failure("the driver refused to create a texture");
    }
    const NativeId id = detail::textureId(texture, target);
    bindToConfigure(id);

#if defined(COMPAGES_GLES3)
    const auto levels = static_cast<GLsizei>(p_desc.levels);
    const auto width = static_cast<GLsizei>(p_desc.width);
    const auto height = static_cast<GLsizei>(p_desc.height);
    if ((p_desc.kind == TextureKind::Texture3D) || (p_desc.kind == TextureKind::Texture2DArray))
    {
        glTexStorage3D(target, levels, format.internal_format, width, height, static_cast<GLsizei>(p_desc.depth));
    }
    else
    {
        glTexStorage2D(target, levels, format.internal_format, width, height);
    }
#else
    allocateLevels(target, p_desc, format);
#endif

    const bool has_mipmaps = p_desc.levels > 1u;
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magnifyToGL(p_desc.magnify));
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minifyToGL(p_desc.minify, has_mipmaps));
    glTexParameteri(target, GL_TEXTURE_WRAP_S, static_cast<GLint>(toGL(p_desc.wrap_x)));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, static_cast<GLint>(toGL(p_desc.wrap_y)));
    glTexParameteri(target, GL_TEXTURE_WRAP_R, static_cast<GLint>(toGL(p_desc.wrap_z)));
    return id;
}

void destroyTexture(NativeId p_texture)
{
    const GLuint texture = detail::textureName(p_texture);
    glDeleteTextures(1, &texture);
}

void writeTexture(NativeId p_texture,
                  TextureDesc const& p_desc,
                  std::uint32_t p_level,
                  std::uint32_t p_x,
                  std::uint32_t p_y,
                  std::uint32_t p_z,
                  std::uint32_t p_width,
                  std::uint32_t p_height,
                  std::uint32_t p_depth,
                  const void* p_pixels)
{
    const GLenum target = bindToConfigure(p_texture);
    const GLFormat format = toGL(p_desc.format);
    const auto level = static_cast<GLint>(p_level);
    const auto x = static_cast<GLint>(p_x);
    const auto y = static_cast<GLint>(p_y);
    const auto width = static_cast<GLsizei>(p_width);
    const auto height = static_cast<GLsizei>(p_height);

    // Rows are packed, whatever their width.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    switch (p_desc.kind)
    {
        case TextureKind::Texture1D:
#if !defined(COMPAGES_GLES3)
            glTexSubImage1D(target, level, x, width, format.layout, format.type, p_pixels);
#endif
            break;

        case TextureKind::Texture2D:
            glTexSubImage2D(target, level, x, y, width, height, format.layout, format.type, p_pixels);
            break;

        // Without DSA a face is a target of its own, not a third coordinate.
        case TextureKind::TextureCube:
        {
            const std::size_t face_bytes = std::size_t(p_width) * p_height * bytesPerPixel(p_desc.format);
            const auto* bytes = static_cast<const std::byte*>(p_pixels);
            for (std::uint32_t face = 0u; face < p_depth; ++face)
            {
                glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + p_z + face, level, x, y, width, height,
                                format.layout, format.type, bytes + (face * face_bytes));
            }
            break;
        }

        case TextureKind::Texture3D:
        case TextureKind::Texture2DArray:
            glTexSubImage3D(target, level, x, y, static_cast<GLint>(p_z), width, height,
                            static_cast<GLsizei>(p_depth), format.layout, format.type, p_pixels);
            break;
    }
}

void readTexture(NativeId p_texture,
                 TextureDesc const& p_desc,
                 std::uint32_t p_level,
                 std::size_t p_bytes,
                 void* p_pixels)
{
    const GLFormat format = toGL(p_desc.format);
    GLint alignment = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

#if defined(COMPAGES_GLES3)
    // No glGetTexImage on ES: the level is attached to a framebuffer and read
    // back, which works for a 2D colour texture in the formats glReadPixels
    // accepts: RGBA bytes, RGBA floats (with EXT_color_buffer_float), red
    // integers.
    (void)p_bytes;
    const bool readable = (p_desc.kind == TextureKind::Texture2D) && !isDepth(p_desc.format) &&
                          (format.type != GL_HALF_FLOAT) &&
                          ((format.layout == GL_RGBA) || (format.layout == GL_RED_INTEGER));
    if (!readable)
    {
        reportError(std::string(name()) + " reads back only 2D colour textures in RGBA (here " +
                    toString(p_desc.kind) + ", " + toString(p_desc.format) + ")");
    }
    else
    {
        GLint draw = 0;
        GLint read = 0;
        glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
        GLuint framebuffer = 0u;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               detail::textureName(p_texture), static_cast<GLint>(p_level));
        const auto level = static_cast<GLint>(p_level);
        glReadPixels(0, 0, atLevel(p_desc.width, level), atLevel(p_desc.height, level), format.layout,
                     format.type, p_pixels);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
        glDeleteFramebuffers(1, &framebuffer);
    }
#else
    const GLenum target = bindToConfigure(p_texture);
    if (p_desc.kind == TextureKind::TextureCube)
    {
        const std::size_t face_bytes = p_bytes / 6u;
        auto* bytes = static_cast<std::byte*>(p_pixels);
        for (GLenum face = 0u; face < 6u; ++face)
        {
            glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, static_cast<GLint>(p_level), format.layout,
                          format.type, bytes + (face * face_bytes));
        }
    }
    else
    {
        glGetTexImage(target, static_cast<GLint>(p_level), format.layout, format.type, p_pixels);
    }
#endif

    glPixelStorei(GL_PACK_ALIGNMENT, alignment);
}

void generateMipmaps(NativeId p_texture)
{
    glGenerateMipmap(bindToConfigure(p_texture));
}

void setTextureFilter(NativeId p_texture,
                      Filter p_magnify,
                      Filter p_minify,
                      bool p_has_mipmaps)
{
    const GLenum target = bindToConfigure(p_texture);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, magnifyToGL(p_magnify));
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minifyToGL(p_minify, p_has_mipmaps));
}

void setTextureWrap(NativeId p_texture, Wrap p_x, Wrap p_y, Wrap p_z)
{
    const GLenum target = bindToConfigure(p_texture);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, static_cast<GLint>(toGL(p_x)));
    glTexParameteri(target, GL_TEXTURE_WRAP_T, static_cast<GLint>(toGL(p_y)));
    glTexParameteri(target, GL_TEXTURE_WRAP_R, static_cast<GLint>(toGL(p_z)));
}

void bindTexture(NativeId p_texture, std::uint32_t p_unit)
{
    glActiveTexture(GL_TEXTURE0 + p_unit);
    const GLenum target = (p_texture == NO_OBJECT) ? GLenum(GL_TEXTURE_2D) : detail::textureTarget(p_texture);
    glBindTexture(target, detail::textureName(p_texture));
}

void bindTextureAsImage(NativeId /* texture */,
                        TextureDesc const& /* desc */,
                        std::uint32_t /* unit */,
                        ImageAccess /* access */,
                        std::uint32_t /* level */)
{
    reportError(std::string(name()) + " has no image load/store (it needs OpenGL 4.2 or OpenGL ES 3.1)");
}

} // namespace compages::gpu::backend
