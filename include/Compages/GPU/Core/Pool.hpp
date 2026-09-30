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

#include "Compages/GPU/Core/Handle.hpp"

#include <string>
#include <type_traits>
#include <vector>

namespace compages::gpu
{

// ****************************************************************************
//! \brief Holds the resources of one kind side by side and hands out handles to
//! them.
//!
//! Everything lives in a single std::vector, so the records of all the buffers,
//! or of all the textures, are contiguous. Releasing a resource does not move
//! the others: the slot is remembered as free and the next creation takes it
//! back, with its reuse count bumped so that handles to the old occupant stop
//! matching.
//!
//! That last point is the reason this exists. In the previous design a resource
//! was a std::unique_ptr inside a std::map keyed by a std::string name, and
//! using a released object was undefined behaviour. Here it is a question the
//! pool can answer, and get() simply returns nullptr.
//!
//! \tparam T what is stored per resource. Must be default constructible, since
//! a free slot has to hold something.
//! \tparam Tag the tag of the handles this pool hands out.
// ****************************************************************************
template <typename T, typename Tag>
class Pool
{
public:

    static_assert(std::is_default_constructible_v<T>,
                  "Pool<T> needs T to be default constructible: a free slot "
                  "still holds a T");

    // ------------------------------------------------------------------------
    //! \brief Put a resource in the pool and get a handle naming it.
    //!
    //! \param[in] p_value the record to store, moved in.
    //! \return the handle, or a failure when the pool is full. Full means
    //! compages::gpu::Handle<Tag>::MAX_COUNT live resources of this kind, which
    //! in practice means a leak rather than a legitimate need.
    // ------------------------------------------------------------------------
    [[nodiscard]] Result<compages::gpu::Handle<Tag>> add(T p_value)
    {
        std::uint16_t index = 0u;

        if (!m_free.empty())
        {
            index = m_free.back();
            m_free.pop_back();
        }
        else
        {
            if (m_slots.size() >= compages::gpu::Handle<Tag>::MAX_COUNT)
            {
                return failure(
                    "too many live resources of this kind (" +
                    std::to_string(compages::gpu::Handle<Tag>::MAX_COUNT) +
                    "), which usually means they are created every "
                    "frame and never released");
            }
            index = static_cast<std::uint16_t>(m_slots.size());
            m_slots.emplace_back();
        }

        Slot& slot = m_slots[index];
        slot.value = std::move(p_value);
        slot.live = true;
        // Reuse counts start at 1: zero is what makes a handle empty.
        slot.generation = nextGeneration(slot.generation);
        ++m_live_count;

        return compages::gpu::Handle<Tag>(index, slot.generation);
    }

    // ------------------------------------------------------------------------
    //! \brief Is this handle still naming a live resource?
    //!
    //! False for an empty handle, for a handle whose resource has been
    //! released, and for a handle whose slot has since been given to somebody
    //! else.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool valid(compages::gpu::Handle<Tag> p_handle) const
    {
        if (!p_handle.valid())
        {
            return false;
        }
        const std::uint16_t index = p_handle.index();
        if (index >= m_slots.size())
        {
            return false;
        }
        Slot const& slot = m_slots[index];
        return slot.live && (slot.generation == p_handle.generation());
    }

    // ------------------------------------------------------------------------
    //! \brief The record behind a handle, or nullptr when the handle is stale.
    // ------------------------------------------------------------------------
    [[nodiscard]] T* get(compages::gpu::Handle<Tag> p_handle)
    {
        return valid(p_handle) ? &m_slots[p_handle.index()].value : nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief The record behind a handle, or nullptr when the handle is stale.
    // ------------------------------------------------------------------------
    [[nodiscard]] T const* get(compages::gpu::Handle<Tag> p_handle) const
    {
        return valid(p_handle) ? &m_slots[p_handle.index()].value : nullptr;
    }

    // ------------------------------------------------------------------------
    //! \brief Give the slot back, so a later creation can take it.
    //!
    //! \return true when something was released, false when the handle was
    //! already stale, which lets a double release be reported rather than
    //! silently corrupting the pool.
    // ------------------------------------------------------------------------
    bool remove(compages::gpu::Handle<Tag> p_handle)
    {
        if (!valid(p_handle))
        {
            return false;
        }
        Slot& slot = m_slots[p_handle.index()];
        slot.live = false;
        // Drop whatever the record owned; a free slot must hold nothing.
        slot.value = T{};
        m_free.emplace_back(p_handle.index());
        --m_live_count;
        return true;
    }

    // ------------------------------------------------------------------------
    //! \brief Visit every live resource.
    //!
    //! Used at shutdown to release what the caller forgot, and by the examples
    //! gallery to show what is still alive after a demo has been closed.
    //!
    //! \param[in] p_visitor called as p_visitor(handle, value).
    // ------------------------------------------------------------------------
    template <typename Visitor>
    void forEach(Visitor&& p_visitor)
    {
        const std::size_t count = m_slots.size();
        for (std::size_t i = 0u; i < count; ++i)
        {
            Slot& slot = m_slots[i];
            if (slot.live)
            {
                p_visitor(compages::gpu::Handle<Tag>(
                              static_cast<std::uint16_t>(i), slot.generation),
                          slot.value);
            }
        }
    }

    // ------------------------------------------------------------------------
    //! \brief How many live resources the pool holds.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t size() const
    {
        return m_live_count;
    }

    // ------------------------------------------------------------------------
    //! \brief Is the pool empty? What shutdown checks to spot a leak.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return m_live_count == 0u;
    }

    // ------------------------------------------------------------------------
    //! \brief How many slots exist, live or free. Grows to the high water mark
    //! of live resources and never shrinks, so that indices stay valid.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::size_t slots() const
    {
        return m_slots.size();
    }

    // ------------------------------------------------------------------------
    //! \brief Release everything and start over from an empty pool.
    // ------------------------------------------------------------------------
    void clear()
    {
        m_slots.clear();
        m_free.clear();
        m_live_count = 0u;
    }

private:

    //! \brief Advance a reuse count, skipping zero on wrap around so that a
    //! live handle is never mistaken for an empty one.
    [[nodiscard]] static std::uint16_t nextGeneration(std::uint16_t p_current)
    {
        const std::uint16_t next = static_cast<std::uint16_t>(p_current + 1u);
        return (next == 0u) ? std::uint16_t(1u) : next;
    }

    struct Slot
    {
        T value{};
        //! \brief How many times this slot has been handed out. Zero means it
        //! never has, which is why add() bumps it before use.
        std::uint16_t generation = 0u;
        bool live = false;
    };

    std::vector<Slot> m_slots;
    //! \brief Slots waiting to be handed out again, most recently freed first:
    //! taking the last one back keeps the reused memory warm in cache.
    std::vector<std::uint16_t> m_free;
    std::size_t m_live_count = 0u;
};

} // namespace compages::gpu
