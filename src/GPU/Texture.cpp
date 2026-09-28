// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/Texture.hpp"
#include "Compages/GPU/Errors.hpp"
#include "Compages/GPU/Device.hpp"
#include "GPU/Internal/Pools.hpp"

#include <stb_image.h>

#include <algorithm>
#include <cstring>
#include <memory>

namespace compages::gpu
{

namespace
{

// ****************************************************************************
//! \brief How large one level of detail is.
// ****************************************************************************
struct Extent
{
    std::uint32_t width = 0u;
    std::uint32_t height = 0u;
    //! \brief How many images the level holds along the third axis: the depth of
    //! a volume, the number of layers of an array, or six for a cube map.
    std::uint32_t depth = 0u;

    [[nodiscard]] std::size_t pixels() const
    {
        return static_cast<std::size_t>(width) * height * depth;
    }
};

//! \brief How large a given level of detail is.
//!
//! Each level is half the size of the one before, but only along the axes that
//! really shrink. The depth of a volume halves; the number of layers of an array
//! does not, because the layers are separate images that happen to share a size.
//! Treating the two alike is a mistake that shows as a write refused for the
//! wrong reason, or worse, accepted.
Extent extentOf(TextureDesc const& p_desc, std::uint32_t p_level)
{
    const auto halve = [p_level](std::uint32_t p_size) {
        return std::max(1u, p_size >> p_level);
    };

    Extent extent;
    extent.width = halve(p_desc.width);
    extent.height = halve(p_desc.height);

    switch (p_desc.kind)
    {
        case TextureKind::Texture3D:
            extent.depth = halve(p_desc.depth);
            break;
        case TextureKind::Texture2DArray:
            extent.depth = p_desc.depth;
            break;
        case TextureKind::TextureCube:
            extent.depth = 6u;
            break;
        case TextureKind::Texture1D:
        case TextureKind::Texture2D:
            extent.depth = 1u;
            break;
    }
    return extent;
}

//! \brief How many bytes one whole level of detail occupies.
std::size_t bytesOfLevel(TextureDesc const& p_desc, std::uint32_t p_level)
{
    return extentOf(p_desc, p_level).pixels() * bytesPerPixel(p_desc.format);
}

//! \brief How many bytes the whole texture occupies.
std::size_t bytesOfTexture(TextureDesc const& p_desc)
{
    std::size_t total = 0u;
    for (std::uint32_t level = 0u; level < p_desc.levels; ++level)
    {
        total += bytesOfLevel(p_desc, level);
    }
    return total;
}

//! \brief How many levels of detail a texture of this size can have, the last
//! being a single pixel.
std::uint32_t fullMipChain(TextureDesc const& p_desc)
{
    std::uint32_t largest = std::max(p_desc.width, p_desc.height);
    if (p_desc.kind == TextureKind::Texture3D)
    {
        largest = std::max(largest, p_desc.depth);
    }

    std::uint32_t levels = 1u;
    while (largest > 1u)
    {
        largest >>= 1u;
        ++levels;
    }
    return levels;
}

//! \brief Fill in what the caller left implied, and refuse what cannot work.
//!
//! Everything checked here is something the driver would either refuse in its own
//! words or, worse, accept and then read wrongly.
Result<TextureDesc> settle(TextureDesc p_desc)
{
    if (p_desc.width == 0u)
    {
        return failure("a texture of zero width cannot be created. A size of "
                       "zero is almost always a size that was never computed");
    }

    switch (p_desc.kind)
    {
        case TextureKind::Texture1D:
            if ((p_desc.height != 1u) || (p_desc.depth != 1u))
            {
                return failure("a 1D texture is a single row of pixels, so its "
                               "height and depth must be left at one");
            }
            break;

        case TextureKind::Texture2D:
            if (p_desc.height == 0u)
            {
                return failure("a 2D texture of zero height cannot be created");
            }
            if (p_desc.depth != 1u)
            {
                return failure("a 2D texture holds one image, so its depth must "
                               "be left at one. For several images of the same "
                               "size use TextureKind::Texture2DArray, and for a "
                               "volume use Texture3D");
            }
            break;

        case TextureKind::Texture3D:
        case TextureKind::Texture2DArray:
            if ((p_desc.height == 0u) || (p_desc.depth == 0u))
            {
                return failure(
                    std::string("a ") + toString(p_desc.kind) +
                    " needs a height and a depth of at least one");
            }
            break;

        case TextureKind::TextureCube:
            if (p_desc.width != p_desc.height)
            {
                return failure("the faces of a cube map are square, but " +
                               std::to_string(p_desc.width) + " by " +
                               std::to_string(p_desc.height) + " was asked for");
            }
            // Six faces, always, whatever was put in the depth.
            p_desc.depth = 6u;
            break;
    }

    // Zero means as many levels as the size allows.
    if (p_desc.levels == 0u)
    {
        p_desc.levels = fullMipChain(p_desc);
    }
    else
    {
        const std::uint32_t most = fullMipChain(p_desc);
        if (p_desc.levels > most)
        {
            return failure(
                std::to_string(p_desc.levels) +
                " levels of detail were asked for, but a texture of " +
                std::to_string(p_desc.width) + " by " +
                std::to_string(p_desc.height) + " can have at most " +
                std::to_string(most) + ", the last one being a single pixel");
        }
    }

    // The driver's own ceilings, named so that a size can be brought under them
    // rather than guessed at.
    const auto largest_2d = static_cast<std::uint32_t>(device().max_texture_size);
    const auto largest_3d =
        static_cast<std::uint32_t>(device().max_texture_size_3d);

    if (p_desc.kind == TextureKind::Texture3D)
    {
        if ((p_desc.width > largest_3d) || (p_desc.height > largest_3d) ||
            (p_desc.depth > largest_3d))
        {
            return failure("a 3D texture of " + std::to_string(p_desc.width) +
                           " by " + std::to_string(p_desc.height) + " by " +
                           std::to_string(p_desc.depth) +
                           " was asked for, but this driver allows at most " +
                           std::to_string(largest_3d) + " along each axis");
        }
    }
    else if ((p_desc.width > largest_2d) || (p_desc.height > largest_2d))
    {
        return failure("a texture of " + std::to_string(p_desc.width) + " by " +
                       std::to_string(p_desc.height) +
                       " was asked for, but this driver allows at most " +
                       std::to_string(largest_2d) + " along each axis");
    }

    return p_desc;
}

// ****************************************************************************
//! \brief An image read from a file, freed by stb_image whatever happens next.
// ****************************************************************************
class LoadedImage
{
public:

    LoadedImage() = default;

    ~LoadedImage()
    {
        if (m_pixels != nullptr)
        {
            stbi_image_free(m_pixels);
        }
    }

    LoadedImage(LoadedImage const&) = delete;
    LoadedImage& operator=(LoadedImage const&) = delete;

    // ------------------------------------------------------------------------
    //! \brief Read a file, choosing the format from what is in it.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status load(std::string const& p_path, bool p_srgb)
    {
        int channels = 0;

        // A file holding values beyond one, as a photograph of a bright sky does,
        // is read as floats: squeezing it into bytes would throw away exactly what
        // it was saved for.
        if (stbi_is_hdr(p_path.c_str()) != 0)
        {
            m_pixels = stbi_loadf(p_path.c_str(), &m_width, &m_height,
                                  &channels, 4);
            m_format = PixelFormat::RGBA32F;
        }
        else
        {
            m_pixels = stbi_load(p_path.c_str(), &m_width, &m_height, &channels,
                                 0);
            switch (channels)
            {
                case 1:
                    m_format = PixelFormat::R8;
                    break;
                case 2:
                    m_format = PixelFormat::RG8;
                    break;
                case 3:
                    m_format = p_srgb ? PixelFormat::SRGB8 : PixelFormat::RGB8;
                    break;
                default:
                    m_format =
                        p_srgb ? PixelFormat::SRGB8A8 : PixelFormat::RGBA8;
                    break;
            }
        }

        if (m_pixels == nullptr)
        {
            // stb_image says what is wrong in a sentence, which is more use than
            // anything this layer could add.
            const char* why = stbi_failure_reason();
            return failure("cannot read the image '" + p_path + "': " +
                           ((why == nullptr) ? "unknown reason" : why));
        }
        if ((m_width <= 0) || (m_height <= 0))
        {
            return failure("the image '" + p_path + "' has no pixels");
        }

        return success();
    }

    [[nodiscard]] std::uint32_t width() const
    {
        return static_cast<std::uint32_t>(m_width);
    }

    [[nodiscard]] std::uint32_t height() const
    {
        return static_cast<std::uint32_t>(m_height);
    }

    [[nodiscard]] PixelFormat format() const
    {
        return m_format;
    }

    [[nodiscard]] std::span<const std::byte> pixels() const
    {
        const std::size_t bytes = static_cast<std::size_t>(m_width) *
                                  static_cast<std::size_t>(m_height) *
                                  bytesPerPixel(m_format);
        return { static_cast<const std::byte*>(m_pixels), bytes };
    }

private:

    void* m_pixels = nullptr;
    int m_width = 0;
    int m_height = 0;
    PixelFormat m_format = PixelFormat::RGBA8;
};

//! \brief The message given when a handle names nothing alive.
std::string staleTextureMessage()
{
    return "this texture no longer exists. Either it was released while "
           "something still referred to it, or the Texture object was moved from "
           "and the old one is being used";
}

} // namespace

Result<Texture> Texture::create(TextureDesc const& p_desc)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is no device "
                       "to put a texture on");
    }

    auto settled_result = settle(p_desc);
    if (!settled_result)
    {
        return compages::failure(settled_result.error());
    }
    auto settled = settled_result.take();
    auto native_result = backend::createTexture(settled);
    if (!native_result)
    {
        return compages::failure(native_result.error());
    }
    auto native = native_result.take();

    auto added = detail::pools().textures.add(
        detail::TextureRecord{ native, settled, bytesOfTexture(settled) });
    if (!added)
    {
        backend::destroyTexture(native);
        return failure(added.error());
    }
    return Texture(added.take());
}

Result<Texture> Texture::fromFile(std::string const& p_path,
                                  LoadOptions const& p_options)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is no device "
                       "to put a texture on");
    }

    // Image files begin at their top row, while a texture coordinate of zero is
    // at the bottom. Turning the rows over here means no shader has to know.
    stbi_set_flip_vertically_on_load(p_options.flip_vertically ? 1 : 0);

    LoadedImage image;
    COMPAGES_TRY(image.load(p_path, p_options.srgb));

    TextureDesc desc;
    desc.kind = TextureKind::Texture2D;
    desc.format = image.format();
    desc.width = image.width();
    desc.height = image.height();
    desc.levels = p_options.mipmaps ? 0u : 1u;
    desc.magnify = p_options.filter;
    desc.minify = p_options.filter;
    desc.wrap_x = p_options.wrap;
    desc.wrap_y = p_options.wrap;

    auto texture_result = create(desc);
    if (!texture_result)
    {
        return compages::failure(texture_result.error());
    }
    auto texture = texture_result.take();
    COMPAGES_TRY(texture.write(image.pixels()));
    if (p_options.mipmaps)
    {
        COMPAGES_TRY(texture.generateMipmaps());
    }
    return texture;
}

Result<Texture> Texture::cubeFromFiles(
    std::array<std::string, 6u> const& p_paths, LoadOptions const& p_options)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is no device "
                       "to put a texture on");
    }

    stbi_set_flip_vertically_on_load(p_options.flip_vertically ? 1 : 0);

    // The six faces are read before anything is set aside on the device, so that
    // a missing file is reported without having reserved memory for nothing.
    std::array<LoadedImage, 6u> faces;
    for (std::size_t i = 0u; i < 6u; ++i)
    {
        COMPAGES_TRY(faces[i].load(p_paths[i], p_options.srgb));
    }

    for (std::size_t i = 1u; i < 6u; ++i)
    {
        if ((faces[i].width() != faces[0].width()) ||
            (faces[i].height() != faces[0].height()) ||
            (faces[i].format() != faces[0].format()))
        {
            return failure(
                "the six faces of a cube map must be the same size and format, "
                "but '" + p_paths[i] + "' is " +
                std::to_string(faces[i].width()) + " by " +
                std::to_string(faces[i].height()) + " " +
                toString(faces[i].format()) + " while '" + p_paths[0] + "' is " +
                std::to_string(faces[0].width()) + " by " +
                std::to_string(faces[0].height()) + " " +
                toString(faces[0].format()));
        }
    }

    TextureDesc desc;
    desc.kind = TextureKind::TextureCube;
    desc.format = faces[0].format();
    desc.width = faces[0].width();
    desc.height = faces[0].height();
    desc.levels = p_options.mipmaps ? 0u : 1u;
    desc.magnify = p_options.filter;
    desc.minify = p_options.filter;
    desc.wrap_x = p_options.wrap;
    desc.wrap_y = p_options.wrap;
    desc.wrap_z = p_options.wrap;

    auto texture_result = create(desc);
    if (!texture_result)
    {
        return compages::failure(texture_result.error());
    }
    auto texture = texture_result.take();
    for (std::uint32_t face = 0u; face < 6u; ++face)
    {
        COMPAGES_TRY(texture.writeLayer(face, faces[face].pixels()));
    }
    if (p_options.mipmaps)
    {
        COMPAGES_TRY(texture.generateMipmaps());
    }
    return texture;
}

Result<Texture> Texture::volumeFromFiles(std::vector<std::string> const& p_paths,
                                         LoadOptions const& p_options)
{
    if (!initialized())
    {
        return failure("compages::gpu::init() has not been called, so there is no device "
                       "to put a texture on");
    }
    if (p_paths.empty())
    {
        return failure("a volume needs at least one image file");
    }

    stbi_set_flip_vertically_on_load(p_options.flip_vertically ? 1 : 0);

    std::vector<std::unique_ptr<LoadedImage>> layers;
    for (std::string const& path : p_paths)
    {
        layers.emplace_back(std::make_unique<LoadedImage>());
        COMPAGES_TRY(layers.back()->load(path, p_options.srgb));
        LoadedImage const& first = *layers.front();
        LoadedImage const& last = *layers.back();
        if ((last.width() != first.width()) || (last.height() != first.height()) ||
            (last.format() != first.format()))
        {
            return failure("the layers of a volume must be the same size and "
                           "format, but '" + path + "' is " +
                           std::to_string(last.width()) + " by " +
                           std::to_string(last.height()) + " " +
                           toString(last.format()) + " while '" + p_paths[0] +
                           "' is " + std::to_string(first.width()) + " by " +
                           std::to_string(first.height()) + " " +
                           toString(first.format()));
        }
    }

    TextureDesc desc;
    desc.kind = TextureKind::Texture3D;
    desc.format = layers[0]->format();
    desc.width = layers[0]->width();
    desc.height = layers[0]->height();
    desc.depth = static_cast<std::uint32_t>(layers.size());
    desc.levels = p_options.mipmaps ? 0u : 1u;
    desc.magnify = p_options.filter;
    desc.minify = p_options.filter;
    desc.wrap_x = p_options.wrap;
    desc.wrap_y = p_options.wrap;
    desc.wrap_z = p_options.wrap;

    auto texture_result = create(desc);
    if (!texture_result)
    {
        return compages::failure(texture_result.error());
    }
    auto texture = texture_result.take();
    for (std::uint32_t z = 0u; z < desc.depth; ++z)
    {
        COMPAGES_TRY(texture.writeLayer(z, layers[z]->pixels()));
    }
    if (p_options.mipmaps)
    {
        COMPAGES_TRY(texture.generateMipmaps());
    }
    return texture;
}

Status Texture::loadVolume(std::vector<std::string> const& p_paths,
                           LoadOptions const& p_options)
{
    COMPAGES_TRY_ASSIGN(*this, volumeFromFiles(p_paths, p_options));
    return success();
}

Status Texture::allocate(TextureDesc const& p_desc)
{
    COMPAGES_TRY_ASSIGN(*this, create(p_desc));
    return success();
}

Status Texture::load(std::string const& p_path, LoadOptions const& p_options)
{
    COMPAGES_TRY_ASSIGN(*this, fromFile(p_path, p_options));
    return success();
}

Status Texture::loadCube(std::array<std::string, 6u> const& p_paths,
                         LoadOptions const& p_options)
{
    COMPAGES_TRY_ASSIGN(*this, cubeFromFiles(p_paths, p_options));
    return success();
}

Texture::Texture(Texture&& p_other) noexcept : m_handle(p_other.m_handle)
{
    p_other.m_handle = TextureHandle{};
}

Texture& Texture::operator=(Texture&& p_other) noexcept
{
    if (this != &p_other)
    {
        release();
        m_handle = p_other.m_handle;
        p_other.m_handle = TextureHandle{};
    }
    return *this;
}

Texture::~Texture()
{
    release();
}

void Texture::release()
{
    detail::TextureRecord* record = detail::pools().textures.get(m_handle);
    if (record != nullptr)
    {
        if (initialized())
        {
            backend::destroyTexture(record->native);
        }
        (void)detail::pools().textures.remove(m_handle);
    }
    m_handle = TextureHandle{};
}

Status Texture::write(std::span<const std::byte> p_pixels, std::uint32_t p_level)
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }
    if (p_level >= record->desc.levels)
    {
        return failure("level of detail " + std::to_string(p_level) +
                       " does not exist: this texture has " +
                       std::to_string(record->desc.levels));
    }

    const Extent extent = extentOf(record->desc, p_level);
    return write(0u,
                 0u,
                 0u,
                 extent.width,
                 extent.height,
                 extent.depth,
                 p_pixels,
                 p_level);
}

Status Texture::write(std::uint32_t p_x,
                      std::uint32_t p_y,
                      std::uint32_t p_z,
                      std::uint32_t p_width,
                      std::uint32_t p_height,
                      std::uint32_t p_depth,
                      std::span<const std::byte> p_pixels,
                      std::uint32_t p_level)
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }
    if (p_level >= record->desc.levels)
    {
        return failure("level of detail " + std::to_string(p_level) +
                       " does not exist: this texture has " +
                       std::to_string(record->desc.levels));
    }

    const Extent extent = extentOf(record->desc, p_level);
    if ((p_x + p_width > extent.width) || (p_y + p_height > extent.height) ||
        (p_z + p_depth > extent.depth))
    {
        return failure(
            "the region being written runs outside the texture: " +
            std::to_string(p_width) + " by " + std::to_string(p_height) +
            " by " + std::to_string(p_depth) + " at " + std::to_string(p_x) +
            ", " + std::to_string(p_y) + ", " + std::to_string(p_z) +
            " does not fit in a level of " + std::to_string(extent.width) +
            " by " + std::to_string(extent.height) + " by " +
            std::to_string(extent.depth));
    }

    // The check the previous layer never made. Handing over fewer pixels than the
    // region needs makes the driver read past the end of the caller's memory,
    // which is undefined behaviour with no message at all.
    const std::size_t needed = static_cast<std::size_t>(p_width) * p_height *
                               p_depth * bytesPerPixel(record->desc.format);
    if (p_pixels.size() != needed)
    {
        return failure(
            "a region of " + std::to_string(p_width) + " by " +
            std::to_string(p_height) + " by " + std::to_string(p_depth) +
            " pixels of " + toString(record->desc.format) + " needs " +
            std::to_string(needed) + " bytes, but " +
            std::to_string(p_pixels.size()) + " were given");
    }

    backend::writeTexture(record->native,
                          record->desc,
                          p_level,
                          p_x,
                          p_y,
                          p_z,
                          p_width,
                          p_height,
                          p_depth,
                          p_pixels.data());
    return success();
}

Status Texture::writeLayer(std::uint32_t p_layer,
                           std::span<const std::byte> p_pixels,
                           std::uint32_t p_level)
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }
    if (p_level >= record->desc.levels)
    {
        return failure("level of detail " + std::to_string(p_level) +
                       " does not exist: this texture has " +
                       std::to_string(record->desc.levels));
    }

    const Extent extent = extentOf(record->desc, p_level);
    if (p_layer >= extent.depth)
    {
        return failure("layer " + std::to_string(p_layer) +
                       " does not exist: this " + toString(record->desc.kind) +
                       " holds " + std::to_string(extent.depth));
    }

    return write(
        0u, 0u, p_layer, extent.width, extent.height, 1u, p_pixels, p_level);
}

Result<std::vector<std::byte>> Texture::read(std::uint32_t p_level) const
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }
    if (p_level >= record->desc.levels)
    {
        return failure("level of detail " + std::to_string(p_level) +
                       " does not exist: this texture has " +
                       std::to_string(record->desc.levels));
    }

    std::vector<std::byte> pixels(bytesOfLevel(record->desc, p_level));
    backend::readTexture(record->native,
                         record->desc,
                         p_level,
                         pixels.size(),
                         pixels.data());
    return pixels;
}

Status Texture::generateMipmaps()
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }
    if (record->desc.levels <= 1u)
    {
        return failure("this texture was created with a single level of detail, "
                       "so there are no smaller ones to build. Ask for levels = "
                       "0 when creating it to get the whole chain");
    }

    backend::generateMipmaps(record->native);
    return success();
}

Status Texture::setFilter(Filter p_magnify, Filter p_minify)
{
    detail::TextureRecord* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }

    backend::setTextureFilter(
        record->native, p_magnify, p_minify, record->desc.levels > 1u);
    record->desc.magnify = p_magnify;
    record->desc.minify = p_minify;
    return success();
}

Status Texture::setWrap(Wrap p_x, Wrap p_y, Wrap p_z)
{
    detail::TextureRecord* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }

    backend::setTextureWrap(record->native, p_x, p_y, p_z);
    record->desc.wrap_x = p_x;
    record->desc.wrap_y = p_y;
    record->desc.wrap_z = p_z;
    return success();
}

void Texture::bind(std::uint32_t p_unit) const
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return reportError(staleTextureMessage());
    }

    const auto units = static_cast<std::uint32_t>(device().max_texture_units);
    if (p_unit >= units)
    {
        return reportError("texture unit " + std::to_string(p_unit) +
                       " does not exist: this driver offers " +
                       std::to_string(units));
    }

    backend::bindTexture(record->native, p_unit);
}

Status Texture::bindAsImage(std::uint32_t p_unit,
                            ImageAccess p_access,
                            std::uint32_t p_level) const
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    if (record == nullptr)
    {
        return failure(staleTextureMessage());
    }
    if (p_level >= record->desc.levels)
    {
        return failure("level of detail " + std::to_string(p_level) +
                       " does not exist: this texture has " +
                       std::to_string(record->desc.levels));
    }

    // Only some formats may be written to as an image, and the ones with three
    // channels are never among them: the hardware has no way to address them by
    // whole pixel. This is worth catching here, since the symptom is a compute
    // pass that writes nothing.
    if ((record->desc.format == PixelFormat::RGB8) ||
        (record->desc.format == PixelFormat::SRGB8))
    {
        return failure(
            std::string("a texture of ") + toString(record->desc.format) +
            " cannot be bound as an image: three channel formats are not among "
            "the ones a shader may write to. Use RGBA8 instead");
    }
    if (isDepth(record->desc.format))
    {
        return failure("a depth texture cannot be bound as an image");
    }

    backend::bindTextureAsImage(
        record->native, record->desc, p_unit, p_access, p_level);
    return success();
}

TextureDesc const& Texture::description() const
{
    static const TextureDesc nothing;
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    return (record == nullptr) ? nothing : record->desc;
}

std::uint32_t Texture::width() const
{
    return description().width;
}

std::uint32_t Texture::height() const
{
    return description().height;
}

std::uint32_t Texture::depth() const
{
    return description().depth;
}

PixelFormat Texture::format() const
{
    return description().format;
}

TextureKind Texture::kind() const
{
    return description().kind;
}

std::uint32_t Texture::levels() const
{
    return description().levels;
}

std::size_t Texture::bytes() const
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    return (record == nullptr) ? 0u : record->bytes;
}

bool Texture::valid() const
{
    return detail::pools().textures.valid(m_handle);
}

std::uintptr_t Texture::nativeId() const
{
    detail::TextureRecord const* record = detail::pools().textures.get(m_handle);
    return (record == nullptr) ? 0u : static_cast<std::uintptr_t>(record->native);
}

std::size_t liveTextures()
{
    return detail::pools().textures.size();
}

std::size_t textureMemory()
{
    std::size_t total = 0u;
    detail::pools().textures.forEach(
        [&total](TextureHandle, detail::TextureRecord const& p_record) {
            total += p_record.bytes;
        });
    return total;
}

} // namespace compages::gpu
