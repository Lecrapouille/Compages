//=============================================================================
// Compages: A C++20 OpenGL wrapper.
// Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
//
// This file is part of Compages.
//
// Compages is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Compages is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Compages.  If not, see <http://www.gnu.org/licenses/>.
//=============================================================================

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
// ****************************************************************************
struct SkinAsset
{
    std::vector<compages::core::Matrix44f> inverse_bind;
};

} // namespace compages::renderer
