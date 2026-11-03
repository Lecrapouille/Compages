// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief What a pixel of a texture is made of, and how a texture is read.
//!
//! A texture is an image stored on the graphics card: a photograph, a depth
//! map, or a grid of numbers a simulation works on. Before creating one, the
//! library needs to know its shape (one image, six for a sky, a volume...),
//! what each pixel holds (colour, depth, a number) and how to read between
//! pixels.
//!
//! \code
//! auto desc = compages::gpu::TextureDesc::image(
//!                 256, 256, compages::gpu::PixelFormat::RGBA16F) // above 1
//!                 .filter(compages::gpu::Filter::Nearest)  // no blending
//!                 .wrap(compages::gpu::Wrap::Repeat);      // tiles
//! \endcode
//!
//! What it holds:
//! - TextureKind, PixelFormat, Filter, Wrap, ImageAccess: the choices.
//! - bytesPerPixel(), isDepth(), isSrgb()...: questions about a format.
// ****************************************************************************

#include <cstddef>
#include <cstdint>

namespace compages::gpu
{

// ----------------------------------------------------------------------------
//! \brief What shape a texture has.
//!
//! One enum rather than one class per shape. What changes between a 2D texture
//! and a cube map is how many images it holds and how a shader addresses them,
//! not what the library has to do, so they share everything except this value.
// ----------------------------------------------------------------------------
enum class TextureKind
{
    //! \brief A single row of pixels. Used for lookup tables and colour ramps.
    Texture1D,
    //! \brief One image, addressed by two coordinates. The common case.
    Texture2D,
    //! \brief A volume, addressed by three coordinates. Used by volume
    //! rendering and by simulations that live in three dimensions.
    Texture3D,
    //! \brief Six square images, addressed by a direction. Used for skyboxes
    //! and for reflections.
    TextureCube,
    //! \brief Several images of the same size, addressed by two coordinates and
    //! a layer number. Unlike a 3D texture, filtering never mixes two layers,
    //! which is what makes it right for a set of unrelated images.
    Texture2DArray,
};

// ----------------------------------------------------------------------------
//! \brief What one pixel holds.
//!
//! The name says the channels, the width of each, and how they are read. R8 is
//! one byte read as a fraction between 0 and 1; R8UI is one byte read as the
//! whole number it is. That distinction matters: a shader sampling an integer
//! format with a float sampler reads nothing sensible, and it is the kind of
//! mistake that shows as a black image rather than as an error.
// ----------------------------------------------------------------------------
enum class PixelFormat
{
    //! \brief One byte, as a fraction. A mask, a height field, an occlusion
    //! map.
    R8,
    //! \brief Two bytes, as fractions.
    RG8,
    //! \brief Three bytes, as fractions. Worth avoiding: most hardware pads it
    //! to four anyway, and reading three bytes per pixel is slower than four.
    RGB8,
    //! \brief Four bytes, as fractions. What a photograph loads as.
    RGBA8,

    //! \brief Three bytes, as fractions, with the colour channels stored with
    //! a gamma curve and converted on read. What a texture painted by an artist
    //! should use, so that filtering happens on linear values.
    SRGB8,
    //! \brief The same with an alpha channel, which stays linear.
    SRGB8A8,

    //! \brief One byte, as a whole number. For data that is a number, not a
    //! colour, such as the state of a cell in a simulation.
    R8UI,
    //! \brief One 32 bit signed whole number.
    R32I,
    //! \brief One 32 bit unsigned whole number.
    R32UI,

    //! \brief One half precision float. Half the memory of R32F, and enough for
    //! most simulations.
    R16F,
    //! \brief Two half precision floats.
    RG16F,
    //! \brief Four half precision floats. The usual choice for a render target
    //! holding values beyond 1, as in high dynamic range.
    RGBA16F,

    //! \brief One single precision float.
    R32F,
    //! \brief Two single precision floats.
    RG32F,
    //! \brief Four single precision floats. What a reaction diffusion
    //! simulation keeps its state in, where precision decides whether the
    //! pattern survives.
    RGBA32F,

    //! \brief Sixteen bits of depth.
    Depth16,
    //! \brief A single precision float of depth. The right choice for a large
    //! scene, where an integer depth buffer runs out of precision far away.
    Depth32F,
    //! \brief Twenty four bits of depth and eight of stencil, in one image.
    Depth24Stencil8,
};

// ----------------------------------------------------------------------------
//! \brief How a texture is read between its pixels.
// ----------------------------------------------------------------------------
enum class Filter
{
    //! \brief Take the nearest pixel. What a simulation and a pixel art texture
    //! want, since anything else invents values that were never computed.
    Nearest,
    //! \brief Mix the neighbouring pixels. What a photograph wants.
    Linear,
};

// ----------------------------------------------------------------------------
//! \brief What happens outside the texture.
// ----------------------------------------------------------------------------
enum class Wrap
{
    //! \brief Start over, tiling the texture.
    Repeat,
    //! \brief Tile, flipping every other copy, so that the seams line up.
    MirroredRepeat,
    //! \brief Hold the pixel at the edge. What a texture meant to be seen once
    //! wants, since repeating would show the opposite edge bleeding in.
    ClampToEdge,
};

// ----------------------------------------------------------------------------
//! \brief What a shader may do with a texture bound as an image.
//!
//! An image is how a compute shader writes to a texture. Saying which way the
//! data flows lets the driver place the memory accordingly, and lets the
//! library ask for the right barrier afterwards.
// ----------------------------------------------------------------------------
enum class ImageAccess
{
    //! \brief The shader only reads it.
    Read,
    //! \brief The shader only writes it.
    Write,
    //! \brief Both, which is what a simulation updating in place needs.
    ReadWrite,
};

// ----------------------------------------------------------------------------
//! \brief Name of the format as written in this enum ("RGBA8"), for error
//! messages.
//! \param[in] p_format the format to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Name of the shape as written in this enum, for error messages.
//! \param[in] p_kind the shape to name.
//! \return a static string, never null.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(TextureKind p_kind);

// ----------------------------------------------------------------------------
//! \brief How many bytes one pixel occupies.
//!
//! What a write has to be checked against: the size of the pixels handed over
//! must match the area being written, and saying so needs this. Depth24Stencil8
//! counts as four.
//!
//! \param[in] p_format the format to measure.
//! \return the bytes per pixel, 1 to 16.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t bytesPerPixel(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief How many channels one pixel has, depth counting as one: 3 for RGB8,
//! 4 for SRGB8A8.
//! \param[in] p_format the format to inspect.
//! \return 1 to 4.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t channelsOf(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Does this format hold depth rather than colour? A depth texture
//! can be attached as the depth of a framebuffer, not as one of its colours.
//! \param[in] p_format the format to inspect.
//! \return true for Depth16, Depth32F and Depth24Stencil8.
// ----------------------------------------------------------------------------
[[nodiscard]] bool isDepth(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Does this format hold a stencil as well? Only Depth24Stencil8 does.
//! \param[in] p_format the format to inspect.
//! \return true for Depth24Stencil8.
// ----------------------------------------------------------------------------
[[nodiscard]] bool hasStencil(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Is this format read as whole numbers rather than as fractions?
//!
//! A shader samples one of these with an isampler or a usampler, never with a
//! plain sampler.
//!
//! \param[in] p_format the format to inspect.
//! \return true for R8UI, R32I and R32UI.
// ----------------------------------------------------------------------------
[[nodiscard]] bool isIntegerFormat(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Are the colour channels stored with a gamma curve? Such a format
//! is converted to linear when sampled and back when written, so the shader
//! always works in linear values.
//! \param[in] p_format the format to inspect.
//! \return true for SRGB8 and SRGB8A8.
// ----------------------------------------------------------------------------
[[nodiscard]] bool isSrgb(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief How many images a texture of this shape holds beyond its width and
//! height: six for a cube map, one otherwise.
//! \param[in] p_kind the shape to inspect.
//! \return 6 for TextureCube, 1 otherwise.
// ----------------------------------------------------------------------------
[[nodiscard]] std::uint32_t facesOf(TextureKind p_kind);

// ----------------------------------------------------------------------------
//! \brief How many coordinates address a texture of this shape: one, two or
//! three. A cube map and an array are addressed as three.
//! \param[in] p_kind the shape to inspect.
//! \return 1, 2 or 3.
// ----------------------------------------------------------------------------
[[nodiscard]] std::uint8_t dimensionsOf(TextureKind p_kind);

} // namespace compages::gpu
