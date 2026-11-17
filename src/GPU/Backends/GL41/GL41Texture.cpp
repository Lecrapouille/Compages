// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "GPU/Backends/GL41/GL41.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

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


#if defined(COMPAGES_GLES3)
// ****************************************************************************
// Reading a texture back on OpenGL ES 3.0, which has no glGetTexImage and
// whose glReadPixels accepts RGBA bytes, RGBA floats and RGBA integers only.
// ****************************************************************************

//! \brief How the texels are read: as which RGBA, before keeping the
//! channels of the format.
struct Reading
{
    GLenum layout;
    GLenum type;
    std::size_t texel; //!< Bytes of one RGBA texel as read.
};

Reading readingOf(PixelFormat p_format)
{
    switch (p_format)
    {
        case PixelFormat::R8UI:
        case PixelFormat::R32UI:
            return { GL_RGBA_INTEGER, GL_UNSIGNED_INT, 16u };
        case PixelFormat::R32I:
            return { GL_RGBA_INTEGER, GL_INT, 16u };
        case PixelFormat::R16F:
        case PixelFormat::RG16F:
        case PixelFormat::RGBA16F:
        case PixelFormat::R32F:
        case PixelFormat::RG32F:
        case PixelFormat::RGBA32F:
            return { GL_RGBA, GL_FLOAT, 16u };
        case PixelFormat::R8:
        case PixelFormat::RG8:
        case PixelFormat::RGB8:
        case PixelFormat::RGBA8:
        case PixelFormat::SRGB8:
        case PixelFormat::SRGB8A8:
        case PixelFormat::Depth16:
        case PixelFormat::Depth32F:
        case PixelFormat::Depth24Stencil8:
            break;
    }
    return { GL_RGBA, GL_UNSIGNED_BYTE, 4u };
}

//! \brief A float as a half float (IEEE 754 binary16), rounded to nearest.
std::uint16_t toHalf(float p_value)
{
    std::uint32_t bits = 0u;
    std::memcpy(&bits, &p_value, sizeof(bits));
    const std::uint32_t sign = (bits >> 16u) & 0x8000u;
    const int exponent = int((bits >> 23u) & 0xFFu) - 127 + 15;
    std::uint32_t mantissa = bits & 0x7FFFFFu;
    if (((bits >> 23u) & 0xFFu) == 0xFFu)
    {
        return std::uint16_t(sign | 0x7C00u | (mantissa != 0u ? 0x200u : 0u));
    }
    if (exponent >= 31)
    {
        return std::uint16_t(sign | 0x7C00u);
    }
    if (exponent <= 0)
    {
        if (exponent < -10)
        {
            return std::uint16_t(sign);
        }
        mantissa = (mantissa | 0x800000u) >> std::uint32_t(1 - exponent);
        return std::uint16_t(sign | ((mantissa + 0x1000u) >> 13u));
    }
    const std::uint32_t half = sign | (std::uint32_t(exponent) << 10u) | (mantissa >> 13u);
    return std::uint16_t(half + ((mantissa >> 12u) & 1u)); // round
}

//! \brief Keep the channels of \c p_format from texels read as RGBA.
void keepChannels(PixelFormat p_format, Reading const& p_reading, std::byte const* p_from, std::size_t p_texels,
                  std::byte* p_to)
{
    const std::size_t channels = channelsOf(p_format);
    const std::size_t size = bytesPerPixel(p_format) / channels;
    for (std::size_t t = 0u; t < p_texels; ++t)
    {
        std::byte const* in = p_from + (t * p_reading.texel);
        std::byte* out = p_to + (t * bytesPerPixel(p_format));
        for (std::size_t c = 0u; c < channels; ++c)
        {
            if (p_reading.type == GL_UNSIGNED_BYTE)
            {
                out[c] = in[c];
            }
            else if ((p_reading.type == GL_FLOAT) && (size == 2u))
            {
                float value = 0.0f;
                std::memcpy(&value, in + (c * 4u), 4u);
                const std::uint16_t half = toHalf(value);
                std::memcpy(out + (c * 2u), &half, 2u);
            }
            else if (size == 1u)
            {
                std::uint32_t value = 0u;
                std::memcpy(&value, in + (c * 4u), 4u);
                out[c] = std::byte(value & 0xFFu);
            }
            else
            {
                std::memcpy(out + (c * size), in + (c * 4u), size);
            }
        }
    }
}

GLuint g_depth_copy = 0u;      //!< The program that writes a depth as bits.
GLint g_depth_copy_level = -1; //!< Its uniform: the level read.

//! \brief The program drawing the depth of a texture into a R32UI target, as
//! the bits of its float: ES reads integer targets back, not depth ones.
GLuint depthCopyProgram()
{
    if (g_depth_copy != 0u)
    {
        return g_depth_copy;
    }
    constexpr char const* vertex = "#version 300 es\n"
                                   "void main()\n{\n"
                                   "    vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));\n"
                                   "    gl_Position = vec4(p * 2.0 - 1.0, 0.0, 1.0);\n}\n";
    constexpr char const* fragment = "#version 300 es\n"
                                     "precision highp float;\nprecision highp int;\n"
                                     "uniform highp sampler2D depth;\nuniform int level;\nout highp uvec4 bits;\n"
                                     "void main()\n{\n"
                                     "    float d = texelFetch(depth, ivec2(gl_FragCoord.xy), level).r;\n"
                                     "    bits = uvec4(floatBitsToUint(d), 0u, 0u, 0u);\n}\n";
    const GLuint program = glCreateProgram();
    for (auto [stage, text] : { std::pair{ GLenum(GL_VERTEX_SHADER), vertex }, std::pair{ GLenum(GL_FRAGMENT_SHADER), fragment } })
    {
        const GLuint shader = glCreateShader(stage);
        glShaderSource(shader, 1, &text, nullptr);
        glCompileShader(shader);
        glAttachShader(program, shader);
        glDeleteShader(shader); // freed with the program
    }
    glLinkProgram(program);
    g_depth_copy = program;
    g_depth_copy_level = glGetUniformLocation(program, "level");
    return program;
}

//! \brief Read one level of a 2D depth texture into \c p_pixels.
void readDepth(NativeId p_texture, TextureDesc const& p_desc, GLint p_level, void* p_pixels)
{
    const GLsizei width = atLevel(p_desc.width, p_level);
    const GLsizei height = atLevel(p_desc.height, p_level);

    // What the copy changes, put back after it.
    GLint viewport[4] = { 0, 0, 0, 0 };
    glGetIntegerv(GL_VIEWPORT, viewport);
    const GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    GLint draw = 0;
    GLint read = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);

    GLuint target = 0u;
    glGenTextures(1, &target);
    glActiveTexture(GL_TEXTURE0 + detail::SCRATCH_UNIT);
    glBindTexture(GL_TEXTURE_2D, target);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_R32UI, width, height);
    GLuint framebuffer = 0u;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);

    // A depth texture is not filterable on ES: NEAREST while it is read.
    const GLenum source = bindToConfigure(p_texture);
    GLint min_filter = 0;
    GLint mag_filter = 0;
    glGetTexParameteriv(source, GL_TEXTURE_MIN_FILTER, &min_filter);
    glGetTexParameteriv(source, GL_TEXTURE_MAG_FILTER, &mag_filter);
    glTexParameteri(source, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(source, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    detail::useProgram(depthCopyProgram());
    glUniform1i(glGetUniformLocation(g_depth_copy, "depth"), GLint(detail::SCRATCH_UNIT));
    glUniform1i(g_depth_copy_level, p_level);
    detail::bindVertexArray(0u);
    glViewport(0, 0, width, height);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    std::vector<std::uint32_t> words(std::size_t(width) * std::size_t(height) * 4u);
    glReadPixels(0, 0, width, height, GL_RGBA_INTEGER, GL_UNSIGNED_INT, words.data());
    auto* out = static_cast<std::byte*>(p_pixels);
    const std::size_t texel = bytesPerPixel(p_desc.format);
    for (std::size_t t = 0u; t < std::size_t(width) * std::size_t(height); ++t)
    {
        float depth = 0.0f;
        std::memcpy(&depth, &words[t * 4u], 4u);
        if (p_desc.format == PixelFormat::Depth32F)
        {
            std::memcpy(out + (t * texel), &depth, 4u);
        }
        else if (p_desc.format == PixelFormat::Depth16)
        {
            const auto value = std::uint16_t(std::lround(std::clamp(depth, 0.0f, 1.0f) * 65535.0f));
            std::memcpy(out + (t * texel), &value, 2u);
        }
        else // Depth24Stencil8: the depth in the high 24 bits, the stencil lost
        {
            const auto value = std::uint32_t(std::lround(std::clamp(depth, 0.0f, 1.0f) * 16777215.0f)) << 8u;
            std::memcpy(out + (t * texel), &value, 4u);
        }
    }

    glTexParameteri(source, GL_TEXTURE_MIN_FILTER, min_filter);
    glTexParameteri(source, GL_TEXTURE_MAG_FILTER, mag_filter);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &target);
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    if (scissor == GL_TRUE)
    {
        glEnable(GL_SCISSOR_TEST);
    }
    // The next pipeline sends all of its state again.
    forgetRenderState();
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
    GLint alignment = 4;
    glGetIntegerv(GL_PACK_ALIGNMENT, &alignment);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);

#if defined(COMPAGES_GLES3)
    if (isDepth(p_desc.format))
    {
        if (p_desc.kind != TextureKind::Texture2D)
        {
            reportError(std::string(name()) + " reads back the depth of a 2D texture only");
        }
        else
        {
            readDepth(p_texture, p_desc, static_cast<GLint>(p_level), p_pixels);
        }
        glPixelStorei(GL_PACK_ALIGNMENT, alignment);
        return;
    }

    // Each face or layer is attached to a framebuffer and read as RGBA, then
    // the channels of the format are kept. Float targets need the extension
    // EXT_color_buffer_float (most WebGL 2 browsers give it).
    const Reading reading = readingOf(p_desc.format);
    GLint draw = 0;
    GLint read = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &draw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &read);
    GLuint framebuffer = 0u;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);
    const auto level = static_cast<GLint>(p_level);
    const GLsizei width = atLevel(p_desc.width, level);
    const GLsizei height = atLevel(p_desc.height, level);
    const GLuint layers = (p_desc.kind == TextureKind::TextureCube)      ? 6u
                          : (p_desc.kind == TextureKind::Texture3D)      ? GLuint(atLevel(p_desc.depth, level))
                          : (p_desc.kind == TextureKind::Texture2DArray) ? GLuint(p_desc.depth)
                                                                         : 1u;
    const std::size_t texels = std::size_t(width) * std::size_t(height);
    std::vector<std::byte> rgba(texels * reading.texel);
    auto* bytes = static_cast<std::byte*>(p_pixels);
    const GLuint texture = detail::textureName(p_texture);
    for (GLuint layer = 0u; (layer < layers) && ((layer + 1u) * texels * bytesPerPixel(p_desc.format) <= p_bytes); ++layer)
    {
        if (p_desc.kind == TextureKind::Texture2D)
        {
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, level);
        }
        else if (p_desc.kind == TextureKind::TextureCube)
        {
            glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + layer,
                                   texture, level);
        }
        else
        {
            glFramebufferTextureLayer(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, level, GLint(layer));
        }
        glReadPixels(0, 0, width, height, reading.layout, reading.type, rgba.data());
        keepChannels(p_desc.format, reading, rgba.data(), texels,
                     bytes + (std::size_t(layer) * texels * bytesPerPixel(p_desc.format)));
    }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(read));
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<GLuint>(draw));
    glDeleteFramebuffers(1, &framebuffer);
#else
    const GLFormat format = toGL(p_desc.format);
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

namespace detail
{

void releaseTextureReadback()
{
#if defined(COMPAGES_GLES3)
    if (g_depth_copy != 0u)
    {
        detail::useProgram(0u);
        glDeleteProgram(g_depth_copy);
        g_depth_copy = 0u;
        g_depth_copy_level = -1;
    }
#endif
}

} // namespace detail

} // namespace compages::gpu::backend
