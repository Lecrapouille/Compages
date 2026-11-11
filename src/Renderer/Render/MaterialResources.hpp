// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Assets/AssetManager.hpp"
#include "Compages/Renderer/Assets/Material.hpp"

namespace compages::renderer
{

//! \brief Bind the textures and the storage buffers of a material, set their
//! samplers: before the first draw with it, in the main pass and in the
//! shadow pass.
[[nodiscard]] Status bindMaterialResources(Material& p_material, AssetManager& p_assets);

} // namespace compages::renderer
