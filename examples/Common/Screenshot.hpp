// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
