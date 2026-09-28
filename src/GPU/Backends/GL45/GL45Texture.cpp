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

namespace compages::gpu::backend
{

namespace
{

// ****************************************************************************
//! \brief How one of our formats is spelled in the three ways OpenGL needs it.
//!
//! OpenGL asks the same question three times over: the internal format says how
//! the device stores the pixels, while the format and the type together say how
//! the bytes being handed over are arranged. Keeping the three together in one
//! row is what stops them from drifting apart, which is a mistake that shows as
//! wrong colours rather than as an error.
// ****************************************************************************
struct GLFormat
{
    GLenum internal_format;
    GLenum layout;
    GLenum type;
};

//------------------------------------------------------------------------------
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

        // An integer format is handed integer pixels: GL_RED_INTEGER rather than
        // GL_RED. Getting this one wrong is the classic way to end up with a
        // texture full of zeroes.
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
            return { GL_DEPTH_COMPONENT16, GL_DEPTH_COMPONENT,
                     GL_UNSIGNED_SHORT };
        case PixelFormat::Depth32F:
            return { GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT };
        case PixelFormat::Depth24Stencil8:
            return { GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL,
                     GL_UNSIGNED_INT_24_8 };
    }
    return { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };
}

//------------------------------------------------------------------------------
GLenum toGL(TextureKind p_kind)
{
    switch (p_kind)
    {
        case TextureKind::Texture1D:
            return GL_TEXTURE_1D;
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

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
//! \brief What the driver is told about magnification, which knows nothing of
//! mipmaps: only one image is ever involved.
//------------------------------------------------------------------------------
GLenum magnifyToGL(Filter p_filter)
{
    return (p_filter == Filter::Linear) ? GL_LINEAR : GL_NEAREST;
}

//------------------------------------------------------------------------------
//! \brief What the driver is told about reduction, which does know about mipmaps.
//!
//! Two choices in one, and forgetting the second is a classic: a texture with
//! mipmaps whose reduction filter says GL_LINEAR never reads them, so all that
//! memory sits unused and the image still shimmers. Asking for Linear on a
//! texture that has mipmaps therefore means filtering between levels as well as
//! within them.
//------------------------------------------------------------------------------
GLenum minifyToGL(Filter p_filter, bool p_has_mipmaps)
{
    if (!p_has_mipmaps)
    {
        return (p_filter == Filter::Linear) ? GL_LINEAR : GL_NEAREST;
    }
    return (p_filter == Filter::Linear) ? GL_LINEAR_MIPMAP_LINEAR
                                        : GL_NEAREST_MIPMAP_NEAREST;
}

//------------------------------------------------------------------------------
GLenum toGL(ImageAccess p_access)
{
    switch (p_access)
    {
        case ImageAccess::Read:
            return GL_READ_ONLY;
        case ImageAccess::Write:
            return GL_WRITE_ONLY;
        case ImageAccess::ReadWrite:
            return GL_READ_WRITE;
    }
    return GL_READ_WRITE;
}

} // namespace

//------------------------------------------------------------------------------
// glCreateTextures and glTextureStorage, with no binding anywhere: the same
// Direct State Access the buffers use. glTextureStorage also does something
// glTexImage2D never did, which is to fix the size and the format once and for
// all. A texture whose format cannot change is one a framebuffer can refer to
// without checking.
//------------------------------------------------------------------------------
Result<NativeId> createTexture(TextureDesc const& p_desc)
{
    const GLenum target = toGL(p_desc.kind);
    const GLFormat format = toGL(p_desc.format);

    GLuint name = 0u;
    glCreateTextures(target, 1, &name);
    if (name == 0u)
    {
        return failure("the driver refused to create a texture");
    }

    const GLsizei levels = static_cast<GLsizei>(p_desc.levels);
    const GLsizei width = static_cast<GLsizei>(p_desc.width);
    const GLsizei height = static_cast<GLsizei>(p_desc.height);

    switch (p_desc.kind)
    {
        case TextureKind::Texture1D:
            glTextureStorage1D(name, levels, format.internal_format, width);
            break;

        // A cube map is six square images, and the driver is told its size the
        // same way a 2D texture is: the six faces are implied by the target.
        case TextureKind::Texture2D:
        case TextureKind::TextureCube:
            glTextureStorage2D(
                name, levels, format.internal_format, width, height);
            break;

        case TextureKind::Texture3D:
        case TextureKind::Texture2DArray:
            glTextureStorage3D(name,
                               levels,
                               format.internal_format,
                               width,
                               height,
                               static_cast<GLsizei>(p_desc.depth));
            break;
    }

    const bool has_mipmaps = p_desc.levels > 1u;
    glTextureParameteri(name, GL_TEXTURE_MAG_FILTER,
                        static_cast<GLint>(magnifyToGL(p_desc.magnify)));
    glTextureParameteri(
        name,
        GL_TEXTURE_MIN_FILTER,
        static_cast<GLint>(minifyToGL(p_desc.minify, has_mipmaps)));
    glTextureParameteri(name, GL_TEXTURE_WRAP_S,
                        static_cast<GLint>(toGL(p_desc.wrap_x)));
    glTextureParameteri(name, GL_TEXTURE_WRAP_T,
                        static_cast<GLint>(toGL(p_desc.wrap_y)));
    glTextureParameteri(name, GL_TEXTURE_WRAP_R,
                        static_cast<GLint>(toGL(p_desc.wrap_z)));

    return static_cast<NativeId>(name);
}

//------------------------------------------------------------------------------
void destroyTexture(NativeId p_texture)
{
    const GLuint name = static_cast<GLuint>(p_texture);
    glDeleteTextures(1, &name);
}

//------------------------------------------------------------------------------
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
    const GLuint name = static_cast<GLuint>(p_texture);
    const GLFormat format = toGL(p_desc.format);

    // Rows are handed over packed, one after the other with no padding. The
    // driver assumes four byte alignment unless told otherwise, which is why an
    // RGB8 image of an odd width used to arrive skewed.
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    switch (p_desc.kind)
    {
        case TextureKind::Texture1D:
            glTextureSubImage1D(name,
                                static_cast<GLint>(p_level),
                                static_cast<GLint>(p_x),
                                static_cast<GLsizei>(p_width),
                                format.layout,
                                format.type,
                                p_pixels);
            break;

        case TextureKind::Texture2D:
            glTextureSubImage2D(name,
                                static_cast<GLint>(p_level),
                                static_cast<GLint>(p_x),
                                static_cast<GLint>(p_y),
                                static_cast<GLsizei>(p_width),
                                static_cast<GLsizei>(p_height),
                                format.layout,
                                format.type,
                                p_pixels);
            break;

        // A face of a cube map and a layer of an array are both reached by the
        // third coordinate, which is what lets one call cover all three shapes.
        case TextureKind::Texture3D:
        case TextureKind::TextureCube:
        case TextureKind::Texture2DArray:
            glTextureSubImage3D(name,
                                static_cast<GLint>(p_level),
                                static_cast<GLint>(p_x),
                                static_cast<GLint>(p_y),
                                static_cast<GLint>(p_z),
                                static_cast<GLsizei>(p_width),
                                static_cast<GLsizei>(p_height),
                                static_cast<GLsizei>(p_depth),
                                format.layout,
                                format.type,
                                p_pixels);
            break;
    }
}

//------------------------------------------------------------------------------
void readTexture(NativeId p_texture,
                 TextureDesc const& p_desc,
                 std::uint32_t p_level,
                 std::size_t p_bytes,
                 void* p_pixels)
{
    const GLFormat format = toGL(p_desc.format);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glGetTextureImage(static_cast<GLuint>(p_texture),
                      static_cast<GLint>(p_level),
                      format.layout,
                      format.type,
                      static_cast<GLsizei>(p_bytes),
                      p_pixels);
}

//------------------------------------------------------------------------------
void generateMipmaps(NativeId p_texture)
{
    glGenerateTextureMipmap(static_cast<GLuint>(p_texture));
}

//------------------------------------------------------------------------------
void setTextureFilter(NativeId p_texture,
                      Filter p_magnify,
                      Filter p_minify,
                      bool p_has_mipmaps)
{
    const GLuint name = static_cast<GLuint>(p_texture);
    glTextureParameteri(name, GL_TEXTURE_MAG_FILTER,
                        static_cast<GLint>(magnifyToGL(p_magnify)));
    glTextureParameteri(name,
                        GL_TEXTURE_MIN_FILTER,
                        static_cast<GLint>(minifyToGL(p_minify, p_has_mipmaps)));
}

//------------------------------------------------------------------------------
void setTextureWrap(NativeId p_texture, Wrap p_x, Wrap p_y, Wrap p_z)
{
    const GLuint name = static_cast<GLuint>(p_texture);
    glTextureParameteri(name, GL_TEXTURE_WRAP_S, static_cast<GLint>(toGL(p_x)));
    glTextureParameteri(name, GL_TEXTURE_WRAP_T, static_cast<GLint>(toGL(p_y)));
    glTextureParameteri(name, GL_TEXTURE_WRAP_R, static_cast<GLint>(toGL(p_z)));
}

//------------------------------------------------------------------------------
// glBindTextureUnit, which names the unit directly. Before Direct State Access
// this was two calls, glActiveTexture then glBindTexture, and the first of them
// left behind a piece of state that the next unrelated call inherited.
//------------------------------------------------------------------------------
void bindTexture(NativeId p_texture, std::uint32_t p_unit)
{
    glBindTextureUnit(static_cast<GLuint>(p_unit),
                      static_cast<GLuint>(p_texture));
}

//------------------------------------------------------------------------------
void bindTextureAsImage(NativeId p_texture,
                        TextureDesc const& p_desc,
                        std::uint32_t p_unit,
                        ImageAccess p_access,
                        std::uint32_t p_level)
{
    // Every layer at once, which is what a compute shader addressing a volume or
    // an array of images expects.
    glBindImageTexture(static_cast<GLuint>(p_unit),
                       static_cast<GLuint>(p_texture),
                       static_cast<GLint>(p_level),
                       GL_TRUE,
                       0,
                       toGL(p_access),
                       toGL(p_desc.format).internal_format);
}

} // namespace compages::gpu::backend
