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

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/World/EntityId.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
namespace compages::world
{



// ****************************************************************************
//! \brief SoA storage for local TRS and derived world matrices, keyed by
//! EntityId index.
//!
//! Position, rotation and scale live in three parallel arrays. Walking a
//! system that only needs positions (physics integration, a translation
//! channel) therefore reads a tight float3 stream instead of skipping a
//! quaternion and a scale at every stride.
//!
//! Dirty tracking is per entity. Marking descendants dirty needs the
//! \c SpatialGraph and is the World's job; the store only records "someone
//! wrote to this entity's TRS since the last update".
//!
//! \code
//! store.allocate(entity);
//! store.localMutable(entity).position = { 0, 2, 0 };
//! store.markDirty(entity);
//! \endcode
// ****************************************************************************
class TransformStore : private compages::core::NotCopiable
{
public:

    TransformStore() = default;
    TransformStore(TransformStore&&) = default;
    TransformStore& operator=(TransformStore&&) = default;

    //! \brief Reserve a TRS slot when an entity becomes spatial.
    void allocate(EntityId p_entity);

    //! \brief Free the slot when the entity is destroyed.
    void release(EntityId p_entity);

    //! \brief Does this entity have a transform slot?
    [[nodiscard]] bool has(EntityId p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief A copy of the local pose. Does not mark anything dirty.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransform local(EntityId p_entity) const;

    // ------------------------------------------------------------------------
    //! \brief Mutable window onto the SoA slot. The caller (or \c World) must
    //! \c markDirty() after writing.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransformView localMutable(EntityId p_entity);

    //! \brief Read-only access to one SoA channel.
    [[nodiscard]] compages::core::Vector3f const& position(EntityId p_entity) const;
    //! \brief Read-only access to one SoA channel.
    [[nodiscard]] compages::core::Quatf const& rotation(EntityId p_entity) const;
    //! \brief Read-only access to one SoA channel.
    [[nodiscard]] compages::core::Vector3f const& scale(EntityId p_entity) const;

    //! \brief Tight view of every stored position (systems, debug).
    [[nodiscard]] std::span<compages::core::Vector3f const> positions() const
    {
        return m_position;
    }
    //! \brief Tight view of every stored rotation.
    [[nodiscard]] std::span<compages::core::Quatf const> rotations() const
    {
        return m_rotation;
    }
    //! \brief Tight view of every stored scale.
    [[nodiscard]] std::span<compages::core::Vector3f const> scales() const
    {
        return m_scale;
    }
    //! \brief Cached world matrices from the last \c TransformSystem pass.
    [[nodiscard]] std::span<compages::core::Matrix44f const> worlds() const
    {
        return m_world;
    }

    //! \brief Local TRS matrix for one entity (does not read the cache).
    [[nodiscard]] compages::core::Matrix44f localMatrix(EntityId p_entity) const;

    //! \brief World matrix after the last \c TransformSystem::update().
    [[nodiscard]] compages::core::Matrix44f const& world(EntityId p_entity) const;

    //! \brief Override the cached world matrix (tests, importers).
    void setWorld(EntityId p_entity, compages::core::Matrix44f const& p_matrix);

    //! \brief Local TRS changed; recompute on the next \c TransformSystem pass.
    void markDirty(EntityId p_entity);

    //! \brief World matrix is up to date for this entity.
    void markClean(EntityId p_entity);

    //! \brief Needs a world-matrix update?
    [[nodiscard]] bool isDirty(EntityId p_entity) const;

    //! \brief Number of entity slots (alive or free).
    [[nodiscard]] std::size_t capacity() const
    {
        return m_flags.size();
    }

private:

    static constexpr std::uint8_t FLAG_PRESENT = 0x01u;
    static constexpr std::uint8_t FLAG_DIRTY = 0x02u;

    void ensureCapacity(std::size_t p_index);
    [[nodiscard]] bool slotMatches(EntityId p_entity) const;

    std::vector<compages::core::Vector3f> m_position;
    std::vector<compages::core::Quatf> m_rotation;
    std::vector<compages::core::Vector3f> m_scale;
    std::vector<compages::core::Matrix44f> m_world;
    std::vector<std::uint8_t> m_flags;
    std::vector<std::uint16_t> m_generation;
};

} // namespace compages::world
