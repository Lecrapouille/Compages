// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Assets/AssetIds.hpp"

namespace compages::renderer
{

// ****************************************************************************
//! \brief Marks the root of an entity subtree spawned from a prefab template.
//!
//! Data only: which prefab id was used. The template itself stays in the
//! AssetManager.
//!
//! \code
//! root.set(compages::renderer::PrefabInstance{ .prefab = robotPrefabId });
//! \endcode
// ****************************************************************************
struct PrefabInstance
{
    PrefabId prefab{};
};

} // namespace compages::renderer
