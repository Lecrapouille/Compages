// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/World/Controllers/Input.hpp"
#include "Compages/World/EntityId.hpp"
#include "Compages/World/Routines/Message.hpp"
#include "Compages/Core/Frame.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace compages::world
{

class World;
class Entity;

// ****************************************************************************
//! \brief Code attached to an entity and run at each step of its World, the
//! way a Unity MonoBehaviour is.
//!
//! Derive from it, override what you need, and add it to an entity like a
//! component:
//! \code
//! struct Spin: compages::world::Routine
//! {
//!     explicit Spin(float p_speed) : speed(p_speed) {}
//!
//!     void update(float p_dt) override
//!     {
//!         transform().rotateY(speed * p_dt);
//!     }
//!
//!     float speed;
//! };
//!
//! cube.add<Spin>(2.0f);    // turns two radians a second
//! \endcode
//!
//! A step of the World calls, for each routine of an enabled entity:
//! \code
//! onEnable()        when it becomes active (added, or its entity enabled)
//! start()           once, before its first fixedUpdate() or update()
//! fixedUpdate(dt)   0, 1 or more times, dt being RoutineSystem::fixedStep()
//! update(dt)        once, dt being the time of the frame
//!   ... joints, animation, world matrices ...
//!   ... timers set by after() and every() ...
//! lateUpdate(dt)    once, the world matrices being up to date
//! onDisable()       when it stops being active
//! onDestroy()       once, before it is removed or its entity destroyed
//! \endcode
//! Routines run by priority() (smaller first), then in the order they were
//! added, whatever their entity: a run is the same each time.
// ****************************************************************************
class Routine
{
public:

    virtual ~Routine() = default;

    //! \brief Called when the routine becomes active: added to an enabled
    //! entity, its entity enabled again, or enabled(true).
    virtual void onEnable() {}

    //! \brief Called once, at the first step where the routine is active,
    //! before its first fixedUpdate() or update().
    virtual void start() {}

    //! \brief Called at a fixed pace, 0, 1 or more times per step, to keep a
    //! simulation the same whatever the frame rate: a controller, a physics
    //! engine, a behavior tree.
    //! \param[in] p_dt RoutineSystem::fixedStep(), in seconds.
    virtual void fixedUpdate(float p_dt)
    {
        (void)p_dt;
    }

    //! \brief Called once per step.
    //! \param[in] p_dt the seconds since the previous step.
    virtual void update(float p_dt)
    {
        (void)p_dt;
    }

    //! \brief Called once per step, after the joints, the animation and the
    //! world matrices: what a camera following an object needs.
    //! \param[in] p_dt the seconds since the previous step.
    virtual void lateUpdate(float p_dt)
    {
        (void)p_dt;
    }

    //! \brief Called when a value is sent to the entity of the routine with
    //! RoutineSystem::send(), right away, if the routine is active.
    virtual void onMessage(Message const& p_message)
    {
        (void)p_message;
    }

    //! \brief Called when the routine stops being active: its entity
    //! disabled, enabled(false), or before onDestroy().
    virtual void onDisable() {}

    //! \brief Called once, before the routine is removed, its entity destroyed
    //! or its World destroyed. The entity is still alive here.
    virtual void onDestroy() {}

    //! \brief The order of the routine in a step: smaller runs first. Routines
    //! with the same priority run in the order they were added.
    [[nodiscard]] virtual int priority() const
    {
        return 0;
    }

    //! \brief Switch the routine on or off, without its entity. Taken into
    //! account at the next step (onEnable() or onDisable() then).
    void enabled(bool p_enabled)
    {
        m_enabled = p_enabled;
    }

    //! \brief Is the routine switched on? Its entity may still be disabled.
    [[nodiscard]] bool enabled() const
    {
        return m_enabled;
    }

protected:

    // ------------------------------------------------------------------------
    //! \brief The entity this routine is attached to.
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
    //! \brief Timing from the last World step (size and delta time).
    // ------------------------------------------------------------------------
    [[nodiscard]] compages::core::Frame const& frame() const;

    //! \brief Names a timer, to cancel it.
    using TimerId = std::uint32_t;

    // ------------------------------------------------------------------------
    //! \brief Call a function once, after some seconds of steps. The time
    //! stops while the routine is not active.
    //!
    //! \code
    //! after(2.0f, [this] { entity().destroy(); });   // gone in two seconds
    //! \endcode
    // ------------------------------------------------------------------------
    TimerId after(float p_seconds, std::function<void()> p_call);

    // ------------------------------------------------------------------------
    //! \brief Call a function every some seconds, until cancel().
    //!
    //! \code
    //! every(0.5f, [this] { blink(); });
    //! \endcode
    // ------------------------------------------------------------------------
    TimerId every(float p_seconds, std::function<void()> p_call);

    //! \brief Stop a timer. Nothing happens for a timer already done.
    void cancel(TimerId p_timer);

private:

    friend class RoutineSystem;

    World* m_world = nullptr;
    EntityId m_entity{};
    //! \brief Rank of addition in the World, for a stable order.
    std::uint64_t m_rank = 0u;
    bool m_enabled = true;
    //! \brief Was onEnable() called without onDisable() since?
    bool m_active = false;
    bool m_started = false;

    struct Timer
    {
        TimerId id;
        //! \brief Seconds before the next call.
        float remaining;
        //! \brief Seconds between two calls; zero for a single call.
        float period;
        std::function<void()> call;
    };

    std::vector<Timer> m_timers;
    TimerId m_next_timer = 1u;
};

} // namespace compages::world
