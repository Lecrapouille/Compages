// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstdint>
#include <functional>

namespace compages::renderer
{

// ****************************************************************************
//! \brief A stable handle to one row in an \c AssetManager table.
//!
//! The tag type \c Tag makes the kind of asset part of the C++ type, so a
//! \c MaterialInstanceId cannot be passed where a \c MeshAssetId is expected.
//! Each asset header defines its own alias, for example in \c MeshAsset.hpp:
//! \code
//! using MeshAssetId = compages::renderer::AssetId<struct MeshAssetTag>;
//! \endcode
//!
//! **Index** (low 16 bits) is the slot number in the manager's dense table.
//! It stays fixed for the lifetime of that registration: reloading GPU data
//! after a context loss does not change the index, so compages::world::World components can keep
//! storing the same id.
//!
//! **Generation** (high 16 bits) counts how many times that slot was reused.
//! When an asset is removed, its slot may be handed to a new asset; the
//! generation is bumped so an old \c MeshAssetId copied elsewhere becomes
//! stale (\c AssetManager::mesh(id) returns \c nullptr). Comparing two ids
//! requires both index and generation to match.
//!
//! An all-zero id is invalid (\c valid() is false). Live ids use generation
//! \c >= 1.
// ****************************************************************************
template <typename Tag>
class AssetId
{
public:

    // ------------------------------------------------------------------------
    //! \brief Empty id (\c valid() is false).
    // ------------------------------------------------------------------------
    constexpr AssetId() = default;

    // ------------------------------------------------------------------------
    //! \brief Pack slot index and generation as stored by \c AssetManager.
    // ------------------------------------------------------------------------
    constexpr AssetId(std::uint16_t p_index, std::uint16_t p_generation)
        : m_bits((static_cast<std::uint32_t>(p_generation) << 16u) |
                 static_cast<std::uint32_t>(p_index))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief \c true when this id refers to a registered asset slot.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool valid() const
    {
        return m_bits != 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Same as \c valid() (explicit to avoid accidental truth tests).
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr explicit operator bool() const
    {
        return valid();
    }

    // ------------------------------------------------------------------------
    //! \brief Stable slot index in the manager table (low 16 bits).
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint16_t index() const
    {
        return static_cast<std::uint16_t>(m_bits & 0xFFFFu);
    }

    // ------------------------------------------------------------------------
    //! \brief Generation bumped when the slot is reused (high 16 bits).
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint16_t generation() const
    {
        return static_cast<std::uint16_t>(m_bits >> 16u);
    }

    // ------------------------------------------------------------------------
    //! \brief Raw encoded value (for hashing and serialization).
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint32_t bits() const
    {
        return m_bits;
    }

    // ------------------------------------------------------------------------
    //! \brief Two ids match only when index and generation both match.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(AssetId const& p_other) const
    {
        return m_bits == p_other.m_bits;
    }

    // ------------------------------------------------------------------------
    //! \brief Negation of \c operator==.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool operator!=(AssetId const& p_other) const
    {
        return m_bits != p_other.m_bits;
    }

    // ------------------------------------------------------------------------
    //! \brief Total order on the packed bits (not semantic asset order).
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(AssetId const& p_other) const
    {
        return m_bits <=> p_other.m_bits;
    }

private:

    std::uint32_t m_bits = 0u;
};

//! \brief Names one mesh in the \c AssetManager.
using MeshAssetId = AssetId<struct MeshAssetTag>;
//! \brief Names one material family in the \c AssetManager.
using MaterialId = AssetId<struct MaterialTag>;
//! \brief Names one material instance in the \c AssetManager.
using MaterialInstanceId = AssetId<struct MaterialInstanceTag>;
//! \brief Names one texture in the \c AssetManager.
using TextureAssetId = AssetId<struct TextureAssetTag>;
//! \brief Names one inverse-bind skeleton in the \c AssetManager.
using SkinAssetId = AssetId<struct SkinAssetTag>;
//! \brief Names one sampled animation clip in the \c AssetManager.
using AnimationClipId = AssetId<struct AnimationClipTag>;
//! \brief Names one prefab template in the \c AssetManager.
using PrefabId = AssetId<struct PrefabTag>;

} // namespace compages::renderer

template <typename Tag>
struct std::hash<compages::renderer::AssetId<Tag>>
{
    //! \brief Hash the packed id bits for \c unordered_map keys.
    [[nodiscard]] std::size_t
    operator()(compages::renderer::AssetId<Tag> const& p_id) const
    {
        return std::hash<std::uint32_t>{}(p_id.bits());
    }
};
