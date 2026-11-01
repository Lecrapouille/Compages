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
    RoutineList* routines = m_world.tryGet<RoutineList>(p_entity);
    if (routines == nullptr)
    {
        routines = &m_world.add(p_entity, RoutineList{});
    }
    routines->list.emplace_back(std::move(p_routine));
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

void RoutineSystem::removeIf(EntityId p_entity, bool (*p_match)(Routine const*))
{
    if (RoutineList* routines = m_world.tryGet<RoutineList>(p_entity))
    {
        std::erase_if(routines->list,
                      [p_match](std::unique_ptr<Routine> const& p_routine)
                      { return p_match(p_routine.get()); });
    }
}

void RoutineSystem::update(float p_dt)
{
    // A routine may create or destroy entities, or add routines: the list
    // of who runs this frame is taken first.
    struct Running
    {
        EntityId entity;
        Routine* routine;
    };
    std::vector<Running> running;
    for (auto [native, routines] : m_world.view<RoutineList>().each())
    {
        if (!m_world.enabledInHierarchy(EntityId(native)))
        {
            continue;
        }
        for (std::unique_ptr<Routine> const& routine : routines.list)
        {
            running.emplace_back(Running{ EntityId(native), routine.get() });
        }
    }
    for (Running const& run : running)
    {
        // Gone if an earlier routine of this same frame destroyed it: the
        // pointer is only followed once found again in its entity.
        RoutineList const* owner = listOf(run.entity);
        if ((owner == nullptr) ||
            std::none_of(owner->list.begin(),
                         owner->list.end(),
                         [&run](std::unique_ptr<Routine> const& p_kept)
                         { return p_kept.get() == run.routine; }))
        {
            continue;
        }
        Routine* routine = run.routine;
        if (!routine->m_started)
        {
            routine->m_started = true;
            routine->start();
        }
        routine->update(p_dt);
    }
}

} // namespace compages::world
