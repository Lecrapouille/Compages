// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdint>

namespace examples::archipelago
{

//! \brief 0 before \p p_from, 1 after \p p_to, a smooth S between.
[[nodiscard]] float smoothstep(float p_from, float p_to, float p_x);

//! \brief A number from 0 to 1 for a corner of an integer lattice.
[[nodiscard]] float lattice(int p_x, int p_y, std::uint32_t p_seed);

//! \brief Value noise: the lattice, smoothly interpolated. From 0 to 1.
[[nodiscard]] float noise(float p_x, float p_y, std::uint32_t p_seed);

//! \brief Octaves of noise, each twice as fine and half as strong. 0 to 1.
[[nodiscard]] float
fbm(float p_x, float p_y, std::uint32_t p_seed, int p_octaves);

} // namespace examples::archipelago
