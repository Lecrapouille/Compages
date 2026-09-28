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

#include "Compages/Core/Result.hpp"
#include "Compages/Renderer/Assets/MeshAsset.hpp"

#include <cstddef>
#include <span>
#include <string>

namespace compages::renderer
{

// ****************************************************************************
//! \brief Read an STL file, ASCII or binary, into an indexed mesh.
//!
//! Corners sharing both their position and their normal are merged, so the
//! flat faces of a CAD part share their vertices while its sharp edges keep
//! one normal per face. A facet stored with a null normal gets the one of its
//! triangle. The mesh is sent to the GPU when a device exists.
//!
//! \code
//! auto mesh = compages::renderer::loadStl("irb2400/visual/link_1.stl");
//! if (mesh) { scene.mesh(mesh.take(), "Link1"); }
//! \endcode
// ****************************************************************************
[[nodiscard]] compages::Result<MeshAsset> loadStl(std::string const& p_path);

//! \brief Same, from the bytes of a file already in memory.
[[nodiscard]] compages::Result<MeshAsset>
parseStl(std::span<const std::byte> p_bytes);

} // namespace compages::renderer
