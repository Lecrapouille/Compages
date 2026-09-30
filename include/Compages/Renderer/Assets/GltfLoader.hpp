// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/AssetManager.hpp"

#include <string>

namespace compages::renderer
{

//! \brief Import a reusable glTF prefab without creating compages::world::World
//! entities.
[[nodiscard]] Result<PrefabId> loadGltf(std::string const& p_path,
                                        AssetManager& p_assets,
                                        MaterialId p_shared_material = {});

} // namespace compages::renderer
