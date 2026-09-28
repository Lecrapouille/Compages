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

#include "Compages/Core/NotCopiable.hpp"
#include "Compages/Core/Result.hpp"
#include "Compages/World/Behavior.hpp"
#include "Compages/World/EntityId.hpp"
#include "Compages/World/Entt.hpp"
#include "Compages/World/Controllers/Input.hpp"
#include "Compages/Core/Frame.hpp"
#include "Compages/World/Spatial/KinematicSystem.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"
#include "Compages/World/Spatial/SpatialGraph.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"
#include "Compages/World/Spatial/TransformSystem.hpp"

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

// ****************************************************************************
//! \file
//! \brief The World: entities, their components, their hierarchy and their
//! behaviors. Headless: nothing here needs a GPU.
//!
//! The syntax is the one of flecs. An entity is created by name and given
//! components, which are plain structs:
//! \code
//! struct Position { float x, y; };
//! struct Velocity { float x, y; };
//!
//! compages::world::World world;
//! world.entity("player").set(Position{ 0, 0 }).set(Velocity{ 1, 0 });
//!
//! world.each<Position, Velocity>([](compages::world::Entity, Position& p, Velocity
//! const& v) {
//!     p.x += v.x;
//!     p.y += v.y;
//! });
//! \endcode
//!
//! Every entity also has a place in space, which can be chained the way
//! three.js does, and a parent:
//! \code
//! compages::world::Entity robot = world.entity("Robot").position(0, 1, 0);
//! robot.child("Head").position(0, 0.8f, 0).scale(0.5f);
//! \endcode
//!
//! A robot arm is the same hierarchy with joints between its links:
//! \code
//! compages::world::Entity base = world.entity("Base");
//! compages::world::Entity arm = base.child("Arm").position(0, 0, 0.5f)
//!                         .revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg);
//! arm.angle(30.0_deg);
//! \endcode
// ****************************************************************************

namespace compages::world
{

class Entity;
struct ViewFrame;

// ****************************************************************************
//! \brief The source of truth of a simulation: the entities and everything
//! attached to them.
//!
//! A World never touches the GPU. It never opens a window and never draws: a
//! test, a tool or a server runs the same simulation without a device. What
//! is drawn, and how, is the business of a compages::renderer::Scene looking at the World.
// ****************************************************************************
class World : private NotCopiable
{
public:

    World() = default;
    World(World&&) = default;
    World& operator=(World&&) = default;

    // ------------------------------------------------------------------------
    //! \brief Create an entity, named or not.
    //!
    //! \code
    //! compages::world::Entity player = world.entity("player");
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity entity(std::string p_name = {});

    //! \brief The handle of an entity known by its id.
    [[nodiscard]] Entity entity(EntityId p_id);

    // ------------------------------------------------------------------------
    //! \brief The entity at the end of a path of names, starting from the
    //! entities without a parent, or an empty handle.
    //!
    //! \code
    //! compages::world::Entity leg = world.lookup("Robot/Body/LeftLeg");
    //! \endcode
    // ------------------------------------------------------------------------
    [[nodiscard]] Entity lookup(std::string_view p_path);

    // ------------------------------------------------------------------------
    //! \brief Call a function for every entity having all the components T,
    //! with the entity and a reference to each component.
    //!
    //! \code
    //! world.each<Position, Velocity>([](compages::world::Entity e, Position& p,
    //! Velocity& v) { ... });
    //! \endcode
    // ------------------------------------------------------------------------
    // Defined in Entity.hpp, once the handle type is complete.
    template <typename... T, typename F>
    void each(F&& p_callback);

    //! \brief Same, read only. The entity is given by its id.
    template <typename... T, typename F>
    void each(F&& p_callback) const
    {
        m_registry.view<T...>().each(
            [&p_callback](entt::entity p_entity, T const&... p_components)
            { std::invoke(p_callback, EntityId(p_entity), p_components...); });
    }

    //! \brief Headless step: run behaviors, then propagate transforms.
    void update(Frame const& p_frame);

    //! \brief Interactive step: timing plus input for behaviors and controllers.
    //!
    //! \code
    //! world.update(viewFrame);   // gallery / Scene path
    //! \endcode
    void update(ViewFrame const& p_view);

    // ------------------------------------------------------------------------
    //! \brief Only compute the places in the world of the entities that moved,
    //! parents before children. Joints are turned into local places first.
    //! Run by update(Frame).
    // ------------------------------------------------------------------------
    void update();

    //! \brief Timing from the last update(Frame) or update(ViewFrame).
    [[nodiscard]] Frame const& frame() const
    {
        return m_frame;
    }

    //! \brief Input from the last update(ViewFrame), or default otherwise.
    [[nodiscard]] Input const& input() const
    {
        return m_input;
    }

    //! \brief How many entities are alive.
    [[nodiscard]] std::size_t living() const
    {
        return m_living;
    }

    // ------------------------------------------------------------------------
    // What follows works on ids and is what the Entity handle is made of. It
    // is also what the renderer, the loaders and the serializer use.
    // ------------------------------------------------------------------------

    //! \brief Create an entity and return its id. The entity has a place in
    //! space and no parent.
    [[nodiscard]] EntityId create(std::string p_name = {});

    //! \brief Destroy an entity, its children first, and all their
    //! components.
    void destroy(EntityId p_entity);

    //! \brief Is this handle still a live entity in the registry?
    [[nodiscard]] bool alive(EntityId p_entity) const;

    //! \brief The name of an entity, or an empty string.
    [[nodiscard]] std::string const& name(EntityId p_entity) const;
    //! \brief Rename an entity (used by lookup paths).
    void setName(EntityId p_entity, std::string p_name);

    //! \brief A disabled entity is skipped by the renderer and the behaviors,
    //! and so are its children.
    void setEnabled(EntityId p_entity, bool p_enabled);
    [[nodiscard]] bool enabled(EntityId p_entity) const;
    //! \brief Enabled on this entity and every ancestor?
    [[nodiscard]] bool enabledInHierarchy(EntityId p_entity) const;

    //! \brief Parent/child links (separate from component storage).
    [[nodiscard]] SpatialGraph& spatial()
    {
        return m_graph;
    }
    [[nodiscard]] SpatialGraph const& spatial() const
    {
        return m_graph;
    }

    //! \brief Change or clear the parent of an entity. Refused when either is
    //! dead or when it would make a cycle.
    [[nodiscard]] compages::Status
    setParent(EntityId p_child,
              EntityId p_parent,
              ReparentPolicy p_policy = ReparentPolicy::KeepLocal);

    //! \brief Parent entity id, or empty at a root.
    [[nodiscard]] EntityId parent(EntityId p_entity) const;
    //! \brief First child in insertion order.
    [[nodiscard]] EntityId firstChild(EntityId p_entity) const;
    //! \brief Next sibling, or empty at the end of the list.
    [[nodiscard]] EntityId nextSibling(EntityId p_entity) const;

    //! \brief The entity at the end of a path of names under p_root, as
    //! "Body/LeftLeg", or an empty id.
    [[nodiscard]] EntityId find(EntityId p_root, std::string_view p_path) const;

    //! \brief How many entities hang under this one, itself included.
    [[nodiscard]] std::size_t descendantCount(EntityId p_entity) const;

    //! \brief SoA local/world transforms for spatial entities.
    [[nodiscard]] TransformStore& transforms()
    {
        return m_transforms;
    }
    [[nodiscard]] TransformStore const& transforms() const
    {
        return m_transforms;
    }

    //! \brief The local place of an entity, to be changed. Its place in the
    //! world, and that of its children, are computed again at the next
    //! update().
    [[nodiscard]] LocalTransformView transform(EntityId p_entity);

    //! \brief A copy of the local place of an entity.
    [[nodiscard]] LocalTransform transform(EntityId p_entity) const;

    //! \brief The place of an entity in the world, as of the last update().
    [[nodiscard]] Matrix44f const& worldMatrix(EntityId p_entity) const;

    //! \brief The EnTT view of the entities having all the components T.
    template <typename... T>
    [[nodiscard]] auto view()
    {
        return m_registry.view<T...>();
    }

    template <typename... T>
    [[nodiscard]] auto view() const
    {
        return m_registry.view<T...>();
    }

    //! \brief Attach a component, replacing the one of that type.
    template <typename T>
    T& add(EntityId p_entity, T p_component = T{})
    {
        assert(alive(p_entity) && "World::add on a dead entity");
        return m_registry.emplace_or_replace<T>(p_entity.native(),
                                                std::move(p_component));
    }

    //! \brief Detach a component, if there is one.
    template <typename T>
    void remove(EntityId p_entity)
    {
        if (alive(p_entity))
        {
            m_registry.remove<T>(p_entity.native());
        }
    }

    //! \brief Does the entity have a component of type \c T?
    template <typename T>
    [[nodiscard]] bool has(EntityId p_entity) const
    {
        return alive(p_entity) && m_registry.all_of<T>(p_entity.native());
    }

    //! \brief The component, or nullptr when the entity has none.
    template <typename T>
    [[nodiscard]] T* tryGet(EntityId p_entity)
    {
        return alive(p_entity) ? m_registry.try_get<T>(p_entity.native())
                               : nullptr;
    }

    template <typename T>
    [[nodiscard]] T const* tryGet(EntityId p_entity) const
    {
        return alive(p_entity) ? m_registry.try_get<T>(p_entity.native())
                               : nullptr;
    }

    //! \brief The component, which the entity must have.
    template <typename T>
    [[nodiscard]] T& get(EntityId p_entity)
    {
        assert(has<T>(p_entity) && "World::get on an entity without one");
        return m_registry.get<T>(p_entity.native());
    }

    template <typename T>
    [[nodiscard]] T const& get(EntityId p_entity) const
    {
        assert(has<T>(p_entity) && "World::get on an entity without one");
        return m_registry.get<T>(p_entity.native());
    }

    //! \brief Attach a behavior to an entity. What Entity::add<T>() does when
    //! T derives from Behavior.
    Behavior& addBehavior(EntityId p_entity,
                          std::unique_ptr<Behavior> p_behavior);

    //! \brief How many behaviors an entity has.
    [[nodiscard]] std::size_t behaviorCount(EntityId p_entity) const;

    //! \brief The first behavior of type T of an entity, or nullptr.
    template <typename T>
    [[nodiscard]] T* behavior(EntityId p_entity)
    {
        Behaviors* behaviors = tryGet<Behaviors>(p_entity);
        if (behaviors == nullptr)
        {
            return nullptr;
        }
        for (std::unique_ptr<Behavior> const& behavior : behaviors->list)
        {
            if (T* found = dynamic_cast<T*>(behavior.get()))
            {
                return found;
            }
        }
        return nullptr;
    }

    //! \brief Take the behaviors of type T off an entity. Not from inside one
    //! of them: it would be destroyed while it runs.
    template <typename T>
    void removeBehaviors(EntityId p_entity)
    {
        if (Behaviors* behaviors = tryGet<Behaviors>(p_entity))
        {
            std::erase_if(
                behaviors->list,
                [](std::unique_ptr<Behavior> const& p_behavior)
                { return dynamic_cast<T*>(p_behavior.get()) != nullptr; });
        }
    }

private:

    void destroyRecursive(EntityId p_entity);
    void runBehaviors();

    struct Name
    {
        std::string value;
    };

    struct Disabled
    {
    };

    //! \brief The behaviors of one entity, as a component.
    struct Behaviors : NotCopiable
    {
        Behaviors() = default;
        Behaviors(Behaviors&&) = default;
        Behaviors& operator=(Behaviors&&) = default;

        std::vector<std::unique_ptr<Behavior>> list;
    };

    entt::registry m_registry;
    std::size_t m_living = 0u;
    SpatialGraph m_graph;
    TransformStore m_transforms;
    KinematicSystem m_kinematic_system;
    TransformSystem m_transform_system;
    Frame m_frame{};
    Input m_input{};

    static const std::string s_empty_name;
};

} // namespace compages::world
