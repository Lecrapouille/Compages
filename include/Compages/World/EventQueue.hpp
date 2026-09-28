// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/World/EntityId.hpp"

#include <span>
#include <vector>

namespace compages::world
{

// ****************************************************************************
//! \brief What happened this frame.
//!
//! \code
//! queue.push({ .kind = compages::world::EventKind::Collision, .a = a, .b = b });
//! \endcode
// ****************************************************************************
enum class EventKind
{
    Collision,
    Trigger,
};

// ****************************************************************************
//! \brief A lightweight event emitted by physics or gameplay systems.
//!
//! \code
//! compages::world::Event e{ .kind = compages::world::EventKind::Trigger, .a = sensor, .b = player };
//! \endcode
// ****************************************************************************
struct Event
{
    EventKind kind = EventKind::Collision;
    EntityId a{};
    EntityId b{};
};

// ****************************************************************************
//! \brief FIFO queue cleared once per frame after dispatch.
//!
//! \code
//! compages::world::EventQueue events;
//! events.push(compages::world::Event{ .kind = compages::world::EventKind::Trigger, .a = player });
//! for (compages::world::Event const& e : events.events()) { /* react */ }
//! events.clear();
//! \endcode
// ****************************************************************************
class EventQueue
{
public:

    //! \brief Append an event for this frame.
    void push(Event p_event) { m_events.emplace_back(p_event); }

    //! \brief Read all events pushed since the last \c clear().
    [[nodiscard]] std::span<Event const> events() const
    {
        return { m_events.data(), m_events.size() };
    }

    //! \brief Drop every event, usually after dispatch.
    void clear() { m_events.clear(); }

private:

    std::vector<Event> m_events;
};

} // namespace compages::world
