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

#include "Common/Screenshot.hpp"

// The one translation unit that holds the code of the writer, and the warnings
// it produces. Ours are left as strict as they are everywhere else.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Warith-conversion"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#pragma GCC diagnostic pop

namespace examples
{

//------------------------------------------------------------------------------
compages::Status writePng(std::string const& p_path,
                          std::uint32_t p_width,
                          std::uint32_t p_height,
                          std::span<const std::byte> p_pixels)
{
    const std::size_t needed =
        static_cast<std::size_t>(p_width) * p_height * 4u;
    if (p_pixels.size() < needed)
    {
        return compages::failure(
            "there are not enough pixels for a picture of this size: " +
            std::to_string(p_pixels.size()) + " bytes for " +
            std::to_string(needed) + " needed");
    }

    // The device hands the bottom row over first, and an image file holds the
    // top row first.
    stbi_flip_vertically_on_write(1);

    const int written = stbi_write_png(p_path.c_str(),
                                       static_cast<int>(p_width),
                                       static_cast<int>(p_height),
                                       4,
                                       p_pixels.data(),
                                       static_cast<int>(p_width * 4u));
    if (written == 0)
    {
        return compages::failure(
            "could not write " + p_path +
            ". Does the directory exist and is it writable?");
    }

    return compages::success();
}

} // namespace examples
