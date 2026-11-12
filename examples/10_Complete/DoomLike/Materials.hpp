// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <string>

namespace compages::renderer
{
class AssetManager;
} // namespace compages::renderer

// ****************************************************************************
//! \file
//! \brief The surfaces of 10b_DoomLike: photographed materials of Poly Haven
//! (CC0), each a colour, a normal map and a roughness map, in
//! external/Compages-data/polyhaven/textures/<name>/.
// ****************************************************************************

namespace examples::doom
{

// ----------------------------------------------------------------------------
//! \brief A material for walls, floors, ceilings and containers: the three
//! pictures projected along the three axes and blended by the normal
//! (triplanar: boxes of any size need no UV), the bumps of the normal map,
//! and the glint of the lamps sharper where the surface is smooth.
//! \param[in] p_assets Where the material and its pictures go.
//! \param[in] p_name The name of the Poly Haven texture (its folder).
//! \param[in] p_tile How many metres one picture covers.
//! \return The material, or why not: a picture is missing.
// ----------------------------------------------------------------------------
[[nodiscard]] compages::Result<compages::renderer::MaterialId>
makeSurface(compages::renderer::AssetManager& p_assets, std::string const& p_name, float p_tile);

} // namespace examples::doom
