// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/World/EntityId.hpp"
#include "Compages/World/Routines/Routine.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief The routines of one entity, as a component of the World: they are
//! destroyed with their entity.
// ****************************************************************************
struct RoutineList: compages::NotCopiable
{
    RoutineList() = default;
    RoutineList(RoutineList&&) = default;
    RoutineList& operator=(RoutineList&&) = default;

    std::vector<std::unique_ptr<Routine>> list;
};

// ****************************************************************************
//! \brief Keeps and runs the routines of a World.
//!
//! Given by World::routines(). Entity::add<T>(), remove<T>(), find<T>() go
//! through it when T derives from Routine, so most code never calls it. The
//! World calls update() in World::step() and lateUpdate() in
//! World::endStep(); see Routine for the order of the calls.
//!
//! \code
//! world.routines().fixedStep(1.0f / 120.0f);   // fixedUpdate() at 120 Hz
//! auto* orbit = world.routines().find<compages::world::Orbit>(camera);
//! \endcode
// ****************************************************************************
class RoutineSystem: private compages::NotCopiable
{
public:

    explicit RoutineSystem(World& p_world) : m_world(p_world) {}
    RoutineSystem(RoutineSystem&&) = delete;
    RoutineSystem& operator=(RoutineSystem&&) = delete;

    //! \brief Attach a routine to an entity. What Entity::add<T>() does when
    //! T derives from Routine. It becomes active at the next step.
    Routine& add(EntityId p_entity, std::unique_ptr<Routine> p_routine);

    //! \brief How many routines an entity has.
    [[nodiscard]] std::size_t count(EntityId p_entity) const;

    //! \brief The first routine of type T of an entity, or nullptr.
    template <typename T>
    [[nodiscard]] T* find(EntityId p_entity) const
    {
        if (RoutineList const* routines = listOf(p_entity))
        {
            for (std::unique_ptr<Routine> const& routine : routines->list)
            {
                if (T* found = dynamic_cast<T*>(routine.get()))
                {
                    return found;
                }
            }
        }
        return nullptr;
    }

    //! \brief Take the routines of type T off an entity, after their
    //! onDisable() and onDestroy(). Not from inside one of them: it would be
    //! destroyed while it runs.
    template <typename T>
    void remove(EntityId p_entity)
    {
        removeIf(p_entity,
                 [](Routine const* p_routine)
                 { return dynamic_cast<T const*>(p_routine) != nullptr; });
    }

    //! \brief Send a value to the active routines of an entity: their
    //! onMessage() is called right away, in the running order.
    //!
    //! \code
    //! world.routines().send(ball, compages::world::Contact{ wall, point, normal, speed });
    //! \endcode
    template <typename T>
    void send(EntityId p_to, T p_value, EntityId p_from = {})
    {
        deliver(p_to, Message{ p_from, std::any(std::move(p_value)) });
    }

    //! \brief Send a value to the active routines of every entity.
    template <typename T>
    void broadcast(T p_value, EntityId p_from = {})
    {
        deliver({}, Message{ p_from, std::any(std::move(p_value)) });
    }

    //! \brief The pace of fixedUpdate(), in seconds. 1/60 by default.
    void fixedStep(float p_seconds);

    //! \brief The pace of fixedUpdate(), in seconds.
    [[nodiscard]] float fixedStep() const
    {
        return m_fixed_step;
    }

    //! \brief onEnable(), onDisable() and start() where needed, then
    //! fixedUpdate() as many times as the elapsed time holds fixed steps, then
    //! update(). Called by World::step().
    void update(float p_dt);

    //! \brief lateUpdate() of the active routines. Called by World::endStep().
    void lateUpdate(float p_dt);

    //! \brief onDisable() and onDestroy() of the routines of an entity, then
    //! drop them. Called by World::destroy() while the entity is alive.
    void release(EntityId p_entity);

    //! \brief release() every entity. Called when the World is destroyed.
    void clear();

private:

    //! \brief A routine in the running order. The rank tells it from another
    //! routine made later at the same address.
    struct Entry
    {
        EntityId entity;
        Routine* routine;
        std::uint64_t rank;
    };

    [[nodiscard]] RoutineList const* listOf(EntityId p_entity) const;
    void removeIf(EntityId p_entity, bool (*p_match)(Routine const*));
    //! \brief Is the routine of an entry still there, and still the same one?
    [[nodiscard]] bool live(Entry const& p_entry) const;
    void sortIfNeeded();
    //! \brief Give a message to the active routines of an entity, or of all
    //! when \c p_to is empty.
    void deliver(EntityId p_to, Message const& p_message);
    void refresh();
    //! \brief Advance the timers of a routine, calling those that are due.
    void tick(Entry const& p_entry, float p_dt);
    //! \brief onDisable() if active, then onDestroy().
    static void retire(Routine& p_routine);

    //! \brief At most this many fixedUpdate() per step: past it, a slow
    //! frame drops the time it could not catch up with.
    static constexpr int MAX_FIXED_STEPS = 8;

    World& m_world;
    std::vector<Entry> m_order;
    bool m_sorted = true;
    std::uint64_t m_next_rank = 0u;
    float m_fixed_step = 1.0f / 60.0f;
    float m_accumulator = 0.0f;
};

} // namespace compages::world
