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
#include <memory>
#include <vector>

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief The routines of one entity, as a component of the World: they are
//! destroyed with their entity.
// ****************************************************************************
struct RoutineList: compages::core::NotCopiable
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
//! through it when T derives from Routine, so most code never calls it.
//!
//! \code
//! compages::world::Orbit* orbit = world.routines().find<compages::world::Orbit>(camera);
//! \endcode
// ****************************************************************************
class RoutineSystem: private compages::core::NotCopiable
{
public:

    explicit RoutineSystem(World& p_world) : m_world(p_world) {}
    RoutineSystem(RoutineSystem&&) = delete;
    RoutineSystem& operator=(RoutineSystem&&) = delete;

    //! \brief Attach a routine to an entity. What Entity::add<T>() does when
    //! T derives from Routine.
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

    //! \brief Take the routines of type T off an entity. Not from inside one
    //! of them: it would be destroyed while it runs.
    template <typename T>
    void remove(EntityId p_entity)
    {
        removeIf(p_entity,
                 [](Routine const* p_routine)
                 { return dynamic_cast<T const*>(p_routine) != nullptr; });
    }

    //! \brief Run the routines of the enabled entities: start() on the new
    //! ones, then update().
    void update(float p_dt);

private:

    [[nodiscard]] RoutineList const* listOf(EntityId p_entity) const;
    void removeIf(EntityId p_entity, bool (*p_match)(Routine const*));

    World& m_world;
};

} // namespace compages::world
