// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>

namespace compages::gpu::detail
{

// ----------------------------------------------------------------------------
//! \brief Write tightly packed pixels into a PNG file, turned upside down:
//! the device gives the bottom row first, a file holds the top row first.
//! \return false when stb_image_write could not write the file.
// ----------------------------------------------------------------------------
bool writePngBottomUp(std::filesystem::path const& p_path,
                      std::uint32_t p_width,
                      std::uint32_t p_height,
                      std::uint32_t p_channels,
                      std::span<const std::byte> p_pixels);

} // namespace compages::gpu::detail
