// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Pictures stored on the graphics card: images, cube maps, volumes.
//!
//! Anything a shape shows on its surface comes from a texture: a photograph,
//! a painted material, the six faces of a sky. A texture can also be a blank
//! sheet the card draws into (see Framebuffer), or a grid of numbers a
//! simulation updates every frame.
//!
//! What it holds:
//! - Texture: the image, loaded from a file or made empty.
//! - TextureDesc: its shape, size and pixel format.
//! - LoadOptions: what to do to a file while loading it.
//! - liveTextures(), textureMemory(): to spot leaks and watch memory.
// ****************************************************************************

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/GPU/Handle.hpp"
#include "Compages/GPU/PixelFormat.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace compages::gpu
{

//! \brief Names an image living on the device.
using TextureHandle = Handle<struct TextureTag>;

// ****************************************************************************
//! \brief What a texture is to be: its shape, its size and what a pixel holds.
//!
//! The order form filled in before a texture is made: how big, which kind of
//! pixels, how to read it.
//!
//! One description covering every shape, rather than a class per shape. A cube
//! map differs from a 2D texture by holding six images, and an array by holding
//! several layers, which is a value here rather than a type of its own.
// ****************************************************************************
struct TextureDesc
{
    //! \brief How many images and how they are addressed.
    TextureKind kind = TextureKind::Texture2D;

    //! \brief What one pixel holds.
    PixelFormat format = PixelFormat::RGBA8;

    //! \brief How many pixels across.
    std::uint32_t width = 0u;

    //! \brief How many pixels down. One for a 1D texture.
    std::uint32_t height = 1u;

    //! \brief How deep for a 3D texture, or how many layers for an array.
    //! Always one for a 2D texture, and six is implied for a cube map whatever
    //! is put here.
    std::uint32_t depth = 1u;

    //! \brief How many levels of detail, each half the size of the one before.
    //! One means no mipmaps; zero means as many as the size allows, which is
    //! what a texture meant to be seen at a distance wants.
    std::uint32_t levels = 1u;

    //! \brief How the texture is read when magnified.
    Filter magnify = Filter::Linear;

    //! \brief How the texture is read when reduced.
    Filter minify = Filter::Linear;

    //! \brief What happens beyond the left and right edges.
    Wrap wrap_x = Wrap::ClampToEdge;

    //! \brief What happens beyond the top and bottom edges.
    Wrap wrap_y = Wrap::ClampToEdge;

    //! \brief What happens beyond the front and back of a 3D texture.
    Wrap wrap_z = Wrap::ClampToEdge;

    // ------------------------------------------------------------------------
    //! \brief A 2D image of this size: the common case, in one call. The
    //! other fields keep their defaults and can be chained:
    //! \code
    //! COMPAGES_TRY_ASSIGN(m_texture, compages::gpu::Texture::create(
    //!     compages::gpu::TextureDesc::image(256u, 256u)
    //!         .filter(compages::gpu::Filter::Nearest)
    //!         .wrap(compages::gpu::Wrap::Repeat)));
    //! \endcode
    //!
    //! \param[in] p_width pixels across.
    //! \param[in] p_height pixels down.
    //! \param[in] p_format what one pixel holds.
    //! \return the description, to chain filter() and wrap().
    // ------------------------------------------------------------------------
    [[nodiscard]] static TextureDesc
    image(std::uint32_t p_width,
          std::uint32_t p_height,
          PixelFormat p_format = PixelFormat::RGBA8)
    {
        TextureDesc desc;
        desc.format = p_format;
        desc.width = p_width;
        desc.height = p_height;
        return desc;
    }

    //! \brief Read the same way magnified and reduced.
    //! \param[in] p_filter Nearest for sharp pixels, Linear for smooth ones.
    //! \return this description, so calls can be chained.
    TextureDesc& filter(Filter p_filter)
    {
        magnify = p_filter;
        minify = p_filter;
        return *this;
    }

    //! \brief Do the same beyond every edge.
    //! \param[in] p_wrap what happens outside the image on every axis.
    //! \return this description, so calls can be chained.
    TextureDesc& wrap(Wrap p_wrap)
    {
        wrap_x = p_wrap;
        wrap_y = p_wrap;
        wrap_z = p_wrap;
        return *this;
    }
};

// ****************************************************************************
//! \brief What to do to an image while loading it from a file.
//!
//! Two presets cover most files: picture() for what is meant to be looked at,
//! data() for what holds numbers.
//!
//! \code
//! COMPAGES_TRY_ASSIGN(m_tiles, compages::gpu::Texture::fromFile(
//!     "tiles.png", compages::gpu::LoadOptions::picture().nearest()));
//! \endcode
// ****************************************************************************
struct LoadOptions
{
    //! \brief Turn the image upside down.
    //!
    //! Image files start at the top row while a texture coordinate of zero is
    //! at the bottom, so a texture loaded as it comes out of the file appears
    //! flipped. Doing it here means it is done once, rather than in every
    //! shader that samples the texture.
    bool flip_vertically = true;

    //! \brief Build the smaller levels of detail after loading.
    bool mipmaps = true;

    //! \brief Read the colour channels as gamma corrected, which is what a file
    //! painted by hand holds. Leave it off for a file holding data, such as a
    //! normal map or a height field, where the values are numbers, not light.
    bool srgb = false;

    //! \brief How the texture is read between its pixels.
    Filter filter = Filter::Linear;

    //! \brief What happens outside the texture.
    Wrap wrap = Wrap::Repeat;

    // ------------------------------------------------------------------------
    //! \brief For a picture meant to be looked at, a photograph or a painted
    //! material: its colours are gamma corrected.
    //! \code
    //! COMPAGES_TRY_ASSIGN(m_grass, compages::gpu::Texture::fromFile(
    //!     dataPath("grass.png"), compages::gpu::LoadOptions::picture()));
    //! \endcode
    //! \return options for a picture: sRGB on.
    // ------------------------------------------------------------------------
    [[nodiscard]] static LoadOptions picture()
    {
        LoadOptions options;
        options.srgb = true;
        return options;
    }

    //! \brief For a file holding numbers rather than light: a blend map, a
    //! normal map, a height field. The defaults.
    //! \return options for data: sRGB off.
    [[nodiscard]] static LoadOptions data()
    {
        return LoadOptions{};
    }

    //! \brief Stretch the edge pixels rather than repeating the image. For a
    //! picture shown once, whose opposite edge would otherwise bleed in.
    //! \return these options, so calls can be chained.
    LoadOptions& clampToEdge()
    {
        wrap = Wrap::ClampToEdge;
        return *this;
    }

    //! \brief Keep the pixels square rather than blending neighbours. For
    //! pixel art, and for data where a blend would invent values.
    //! \return these options, so calls can be chained.
    LoadOptions& nearest()
    {
        filter = Filter::Nearest;
        return *this;
    }
};

// ****************************************************************************
//! \brief A picture stored on the graphics card, ready to be painted onto
//! shapes or drawn into.
//!
//! A texture is what a shader reads when it gives a surface its look: the
//! grain of wood on a crate, the sky around a scene. It can also be a blank
//! sheet the card draws into, or a grid of numbers a simulation updates.
//!
//! \code
//! // Loaded from a file, with its mipmaps built.
//! compages::gpu::Texture wood;
//! COMPAGES_TRY_ASSIGN(wood,
//! compages::gpu::Texture::fromFile("textures/wood.png"));
//!
//! // Or made empty, to be written into or rendered to.
//! compages::gpu::Texture state;
//! COMPAGES_TRY_ASSIGN(state, compages::gpu::Texture::create(
//!     compages::gpu::TextureDesc::image(512u, 512u,
//!                                       compages::gpu::PixelFormat::RGBA32F)
//!         .filter(compages::gpu::Filter::Nearest)));
//! \endcode
//!
//! A texture is made by create(), fromFile(), cubeFromFiles() or
//! volumeFromFiles(), as a Buffer is by from() or create(): what it is never
//! changes afterwards.
//!
//! The memory is set aside once, when the texture is created, and its size and
//! format never change afterwards. That is deliberate: a texture whose format
//! can be changed under a framebuffer or a pipeline that refers to it is a
//! source of failures that appear far from their cause. To change the size,
//! make another one.
// ****************************************************************************
class Texture: private compages::NotCopiable
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty texture: valid() is false until one made by create(),
    //! fromFile(), cubeFromFiles() or volumeFromFiles() is moved in.
    // ------------------------------------------------------------------------
    Texture() = default;

    // ------------------------------------------------------------------------
    //! \brief Set aside the memory for a texture, without filling it.
    //!
    //! \param[in] p_desc what the texture is to be.
    //! \return the texture, or why it could not be made: a size of zero, a size
    //! beyond what the driver allows, or a shape whose sizes do not agree, such
    //! as a cube map that is not square.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture> create(TextureDesc const& p_desc);

    // ------------------------------------------------------------------------
    //! \brief Read an image file into a 2D texture.
    //!
    //! Reads whatever stb_image reads, which covers PNG, JPEG, BMP, TGA, GIF,
    //! HDR and a few more. The number of channels in the file decides the
    //! format: one channel becomes R8, three become RGB8, four RGBA8, with the
    //! sRGB variants when asked for.
    //!
    //! \param[in] p_path the file to read.
    //! \param[in] p_options what to do to it on the way in.
    //! \return the texture, or why not: the file is missing, or is not an image
    //! stb_image understands, in which case its own words are passed on.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture>
    fromFile(std::filesystem::path const& p_path,
             LoadOptions const& p_options = {});

    // ------------------------------------------------------------------------
    //! \brief Read six image files into a cube map.
    //!
    //! \param[in] p_paths the six faces, in the order the hardware expects
    //! them: positive x, negative x, positive y, negative y, positive z,
    //! negative z.
    //! \param[in] p_options what to do to them on the way in. Flipping is off
    //! by default for a cube map, whose faces are stored the way files are.
    //! \return the cube map, or why not: a missing file, or faces that are not
    //! all square and of the same size.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture>
    cubeFromFiles(std::array<std::filesystem::path, 6u> const& p_paths,
                  LoadOptions const& p_options = { .flip_vertically = false,
                                                   .wrap = Wrap::ClampToEdge });

    // ------------------------------------------------------------------------
    //! \brief Read image files of the same size into the layers of a 3D
    //! texture, the first file at depth zero.
    //!
    //! A shader samples it with three coordinates; the third blends between
    //! the images, which is how a terrain goes from sand to snow:
    //! \code
    //! COMPAGES_TRY_ASSIGN(m_layers, compages::gpu::Texture::volumeFromFiles(
    //!     { dataPath("sand.png"), dataPath("snow.png") }));
    //! \endcode
    //!
    //! No mipmaps by default: the smaller levels of a 3D texture also halve
    //! its depth, which would blend the layers together from afar.
    //!
    //! \param[in] p_paths the layers, in depth order, all the same size.
    //! \param[in] p_options what to do to them on the way in.
    //! \return the texture, or why not: a missing file or a size that differs.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture> volumeFromFiles(
        std::vector<std::filesystem::path> const& p_paths,
        LoadOptions const& p_options = { .mipmaps = false,
                                         .wrap = Wrap::ClampToEdge });

    //! \brief Take over the image of p_other, which is left not valid().
    //! \param[in,out] p_other the texture to empty.
    Texture(Texture&& p_other) noexcept;
    //! \brief Release this image, then take over p_other's.
    //! \param[in,out] p_other the texture to empty.
    //! \return this object, now owning the other image.
    Texture& operator=(Texture&& p_other) noexcept;
    //! \brief Release the image. A framebuffer still naming it fails on its
    //! next pass.
    ~Texture();

    // ------------------------------------------------------------------------
    //! \brief Give the memory back now rather than at the end of the scope.
    //! A new texture can then be moved in, from create() or a load.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Fill the whole of one level of detail.
    //!
    //! \param[in] p_pixels the pixels, row by row from the bottom, as many
    //! bytes as the level needs. A count that does not match is refused rather
    //! than read past its end.
    //! \param[in] p_level which level of detail, zero being the largest.
    //! \return an error on a byte count that does not match, or a released
    //! texture.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status write(std::span<const std::byte> p_pixels,
                               std::uint32_t p_level = 0u);

    // ------------------------------------------------------------------------
    //! \brief Fill part of one level of detail.
    //!
    //! \param[in] p_x, p_y, p_z where the region starts. The z is the layer for
    //! an array and the face for a cube map.
    //! \param[in] p_width, p_height, p_depth how large the region is.
    //! \param[in] p_pixels the pixels, as many bytes as the region needs.
    //! \param[in] p_level which level of detail.
    //! \return an error when the region runs past the level or the byte count
    //! does not match.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status write(std::uint32_t p_x,
                               std::uint32_t p_y,
                               std::uint32_t p_z,
                               std::uint32_t p_width,
                               std::uint32_t p_height,
                               std::uint32_t p_depth,
                               std::span<const std::byte> p_pixels,
                               std::uint32_t p_level = 0u);

    // ------------------------------------------------------------------------
    //! \brief Fill one face of a cube map, or one layer of an array, whole.
    //!
    //! \param[in] p_layer the face (0 to 5, in the order +x, -x, +y, -y, +z,
    //! -z) or the layer.
    //! \param[in] p_pixels the pixels of one face or layer.
    //! \param[in] p_level which level of detail.
    //! \return an error on a layer past the last one or a byte count that does
    //! not match.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status writeLayer(std::uint32_t p_layer,
                                    std::span<const std::byte> p_pixels,
                                    std::uint32_t p_level = 0u);

    // ------------------------------------------------------------------------
    //! \brief Read a level of detail back into CPU memory.
    //!
    //! Waits for the device to finish with the image, so this belongs in tests,
    //! in screenshots and in debugging rather than in a frame.
    //!
    //! \param[in] p_level which level of detail.
    //! \return the pixels of that level, bottom row first, as write() takes
    //! them; an error for a level the texture does not have.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<std::vector<std::byte>>
    read(std::uint32_t p_level = 0u) const;

    // ------------------------------------------------------------------------
    //! \brief Build the smaller levels of detail from the largest one.
    //!
    //! Needed after writing to a texture created with more than one level;
    //! fromFile() does it when LoadOptions::mipmaps is on.
    //! \return an error when the texture has a single level.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status generateMipmaps();

    // ------------------------------------------------------------------------
    //! \brief Make the texture readable by a shader on the given texture unit.
    //!
    //! The unit is the number a sampler uniform is set to: bind on unit 3 and
    //! set the sampler to 3. compages::gpu::Drawable does both when a texture
    //! is assigned to a sampler by name.
    //!
    //! A released texture or a unit the driver does not have is recorded as the
    //! frame error (see Errors.hpp).
    //!
    //! \param[in] p_unit the texture unit, from 0 to
    //! DeviceInfo::max_texture_units - 1.
    // ------------------------------------------------------------------------
    void bind(std::uint32_t p_unit) const;

    // ------------------------------------------------------------------------
    //! \brief Make the texture writable by a shader, as an image.
    //!
    //! This is how a compute pass produces a texture: unlike a sampler, an
    //! image is addressed by whole pixel and may be written to. The unit is the
    //! number given in the shader's `layout(binding = ...)` on the image
    //! uniform.
    //!
    //! \param[in] p_unit which image unit.
    //! \param[in] p_access what the shader is going to do with it.
    //! \param[in] p_level which level of detail to expose.
    //! \return an error on a unit the driver does not have, or a released
    //! texture.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bindAsImage(std::uint32_t p_unit,
                                     ImageAccess p_access,
                                     std::uint32_t p_level = 0u) const;

    // ------------------------------------------------------------------------
    //! \brief The description it was made with, which never changes.
    //! \return the description.
    // ------------------------------------------------------------------------
    [[nodiscard]] TextureDesc const& description() const;

    //! \brief How many pixels across, at level 0.
    //! \return the width in pixels.
    [[nodiscard]] std::uint32_t width() const;
    //! \brief How many pixels down, at level 0. 1 for a 1D texture.
    //! \return the height in pixels.
    [[nodiscard]] std::uint32_t height() const;
    //! \brief How deep, or how many layers. 1 for a 2D texture.
    //! \return the depth or the layer count.
    [[nodiscard]] std::uint32_t depth() const;
    //! \brief What one pixel holds.
    //! \return the pixel format.
    [[nodiscard]] PixelFormat format() const;
    //! \brief What shape it is.
    //! \return the texture kind.
    [[nodiscard]] TextureKind kind() const;
    //! \brief How many levels of detail it holds, the levels = 0 of a
    //! description resolved to the real count.
    //! \return the number of levels, at least 1.
    [[nodiscard]] std::uint32_t levels() const;

    // ------------------------------------------------------------------------
    //! \brief How many bytes the whole texture occupies on the device, every
    //! level and every face counted.
    //!
    //! What an overlay shows to say how much memory a scene is using.
    //! \return the bytes on the card, an estimate.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const;

    // ------------------------------------------------------------------------
    //! \brief False when default constructed, moved from, released, or after
    //! gpu::shutdown().
    //! \return true when the image exists.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief The handle naming this image, for a Framebuffer attachment or
    //! code that keeps it without owning it.
    //! \return the handle; empty when not valid().
    // ------------------------------------------------------------------------
    [[nodiscard]] TextureHandle handle() const
    {
        return m_handle;
    }

    // ------------------------------------------------------------------------
    //! \brief What the driver calls the image, or zero when there is none.
    //!
    //! Only for a library sharing the context that takes a texture by its
    //! driver name, such as Dear ImGui showing a picture:
    //! \code
    //! ImGui::Image(ImTextureID(m_color.nativeId()), size);
    //! \endcode
    //! \return the driver name, 0 when not valid().
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uintptr_t nativeId() const;

private:

    explicit Texture(TextureHandle p_handle) : m_handle(p_handle) {}

    TextureHandle m_handle;
};

// ----------------------------------------------------------------------------
//! \brief How many textures are alive, every kind together. Used to spot
//! leaks.
//! \return the number of live textures.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t liveTextures();

// ----------------------------------------------------------------------------
//! \brief How many bytes every live texture occupies, added up: an estimate,
//! since the driver may pad or compress.
//! \return the bytes, added up.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t textureMemory();

} // namespace compages::gpu
