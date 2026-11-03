// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"
#include "Compages/World/EntityId.hpp"

#include <any>

namespace compages::world
{

// ****************************************************************************
//! \brief A value sent to the routines of an entity, received by
//! Routine::onMessage(). Any type can be sent; as<T>() reads it back.
//!
//! \code
//! struct Hit { float damage; };
//! world.routines().send(enemy, Hit{ 10.0f });
//!
//! void onMessage(compages::world::Message const& p_message) override
//! {
//!     if (Hit const* hit = p_message.as<Hit>()) { life -= hit->damage; }
//! }
//! \endcode
// ****************************************************************************
struct Message
{
    //! \brief Who sent it, or empty.
    EntityId from;
    std::any value;

    //! \brief The value when it is a T, else nullptr.
    template <typename T>
    [[nodiscard]] T const* as() const
    {
        return std::any_cast<T>(&value);
    }
};

// ****************************************************************************
//! \brief Two bodies touched: what an outside physics engine sends to the
//! routines of each of them. Compages detects no collision itself.
//!
//! \code
//! // In the step of the engine, for each new contact between a and b:
//! world.routines().send(a, compages::world::Contact{ b, point, normal, speed });
//! world.routines().send(b, compages::world::Contact{ a, point, -normal, speed });
//! \endcode
// ****************************************************************************
struct Contact
{
    //! \brief The other body.
    EntityId other;
    //! \brief Where they touched, in the world.
    compages::Vector3f point{ 0.0f, 0.0f, 0.0f };
    //! \brief Unit direction pushing this body away from the other one.
    compages::Vector3f normal{ 0.0f, 1.0f, 0.0f };
    //! \brief How fast they came together, in metres per second.
    float speed = 0.0f;
};

} // namespace compages::world
