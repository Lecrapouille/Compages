// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace compages::renderer
{
class Scene;
} // namespace compages::renderer

// ****************************************************************************
//! \file
//! \brief The pictures of 10f_FlowingWater, made by the program: no file to
//! download, and every one tiles (its left edge meets its right edge), so
//! that it repeats over walls 24 m long without a seam.
// ****************************************************************************

namespace examples::flowing
{

// ****************************************************************************
//! \brief A square picture of RGBA bytes, rows from the bottom.
// ****************************************************************************
struct Picture
{
    int size = 0;
    std::vector<std::uint8_t> rgba;
};

// ----------------------------------------------------------------------------
//! \brief Smooth noise from 0 to 1 that repeats every \c p_period units:
//! random values on the corners of a lattice, blended in between (value
//! noise). Same place, same seed: same value.
//! \param[in] p_x, p_y Where, in cells of the lattice.
//! \param[in] p_period After how many cells it repeats.
//! \param[in] p_seed Another seed, another noise.
// ----------------------------------------------------------------------------
[[nodiscard]] float tiledNoise(float p_x, float p_y, int p_period, std::uint32_t p_seed);

// ----------------------------------------------------------------------------
//! \brief Several noises, each twice finer and half as strong as the one
//! before (fractal Brownian motion): from 0 to 1, repeating every
//! \c p_period cells of the first one.
// ----------------------------------------------------------------------------
[[nodiscard]] float tiledFbm(float p_x, float p_y, int p_period, int p_octaves, std::uint32_t p_seed);

//! \brief The noise texture of the paper: R a smooth noise (the offset of the
//! phase of each pixel), G a coarser one (where the foam gathers).
[[nodiscard]] Picture noisePicture();
//! \brief Small ripples: a normal map (RGB = normal * 0.5 + 0.5).
[[nodiscard]] Picture ripplePicture();
//! \brief The colour map carried by the flow: grimy foam in patches and
//! bubbles, its opacity in alpha.
[[nodiscard]] Picture foamPicture();
//! \brief Concrete cast in panels: stains, pits, the joints of the forms.
[[nodiscard]] Picture concretePicture();
//! \brief Old painted metal: green-grey paint, scratches, rust in blotches.
[[nodiscard]] Picture paintedMetalPicture();
//! \brief Yellow and black stripes, for the edge of the pool.
[[nodiscard]] Picture hazardPicture();

// ----------------------------------------------------------------------------
//! \brief Send a picture to the GPU, repeated, with mipmaps, and keep it in
//! the assets of the scene.
//! \param[in] p_srgb True for a colour painted for the eye, false for data
//! (noise, normals).
// ----------------------------------------------------------------------------
[[nodiscard]] compages::Result<compages::renderer::TextureAssetId>
addPicture(compages::renderer::Scene& p_scene, std::string const& p_name, Picture p_picture, bool p_srgb);

} // namespace examples::flowing
