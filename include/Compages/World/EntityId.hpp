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

#include "Compages/World/Entt.hpp"

#include <compare>
#include <cstdint>
#include <functional>

namespace compages::world
{

// ****************************************************************************
//! \brief Names one identity inside a World, and knows when that identity is
//! gone.
//!
//! An EntityId is a handle: an index into the registry's slot arrays plus a
//! generation counter, packed in 32 bits. Copying is free. Destroying an entity
//! bumps its slot's generation, so a leftover handle is stale rather than
//! silently naming whatever reused the slot.
//!
//! An EntityId carries no data of its own. It is not a "GameObject": components
//! sit in their own stores keyed by EntityId, and spatial relations live in the
//! SpatialGraph. An EntityId may be spatial (owns a SpatialNode) or not.
//!
//! An empty EntityId is exactly zero. Live generations start at one, so slot zero
//! is an ordinary first entity and not a missing one.
//!
//! \code
//! compages::world::EntityId id = world.create("Bullet");
//! if (world.alive(id)) { world.destroy(id); }
//! \endcode
// ****************************************************************************
class EntityId
{
public:

    //! \brief How many entities can be alive at the same time.
    static constexpr std::uint32_t MAX_COUNT = 0xFFFFu;

    // ------------------------------------------------------------------------
    //! \brief An empty entity, naming nothing.
    // ------------------------------------------------------------------------
    constexpr EntityId() = default;

    constexpr explicit EntityId(entt::entity p_value)
        : m_value(p_value)
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Name the identity living in a slot, at a given generation.
    //!
    //! Only a World or a registry adapter has any business calling this.
    //!
    //! \param[in] p_index which slot in the registry.
    //! \param[in] p_generation how many times that slot has been used, starting
    //! at 1. Zero is what makes an entity empty, so it is never a live value.
    // ------------------------------------------------------------------------
    constexpr EntityId(std::uint16_t p_index, std::uint16_t p_generation)
        : m_value(entt::entt_traits<entt::entity>::construct(
              p_index, p_generation))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Does this handle name anything at all?
    //!
    //! Says nothing about whether the entity is still alive: only the registry
    //! knows that. This merely tells an empty handle from a filled one.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool valid() const
    {
        return m_value != entt::null;
    }

    [[nodiscard]] constexpr explicit operator bool() const
    {
        return valid();
    }

    //! \brief Registry slot index (stable while the entity lives).
    [[nodiscard]] constexpr std::uint32_t index() const
    {
        return entt::to_entity(m_value);
    }

    //! \brief How many times this slot was reused (stale-handle detection).
    [[nodiscard]] constexpr std::uint16_t generation() const
    {
        return entt::to_version(m_value);
    }

    //! \brief Packed index and generation for hashing and maps.
    [[nodiscard]] constexpr std::uint32_t bits() const
    {
        return entt::to_integral(m_value);
    }

    //! \brief Underlying EnTT entity value.
    [[nodiscard]] constexpr entt::entity native() const
    {
        return m_value;
    }

    //! \brief Lexicographic order on the packed bits.
    [[nodiscard]] constexpr auto operator<=>(EntityId const&) const = default;

private:

    entt::entity m_value = entt::null;
};

} // namespace compages::world

template <>
struct std::hash<compages::world::EntityId>
{
    [[nodiscard]] std::size_t operator()(compages::world::EntityId const& p_entity) const
    {
        return std::hash<std::uint32_t>{}(p_entity.bits());
    }
};
