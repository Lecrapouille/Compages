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

#pragma once

#include <cstddef>
#include <cstdint>

// ****************************************************************************
//! \file
//! \brief What a pixel is made of, and how a texture is read.
// ****************************************************************************

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
    //! \brief A volume, addressed by three coordinates. Used by volume rendering
    //! and by simulations that live in three dimensions.
    Texture3D,
    //! \brief Six square images, addressed by a direction. Used for skyboxes and
    //! for reflections.
    TextureCube,
    //! \brief Several images of the same size, addressed by two coordinates and a
    //! layer number. Unlike a 3D texture, filtering never mixes two layers, which
    //! is what makes it right for a set of unrelated images.
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
    //! \brief One byte, as a fraction. A mask, a height field, an occlusion map.
    R8,
    //! \brief Two bytes, as fractions.
    RG8,
    //! \brief Three bytes, as fractions. Worth avoiding: most hardware pads it
    //! to four anyway, and reading three bytes per pixel is slower than four.
    RGB8,
    //! \brief Four bytes, as fractions. What a photograph loads as.
    RGBA8,

    //! \brief Four bytes, as fractions, with the colour channels stored with a
    //! gamma curve and converted on read. What a texture painted by an artist
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
    //! \brief Four single precision floats. What a reaction diffusion simulation
    //! keeps its state in, where precision decides whether the pattern survives.
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
//! data flows lets the driver place the memory accordingly, and lets the library
//! ask for the right barrier afterwards.
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
//! \brief Name of the format, for error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Name of the shape, for error messages.
// ----------------------------------------------------------------------------
[[nodiscard]] const char* toString(TextureKind p_kind);

// ----------------------------------------------------------------------------
//! \brief How many bytes one pixel occupies.
//!
//! What a write has to be checked against: the size of the pixels handed over
//! must match the area being written, and saying so needs this.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t bytesPerPixel(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief How many channels one pixel has, depth counting as one.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t channelsOf(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Does this format hold depth rather than colour?
// ----------------------------------------------------------------------------
[[nodiscard]] bool isDepth(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Does this format hold a stencil as well?
// ----------------------------------------------------------------------------
[[nodiscard]] bool hasStencil(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Is this format read as whole numbers rather than as fractions?
//!
//! A shader samples one of these with an isampler or a usampler, never with a
//! plain sampler.
// ----------------------------------------------------------------------------
[[nodiscard]] bool isIntegerFormat(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief Are the colour channels stored with a gamma curve?
// ----------------------------------------------------------------------------
[[nodiscard]] bool isSrgb(PixelFormat p_format);

// ----------------------------------------------------------------------------
//! \brief How many images a texture of this shape holds beyond its width and
//! height: six for a cube map, one otherwise.
// ----------------------------------------------------------------------------
[[nodiscard]] std::uint32_t facesOf(TextureKind p_kind);

// ----------------------------------------------------------------------------
//! \brief How many coordinates address a texture of this shape: one, two or
//! three. A cube map and an array are addressed as three.
// ----------------------------------------------------------------------------
[[nodiscard]] std::uint8_t dimensionsOf(TextureKind p_kind);

} // namespace compages::gpu
