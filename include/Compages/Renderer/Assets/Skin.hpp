// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Matrix.hpp"
#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <vector>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Inverse-bind matrices of a glTF skin, in this library's matrix
//! storage (CPU rows are the shader columns).
// ****************************************************************************
struct SkinAsset
{
    std::vector<Matrix44f> inverse_bind;
};

} // namespace compages::renderer
