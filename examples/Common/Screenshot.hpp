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

#include "Compages/GPU/Core/Handle.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

// ****************************************************************************
//! \file
//! \brief Writing an image file, which the library does not do either.
//!
//! Reading an image is part of the library because a texture has to come from
//! somewhere. Writing one is not: nothing in a renderer needs it, and the only
//! reason the examples want it is to make the pictures in the documentation.
//!
//! It lives in its own translation unit because it carries a single file library
//! with it, and third party code is worth keeping to one place where the warnings
//! it produces can be turned off without turning them off for our own code.
// ****************************************************************************

namespace examples
{

// ----------------------------------------------------------------------------
//! \brief Write pixels to a PNG file.
//!
//! \param[in] p_path where to write, including the extension.
//! \param[in] p_width,p_height the size of the picture in pixels.
//! \param[in] p_pixels four bytes per pixel, red first, bottom row first as the
//! device hands them over. Turned the right way up on the way out.
//! \return why the file could not be written.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::gpu::Status writePng(std::string const& p_path,
                                   std::uint32_t p_width,
                                   std::uint32_t p_height,
                                   std::span<const std::byte> p_pixels);

} // namespace examples
