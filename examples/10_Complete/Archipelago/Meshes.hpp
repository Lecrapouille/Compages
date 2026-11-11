// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief The meshes and pictures of 10e_Archipelago, made on the CPU, and
//! the way to swap the mesh of an entity already on the GPU.
// ****************************************************************************

#include "10_Complete/Archipelago/Land.hpp"

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"
#include "Compages/Renderer/Assets/TextureAsset.hpp"
#include "Compages/Renderer/Scene/Scene.hpp"

#include <string>

namespace examples::archipelago
{

//! \brief A square of a patch for a mesh: its first corner (i, j), its quads
//! along a side, and the texture coordinates of its first corner and their
//! step from a corner to the next.
struct Square
{
    int i;
    int j;
    int cells;
    compages::Vector2f uv;
    float uv_step;
};

// ----------------------------------------------------------------------------
//! \brief The mesh of a square of a patch, the sea flat at the sea bed.
//! \param[in] p_skirt Add a wall this deep along the edges (0: none): it
//!   hides the cracks with the coarser tiles next to it.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::renderer::MeshAsset
meshOf(Patch const& p_patch, Square const& p_square, float p_skirt);

//! \brief The picture of a patch for the GPU: sRGB (the alpha, the water,
//! stays linear), with mipmaps for the view from afar.
[[nodiscard]] compages::renderer::TextureAsset pictureOf(std::string p_name,
                                                         Patch const& p_patch);

//! \brief The ocean around the map: four trapezoids out to 150 km, as deep
//! as the edge of the map, reading the deep water of the edge of the picture.
[[nodiscard]] compages::renderer::MeshAsset oceanMesh();

//! \brief A square of one metre, \p p_cells x \p p_cells quads, centred.
[[nodiscard]] compages::renderer::MeshAsset gridMesh(int p_cells);

//! \brief A triangle hidden under the sea: the first mesh of an overlay
//! made later.
[[nodiscard]] compages::renderer::MeshAsset hiddenMesh();

} // namespace examples::archipelago
