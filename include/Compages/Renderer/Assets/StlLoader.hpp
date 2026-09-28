// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
