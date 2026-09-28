// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/GPU/Core/PixelFormat.hpp"

#include <iterator>

namespace compages::gpu
{

namespace
{

//! \brief How the values of a format are read by a shader.
enum class Reading
{
    //! \brief A whole number turned into a fraction between 0 and 1.
    Normalized,
    //! \brief A whole number, as it is.
    Integer,
    //! \brief A floating point value.
    Float,
    //! \brief A fraction, with the colour channels gamma corrected on read.
    Srgb,
    //! \brief Depth, which is a fraction with its own rules.
    Depth,
};

// ****************************************************************************
//! \brief Everything worth knowing about one pixel format, in one row.
// ****************************************************************************
struct FormatInfo
{
    //! \brief Which format this row describes, so the table can be checked
    //! against the enum at compile time.
    PixelFormat format;
    //! \brief How it is named in messages.
    const char* name;
    //! \brief How many channels, depth counting as one.
    std::uint8_t channels;
    //! \brief How many bytes one pixel occupies.
    std::uint8_t bytes;
    //! \brief How a shader reads it.
    Reading reading;
    //! \brief Does it carry a stencil as well?
    bool stencil;
};

// ----------------------------------------------------------------------------
//! \brief One row per format, in the order the enum declares them.
// ----------------------------------------------------------------------------
constexpr FormatInfo FORMATS[] = {
    { PixelFormat::R8, "R8", 1u, 1u, Reading::Normalized, false },
    { PixelFormat::RG8, "RG8", 2u, 2u, Reading::Normalized, false },
    { PixelFormat::RGB8, "RGB8", 3u, 3u, Reading::Normalized, false },
    { PixelFormat::RGBA8, "RGBA8", 4u, 4u, Reading::Normalized, false },

    { PixelFormat::SRGB8, "SRGB8", 3u, 3u, Reading::Srgb, false },
    { PixelFormat::SRGB8A8, "SRGB8A8", 4u, 4u, Reading::Srgb, false },

    { PixelFormat::R8UI, "R8UI", 1u, 1u, Reading::Integer, false },
    { PixelFormat::R32I, "R32I", 1u, 4u, Reading::Integer, false },
    { PixelFormat::R32UI, "R32UI", 1u, 4u, Reading::Integer, false },

    { PixelFormat::R16F, "R16F", 1u, 2u, Reading::Float, false },
    { PixelFormat::RG16F, "RG16F", 2u, 4u, Reading::Float, false },
    { PixelFormat::RGBA16F, "RGBA16F", 4u, 8u, Reading::Float, false },

    { PixelFormat::R32F, "R32F", 1u, 4u, Reading::Float, false },
    { PixelFormat::RG32F, "RG32F", 2u, 8u, Reading::Float, false },
    { PixelFormat::RGBA32F, "RGBA32F", 4u, 16u, Reading::Float, false },

    { PixelFormat::Depth16, "Depth16", 1u, 2u, Reading::Depth, false },
    { PixelFormat::Depth32F, "Depth32F", 1u, 4u, Reading::Depth, false },
    { PixelFormat::Depth24Stencil8, "Depth24Stencil8", 2u, 4u, Reading::Depth,
      true },
};

static_assert(std::size(FORMATS) ==
                  static_cast<std::size_t>(PixelFormat::Depth24Stencil8) + 1u,
              "a PixelFormat was added without its row in the FORMATS table of "
              "PixelFormat.cpp");

// ----------------------------------------------------------------------------
//! \brief Every row must sit at the position of the format it describes.
// ----------------------------------------------------------------------------
constexpr bool tableIsInOrder()
{
    for (std::size_t i = 0u; i < std::size(FORMATS); ++i)
    {
        if (FORMATS[i].format != static_cast<PixelFormat>(i))
        {
            return false;
        }
    }
    return true;
}

static_assert(tableIsInOrder(),
              "the rows of the FORMATS table of PixelFormat.cpp are not in the "
              "order the PixelFormat enum declares them");

FormatInfo const& infoOf(PixelFormat p_format)
{
    const auto index = static_cast<std::size_t>(p_format);
    if (index >= std::size(FORMATS))
    {
        return FORMATS[static_cast<std::size_t>(PixelFormat::RGBA8)];
    }
    return FORMATS[index];
}

} // namespace

const char* toString(PixelFormat p_format)
{
    return infoOf(p_format).name;
}

const char* toString(TextureKind p_kind)
{
    switch (p_kind)
    {
        case TextureKind::Texture1D:
            return "1D texture";
        case TextureKind::Texture2D:
            return "2D texture";
        case TextureKind::Texture3D:
            return "3D texture";
        case TextureKind::TextureCube:
            return "cube map";
        case TextureKind::Texture2DArray:
            return "2D texture array";
    }
    return "texture";
}

std::size_t bytesPerPixel(PixelFormat p_format)
{
    return infoOf(p_format).bytes;
}

std::size_t channelsOf(PixelFormat p_format)
{
    return infoOf(p_format).channels;
}

bool isDepth(PixelFormat p_format)
{
    return infoOf(p_format).reading == Reading::Depth;
}

bool hasStencil(PixelFormat p_format)
{
    return infoOf(p_format).stencil;
}

bool isIntegerFormat(PixelFormat p_format)
{
    return infoOf(p_format).reading == Reading::Integer;
}

bool isSrgb(PixelFormat p_format)
{
    return infoOf(p_format).reading == Reading::Srgb;
}

std::uint32_t facesOf(TextureKind p_kind)
{
    return (p_kind == TextureKind::TextureCube) ? 6u : 1u;
}

std::uint8_t dimensionsOf(TextureKind p_kind)
{
    switch (p_kind)
    {
        case TextureKind::Texture1D:
            return 1u;
        case TextureKind::Texture2D:
            return 2u;
        // A cube map and an array both hold several images, and the driver treats
        // them as three dimensional for the purpose of writing into them.
        case TextureKind::Texture3D:
        case TextureKind::TextureCube:
        case TextureKind::Texture2DArray:
            return 3u;
    }
    return 2u;
}

} // namespace compages::gpu
