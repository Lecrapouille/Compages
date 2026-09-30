// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/World/Controllers/Input.hpp"
#include "Compages/World/EntityId.hpp"
#include "Compages/Core/Frame.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

namespace compages::world
{

class World;
class Entity;

// ****************************************************************************
//! \brief A piece of code attached to an entity and run every frame, the way
//! a Unity MonoBehaviour is.
//!
//! Derive from it, override update(), and add it to an entity like a
//! component:
//! \code
//! struct Spin: compages::world::Behavior
//! {
//!     float speed = 1.0f;
//!     explicit Spin(float p_speed) : speed(p_speed) {}
//!
//!     void update(float p_dt) override
//!     {
//!         transform().rotateY(speed * p_dt);
//!     }
//! };
//!
//! cube.add<Spin>(2.0f);    // turns two radians a second
//! \endcode
//!
//! World::update() runs them, in the order they were added, after calling
//! start() once on the ones that are new. An entity can have several.
// ****************************************************************************
class Behavior
{
public:

    virtual ~Behavior() = default;

    // ------------------------------------------------------------------------
    //! \brief Called once, at the first update after the behavior was added.
    // ------------------------------------------------------------------------
    virtual void start()
    {
        /* do nothing */
    }

    // ------------------------------------------------------------------------
    //! \brief Called at every update of the World.
    //! \param[in] p_dt the seconds since the previous frame.
    // ------------------------------------------------------------------------
    virtual void update(float p_dt)
    {
        (void)p_dt;
    }

protected:

    // ------------------------------------------------------------------------
    //! \brief The entity this behavior is attached to.
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity entity() const;

    // ------------------------------------------------------------------------
    //! \brief The place of that entity, to be changed.
    // ------------------------------------------------------------------------
    [[nodiscard]] LocalTransformView transform() const;

    // ------------------------------------------------------------------------
    //! \brief The world the entity lives in.
    // ------------------------------------------------------------------------
    [[nodiscard]] World& world() const
    {
        return *m_world;
    }

    // ------------------------------------------------------------------------
    //! \brief The mouse and the keys during this frame.
    // ------------------------------------------------------------------------
    [[nodiscard]] Input const& input() const;

    // ------------------------------------------------------------------------
    //! \brief Timing from the last World update (size and delta time).
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::core::Frame const& frame() const;

private:

    friend class World;

    World* m_world = nullptr;
    EntityId m_entity{};
    bool m_started = false;
};

} // namespace compages::world
