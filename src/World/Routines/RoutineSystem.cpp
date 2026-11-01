// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Routines/RoutineSystem.hpp"
#include "Compages/World/World.hpp"

#include <algorithm>
#include <cassert>

namespace compages::world
{

Routine& RoutineSystem::add(EntityId p_entity, std::unique_ptr<Routine> p_routine)
{
    assert(m_world.alive(p_entity) && "a routine added to a dead entity");
    p_routine->m_world = &m_world;
    p_routine->m_entity = p_entity;
    p_routine->m_rank = m_next_rank++;
    RoutineList* routines = m_world.tryGet<RoutineList>(p_entity);
    if (routines == nullptr)
    {
        routines = &m_world.add(p_entity, RoutineList{});
    }
    routines->list.emplace_back(std::move(p_routine));
    m_sorted = false;
    return *routines->list.back();
}

std::size_t RoutineSystem::count(EntityId p_entity) const
{
    RoutineList const* routines = listOf(p_entity);
    return (routines == nullptr) ? 0u : routines->list.size();
}

RoutineList const* RoutineSystem::listOf(EntityId p_entity) const
{
    return static_cast<World const&>(m_world).tryGet<RoutineList>(p_entity);
}

void RoutineSystem::retire(Routine& p_routine)
{
    if (p_routine.m_active)
    {
        p_routine.m_active = false;
        p_routine.onDisable();
    }
    p_routine.onDestroy();
}

void RoutineSystem::removeIf(EntityId p_entity, bool (*p_match)(Routine const*))
{
    RoutineList* routines = m_world.tryGet<RoutineList>(p_entity);
    if (routines == nullptr)
    {
        return;
    }
    // Take the matching ones out first: their onDestroy() may add routines to
    // the same entity.
    std::vector<std::unique_ptr<Routine>> leaving;
    auto kept = std::stable_partition(routines->list.begin(), routines->list.end(),
                                      [p_match](std::unique_ptr<Routine> const& p_routine)
                                      { return !p_match(p_routine.get()); });
    std::move(kept, routines->list.end(), std::back_inserter(leaving));
    routines->list.erase(kept, routines->list.end());
    for (std::unique_ptr<Routine> const& routine : leaving)
    {
        retire(*routine);
    }
    m_sorted = false;
}

void RoutineSystem::release(EntityId p_entity)
{
    RoutineList* routines = m_world.tryGet<RoutineList>(p_entity);
    if (routines == nullptr)
    {
        return;
    }
    std::vector<std::unique_ptr<Routine>> leaving = std::move(routines->list);
    routines->list.clear();
    for (std::unique_ptr<Routine> const& routine : leaving)
    {
        retire(*routine);
    }
    m_sorted = false;
}

void RoutineSystem::clear()
{
    std::vector<EntityId> owners;
    for (auto [native, routines] : m_world.view<RoutineList>().each())
    {
        (void)routines;
        owners.push_back(EntityId(native));
    }
    for (EntityId const owner : owners)
    {
        release(owner);
    }
    m_order.clear();
}

void RoutineSystem::fixedStep(float p_seconds)
{
    assert((p_seconds > 0.0f) && "the fixed step must be positive");
    m_fixed_step = p_seconds;
}

bool RoutineSystem::live(Entry const& p_entry) const
{
    RoutineList const* owner = listOf(p_entry.entity);
    if (owner == nullptr)
    {
        return false;
    }
    // The pointer is only followed once found again in its entity.
    return std::any_of(owner->list.begin(), owner->list.end(),
                       [&p_entry](std::unique_ptr<Routine> const& p_kept)
                       { return (p_kept.get() == p_entry.routine) && (p_kept->m_rank == p_entry.rank); });
}

void RoutineSystem::sortIfNeeded()
{
    if (m_sorted)
    {
        return;
    }
    m_order.clear();
    for (auto [native, routines] : m_world.view<RoutineList>().each())
    {
        for (std::unique_ptr<Routine> const& routine : routines.list)
        {
            m_order.push_back(Entry{ EntityId(native), routine.get(), routine->m_rank });
        }
    }
    std::sort(m_order.begin(), m_order.end(),
              [](Entry const& p_a, Entry const& p_b)
              {
                  const int a = p_a.routine->priority();
                  const int b = p_b.routine->priority();
                  return (a != b) ? (a < b) : (p_a.rank < p_b.rank);
              });
    m_sorted = true;
}

void RoutineSystem::refresh()
{
    for (Entry const& entry : m_order)
    {
        if (!live(entry))
        {
            continue;
        }
        Routine& routine = *entry.routine;
        const bool active = routine.m_enabled && m_world.enabledInHierarchy(entry.entity);
        if (active && !routine.m_active)
        {
            routine.m_active = true;
            routine.onEnable();
        }
        else if (!active && routine.m_active)
        {
            routine.m_active = false;
            routine.onDisable();
        }
        if (routine.m_active && !routine.m_started && live(entry))
        {
            routine.m_started = true;
            routine.start();
        }
    }
}

void RoutineSystem::update(float p_dt)
{
    // A routine may create or destroy entities, or add routines: the order is
    // only rebuilt between two passes, and an entry is checked before use.
    sortIfNeeded();
    refresh();

    m_accumulator += p_dt;
    int steps = 0;
    while ((m_accumulator >= m_fixed_step) && (steps < MAX_FIXED_STEPS))
    {
        for (Entry const& entry : m_order)
        {
            if (live(entry) && entry.routine->m_active)
            {
                entry.routine->fixedUpdate(m_fixed_step);
            }
        }
        m_accumulator -= m_fixed_step;
        ++steps;
    }
    if (steps == MAX_FIXED_STEPS)
    {
        m_accumulator = std::min(m_accumulator, m_fixed_step);
    }

    for (Entry const& entry : m_order)
    {
        if (live(entry) && entry.routine->m_active)
        {
            entry.routine->update(p_dt);
        }
    }
}

void RoutineSystem::lateUpdate(float p_dt)
{
    for (Entry const& entry : m_order)
    {
        if (live(entry) && entry.routine->m_active)
        {
            entry.routine->lateUpdate(p_dt);
        }
    }
}

} // namespace compages::world
