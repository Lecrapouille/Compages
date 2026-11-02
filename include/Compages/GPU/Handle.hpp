// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Result.hpp"

#include <cstddef>
#include <cstdint>
#include <functional>

namespace compages::gpu
{


// ****************************************************************************
//! \brief Names one GPU resource, in 32 bits, and knows when it is stale.
//!
//! Resources are not held through pointers. A handle is a slot number plus a
//! counter of how many times that slot has been reused, and the resource itself
//! lives in a contiguous pool. Three things follow from that.
//!
//! Copying is free and safe. A handle can be stored in a scene node, in a
//! command list, in a hash map, and none of that keeps the resource alive or
//! risks pointing at freed memory.
//!
//! Using a released resource is caught instead of corrupting. When a slot is
//! reused its counter changes, so an old handle no longer matches what the slot
//! holds and the pool reports it as stale. A dangling pointer offers no such
//! comfort.
//!
//! And the resources of one kind sit next to each other in memory, which is what
//! the old design could not do: it kept one std::unique_ptr per object inside a
//! std::map keyed by std::string, so walking the buffers of a mesh meant
//! chasing pointers through the heap and comparing strings along the way.
//!
//! The tag makes the type of resource part of the type of the handle, so a
//! texture handle cannot be passed where a buffer handle is expected. It is
//! never defined, only named:
//! \code
//! using BufferHandle = compages::gpu::Handle<struct BufferTag>;
//! \endcode
//!
//! \tparam Tag an incomplete type naming the kind of resource.
// ****************************************************************************
template <typename Tag>
class Handle
{
public:

    //! \brief How many resources of one kind can exist at the same time.
    static constexpr std::uint32_t MAX_COUNT = 0xFFFFu;

    // ------------------------------------------------------------------------
    //! \brief An empty handle, naming no resource.
    // ------------------------------------------------------------------------
    constexpr Handle() = default;

    // ------------------------------------------------------------------------
    //! \brief Name the resource living in a slot, at a given reuse count.
    //!
    //! Only a Pool has any business calling this.
    //!
    //! \param[in] p_index which slot.
    //! \param[in] p_generation how many times that slot has been used, starting
    //! at 1. Zero is what makes a handle empty, so it is never a live value.
    // ------------------------------------------------------------------------
    constexpr Handle(std::uint16_t p_index, std::uint16_t p_generation)
        : m_bits((static_cast<std::uint32_t>(p_generation) << 16u) |
                 static_cast<std::uint32_t>(p_index))
    {
    }

    // ------------------------------------------------------------------------
    //! \brief Does this handle name anything at all?
    //!
    //! Says nothing about whether the resource is still alive: only the pool
    //! knows that. This merely tells an empty handle from a filled one.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool valid() const
    {
        return m_bits != 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief Same as valid(), so that a handle can be tested directly.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr explicit operator bool() const
    {
        return valid();
    }

    // ------------------------------------------------------------------------
    //! \brief Which slot of the pool. Meaningful to the pool, and to a human
    //! reading a log; nothing else should depend on the value.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint16_t index() const
    {
        return static_cast<std::uint16_t>(m_bits & 0xFFFFu);
    }

    // ------------------------------------------------------------------------
    //! \brief How many times the slot had been used when this handle was made.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint16_t generation() const
    {
        return static_cast<std::uint16_t>(m_bits >> 16u);
    }

    // ------------------------------------------------------------------------
    //! \brief The whole handle as one number, for logs and hashing.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr std::uint32_t bits() const
    {
        return m_bits;
    }

    // ------------------------------------------------------------------------
    //! \brief Two handles are the same when they name the same resource at the
    //! same reuse count.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr bool operator==(Handle const& p_other) const
    {
        return m_bits == p_other.m_bits;
    }

    // ------------------------------------------------------------------------
    //! \brief Ordering, so handles can be keys of an ordered container.
    // ------------------------------------------------------------------------
    [[nodiscard]] constexpr auto operator<=>(Handle const& p_other) const
    {
        return m_bits <=> p_other.m_bits;
    }

private:

    //! \brief Reuse count in the high 16 bits, slot number in the low 16. An
    //! empty handle is exactly zero, which is why reuse counts start at 1.
    std::uint32_t m_bits = 0u;
};

} // namespace compages::gpu

// ----------------------------------------------------------------------------
//! \brief Lets a handle be the key of a std::unordered_map, which is how the
//! backend caches things per resource.
// ----------------------------------------------------------------------------
template <typename Tag>
struct std::hash<compages::gpu::Handle<Tag>>
{
    [[nodiscard]] std::size_t operator()(compages::gpu::Handle<Tag> const& p_handle) const
    {
        return std::hash<std::uint32_t>{}(p_handle.bits());
    }
};
