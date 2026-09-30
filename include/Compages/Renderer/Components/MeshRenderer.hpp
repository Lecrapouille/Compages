// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

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
