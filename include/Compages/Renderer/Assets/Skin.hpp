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

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::renderer
{



// ****************************************************************************
//! \brief Inverse-bind matrices of a glTF skin, in this library's matrix
//! storage (CPU rows are the shader columns).
//!
//! Each matrix brings a vertex from the space of the model to the space of
//! one joint at rest. The glTF loader makes it; the AnimationSystem combines
//! it with the current pose of the joints into the palette the vertex shader
//! uses (SkinInstance).
// ****************************************************************************
struct SkinAsset
{
    std::vector<compages::Matrix44f> inverse_bind;
};

} // namespace compages::renderer
