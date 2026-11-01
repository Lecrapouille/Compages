// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Renderer/Assets/Prefab.hpp"

namespace compages::renderer
{

// ****************************************************************************
//! \brief The cube robot from \c 17_MovingRobot, baked as a prefab template.
//!
//! Expects mesh \c "box" and material instances \c "wood", \c "dark" and
//! \c "light" to exist in the AssetManager before instantiation.
// ****************************************************************************
[[nodiscard]] Prefab makeRobotPrefab();

} // namespace compages::renderer
