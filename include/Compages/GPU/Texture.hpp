// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/Core/Handle.hpp"
#include "Compages/GPU/Core/PixelFormat.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
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

    //! \brief How deep for a 3D texture, or how many layers for an array. Always
    //! one for a 2D texture, and six is implied for a cube map whatever is put
    //! here.
    std::uint32_t depth = 1u;

    //! \brief How many levels of detail, each half the size of the one before.
    //! One means no mipmaps; zero means as many as the size allows, which is what
    //! a texture meant to be seen at a distance wants.
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
    //! COMPAGES_TRY(m_texture.allocate(
    //!     compages::gpu::TextureDesc::image(256u, 256u).filter(compages::gpu::Filter::Nearest)
    //!                                       .wrap(compages::gpu::Wrap::Repeat)));
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] static TextureDesc image(std::uint32_t p_width,
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
    TextureDesc& filter(Filter p_filter)
    {
        magnify = p_filter;
        minify = p_filter;
        return *this;
    }

    //! \brief Do the same beyond every edge.
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
// ****************************************************************************
struct LoadOptions
{
    //! \brief Turn the image upside down.
    //!
    //! Image files start at the top row while a texture coordinate of zero is at
    //! the bottom, so a texture loaded as it comes out of the file appears
    //! flipped. Doing it here means it is done once, rather than in every shader
    //! that samples the texture.
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
    //! COMPAGES_TRY(m_grass.load(dataPath("grass.png"), compages::gpu::LoadOptions::picture()));
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] static LoadOptions picture()
    {
        LoadOptions options;
        options.srgb = true;
        return options;
    }

    //! \brief For a file holding numbers rather than light: a blend map, a
    //! normal map, a height field. The defaults.
    [[nodiscard]] static LoadOptions data()
    {
        return LoadOptions{};
    }

    //! \brief Stretch the edge pixels rather than repeating the image.
    LoadOptions& clampToEdge()
    {
        wrap = Wrap::ClampToEdge;
        return *this;
    }

    //! \brief Keep the pixels square rather than blending neighbours.
    LoadOptions& nearest()
    {
        filter = Filter::Nearest;
        return *this;
    }
};

// ****************************************************************************
//! \brief An image living on the device, of any shape.
//!
//! \code
//! // Loaded from a file, with its mipmaps built.
//! compages::gpu::Texture wood;
//! COMPAGES_TRY(wood.load("textures/wood.png"));
//!
//! // Or made empty, to be written into or rendered to.
//! compages::gpu::Texture state;
//! COMPAGES_TRY(state.allocate({
//!     .format = compages::gpu::PixelFormat::RGBA32F,
//!     .width = 512u, .height = 512u,
//!     .magnify = compages::gpu::Filter::Nearest, .minify = compages::gpu::Filter::Nearest }));
//! \endcode
//!
//! The in place load(), loadCube() and allocate() suit a member. The static
//! fromFile(), cubeFromFiles() and create() return the texture as a Result,
//! which suits code filling a container.
//!
//! The memory is set aside once, when the texture is created, and its size and
//! format never change afterwards. That is deliberate: a texture whose format can
//! be changed under a framebuffer or a pipeline that refers to it is a source of
//! failures that appear far from their cause. To change the size, make another
//! one.
// ****************************************************************************
class Texture
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty texture, owning nothing.
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
    //! HDR and a few more. The number of channels in the file decides the format:
    //! one channel becomes R8, three become RGB8, four RGBA8, with the sRGB
    //! variants when asked for.
    //!
    //! \param[in] p_path the file to read.
    //! \param[in] p_options what to do to it on the way in.
    //! \return the texture, or why not: the file is missing, or is not an image
    //! stb_image understands, in which case its own words are passed on.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture> fromFile(
        std::string const& p_path, LoadOptions const& p_options = {});

    // ------------------------------------------------------------------------
    //! \brief Read six image files into a cube map.
    //!
    //! \param[in] p_paths the six faces, in the order the hardware expects them:
    //! positive x, negative x, positive y, negative y, positive z, negative z.
    //! \param[in] p_options what to do to them on the way in. Flipping is off by
    //! default for a cube map, whose faces are stored the way files are.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture> cubeFromFiles(
        std::array<std::string, 6u> const& p_paths,
        LoadOptions const& p_options = { .flip_vertically = false,
                                         .wrap = Wrap::ClampToEdge });

    // ------------------------------------------------------------------------
    //! \brief Same as create(), into this object.
    //!
    //! \return why not, in which case this object is left as it was.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status allocate(TextureDesc const& p_desc);

    // ------------------------------------------------------------------------
    //! \brief Same as fromFile(), into this object.
    //!
    //! \code
    //! COMPAGES_TRY(m_texture.load(dataPath("textures/wooden-crate.jpg")));
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] Status load(std::string const& p_path,
                              LoadOptions const& p_options = {});

    // ------------------------------------------------------------------------
    //! \brief Same as cubeFromFiles(), into this object.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status loadCube(
        std::array<std::string, 6u> const& p_paths,
        LoadOptions const& p_options = { .flip_vertically = false,
                                         .wrap = Wrap::ClampToEdge });

    // ------------------------------------------------------------------------
    //! \brief Read image files of the same size into the layers of a 3D
    //! texture, the first file at depth zero.
    //!
    //! A shader samples it with three coordinates; the third blends between
    //! the images, which is how a terrain goes from sand to snow:
    //! \code
    //! COMPAGES_TRY(m_layers.loadVolume({ dataPath("sand.png"), dataPath("snow.png") }));
    //! \endcode
    //!
    //! No mipmaps by default: the smaller levels of a 3D texture also halve
    //! its depth, which would blend the layers together from afar.
    // ------------------------------------------------------------------------
    [[nodiscard]] static Result<Texture> volumeFromFiles(
        std::vector<std::string> const& p_paths,
        LoadOptions const& p_options = { .mipmaps = false,
                                         .wrap = Wrap::ClampToEdge });

    //! \brief Same as volumeFromFiles(), into this object.
    [[nodiscard]] Status loadVolume(
        std::vector<std::string> const& p_paths,
        LoadOptions const& p_options = { .mipmaps = false,
                                         .wrap = Wrap::ClampToEdge });

    Texture(Texture&& p_other) noexcept;
    Texture& operator=(Texture&& p_other) noexcept;
    Texture(Texture const&) = delete;
    Texture& operator=(Texture const&) = delete;
    ~Texture();

    // ------------------------------------------------------------------------
    //! \brief Give the memory back now rather than at the end of the scope.
    // ------------------------------------------------------------------------
    void release();

    // ------------------------------------------------------------------------
    //! \brief Fill the whole of one level of detail.
    //!
    //! \param[in] p_pixels the pixels, row by row from the bottom, as many bytes
    //! as the level needs. A count that does not match is refused rather than
    //! read past its end.
    //! \param[in] p_level which level of detail, zero being the largest.
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
    //! \brief Fill one face of a cube map, or one layer of an array.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status writeLayer(std::uint32_t p_layer,
                                    std::span<const std::byte> p_pixels,
                                    std::uint32_t p_level = 0u);

    // ------------------------------------------------------------------------
    //! \brief Read a level of detail back into CPU memory.
    //!
    //! Waits for the device to finish with the image, so this belongs in tests,
    //! in screenshots and in debugging rather than in a frame.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<std::vector<std::byte>> read(
        std::uint32_t p_level = 0u) const;

    // ------------------------------------------------------------------------
    //! \brief Build the smaller levels of detail from the largest one.
    //!
    //! Needed after writing to a texture created with more than one level.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status generateMipmaps();

    // ------------------------------------------------------------------------
    //! \brief Change how the texture is read between its pixels.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status setFilter(Filter p_magnify, Filter p_minify);

    // ------------------------------------------------------------------------
    //! \brief Change what happens outside the texture.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status setWrap(Wrap p_x, Wrap p_y, Wrap p_z = Wrap::ClampToEdge);

    // ------------------------------------------------------------------------
    //! \brief Make the texture readable by a shader on the given texture unit.
    //!
    //! The unit is the number a sampler uniform is set to: bind on unit 3 and set
    //! the sampler to 3. compages::gpu::Drawable does both when a texture is assigned to
    //! a sampler by name.
    //!
    //! A released texture or a unit the driver does not have is recorded as the
    //! frame error (see Errors.hpp).
    // ------------------------------------------------------------------------
    void bind(std::uint32_t p_unit) const;

    // ------------------------------------------------------------------------
    //! \brief Make the texture writable by a shader, as an image.
    //!
    //! This is how a compute pass produces a texture: unlike a sampler, an image
    //! is addressed by whole pixel and may be written to. The unit is the number
    //! given in the shader's `layout(binding = ...)` on the image uniform.
    //!
    //! \param[in] p_unit which image unit.
    //! \param[in] p_access what the shader is going to do with it.
    //! \param[in] p_level which level of detail to expose.
    // ------------------------------------------------------------------------
    [[nodiscard]] Status bindAsImage(std::uint32_t p_unit,
                                     ImageAccess p_access,
                                     std::uint32_t p_level = 0u) const;

    // ------------------------------------------------------------------------
    //! \brief What the texture is.
    // ------------------------------------------------------------------------
    [[nodiscard]] TextureDesc const& description() const;

    //! \brief How many pixels across.
    [[nodiscard]] std::uint32_t width() const;
    //! \brief How many pixels down.
    [[nodiscard]] std::uint32_t height() const;
    //! \brief How deep, or how many layers.
    [[nodiscard]] std::uint32_t depth() const;
    //! \brief What one pixel holds.
    [[nodiscard]] PixelFormat format() const;
    //! \brief What shape it is.
    [[nodiscard]] TextureKind kind() const;
    //! \brief How many levels of detail it holds.
    [[nodiscard]] std::uint32_t levels() const;

    // ------------------------------------------------------------------------
    //! \brief How many bytes the whole texture occupies on the device, every
    //! level and every face counted.
    //!
    //! What an overlay shows to say how much memory a scene is using.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t bytes() const;

    // ------------------------------------------------------------------------
    //! \brief Is there an image here?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid() const;

    // ------------------------------------------------------------------------
    //! \brief Name of the image, for the parts of the library that take one.
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
    // ------------------------------------------------------------------------
    [[nodiscard]] std::uintptr_t nativeId() const;

private:

    explicit Texture(TextureHandle p_handle) : m_handle(p_handle) {}

    TextureHandle m_handle;
};

// ----------------------------------------------------------------------------
//! \brief How many textures are alive. Used to spot leaks.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t liveTextures();

// ----------------------------------------------------------------------------
//! \brief How many bytes every live texture occupies, added up.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t textureMemory();

} // namespace compages::gpu
