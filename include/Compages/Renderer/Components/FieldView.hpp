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
//! \brief What Scene::show() keeps on the entity that draws a field: the ids
//! of its material and of the picture of its values, to write new values.
// ****************************************************************************
struct FieldView
{
    //! \brief The material made for this field: its range, its relief.
    MaterialId material;
    //! \brief One float per point of the grid.
    TextureAssetId values;
    //! \brief The size of the grid; new values must have the same.
    int columns = 0;
    int rows = 0;
};

} // namespace compages::renderer
