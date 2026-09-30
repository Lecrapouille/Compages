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

#include "Compages/Renderer/Assets/AssetIds.hpp"

#include <cstdint>

namespace compages::renderer
{

// ****************************************************************************
//! \brief What can be done with a Renderable, packed as bit flags.
//!
//! Bits are stable identifiers, not indices into an array. The extractor reads
//! them to decide whether an item shows up in a given pass.
//!
//! \code
//! r.flags = compages::renderer::RenderFlags::NoShadowCaster | compages::renderer::RenderFlags::Overlay;
//! \endcode
// ****************************************************************************
enum class RenderFlags : std::uint32_t
{
    //! \brief Default: shows up in every pass the material declares.
    None = 0u,
    //! \brief Ignored during the depth prepass. What a decal wants.
    NoDepthPrepass = 1u << 0u,
    //! \brief Ignored by shadow-caster passes.
    NoShadowCaster = 1u << 1u,
    //! \brief Ignored by shadow-receiver passes.
    NoShadowReceiver = 1u << 2u,
    //! \brief Drawn on top of everything at the end. What a UI billboard wants.
    Overlay = 1u << 3u,
};

[[nodiscard]] constexpr RenderFlags operator|(RenderFlags p_a, RenderFlags p_b)
{
    return static_cast<RenderFlags>(static_cast<std::uint32_t>(p_a) |
                                    static_cast<std::uint32_t>(p_b));
}

[[nodiscard]] constexpr bool has(RenderFlags p_flags, RenderFlags p_flag)
{
    return (static_cast<std::uint32_t>(p_flags) &
            static_cast<std::uint32_t>(p_flag)) != 0u;
}

// ****************************************************************************
//! \brief "Draw this mesh with this material", stored on a world entity.
//!
//! Data only: generational ids into the AssetManager, not GPU objects and not
//! the assets themselves. The compages::world::World stays a simulation; resolving an id to a
//! pipeline or a buffer is the renderer's job, at extraction time.
//!
//! \code
//! cube.set(compages::renderer::MeshRenderer{ .mesh = meshId, .material_instance = matId });
//! \endcode
// ****************************************************************************
struct MeshRenderer
{
    //! \brief Which mesh to draw.
    MeshAssetId mesh;
    //! \brief Which material instance to draw it with. The instance names the
    //! Material family, so the renderer picks the pipeline from there.
    MaterialInstanceId material_instance;
    //! \brief Per-pass filtering. Default: shows up in every pass.
    RenderFlags flags = RenderFlags::None;
};

} // namespace compages::renderer
