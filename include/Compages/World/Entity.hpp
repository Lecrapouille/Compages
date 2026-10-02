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

#include "Compages/World/Spatial/Joint.hpp"
#include "Compages/World/Spatial/LookAt.hpp"
#include "Compages/World/World.hpp"

#include <cassert>
#include <functional>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"

namespace compages::world
{

class World;

// ****************************************************************************
//! \brief Handle on one entity inside a \c World: chain calls like flecs or
//! three.js.
//!
//! \code
//! compages::world::Entity ship = world.entity("Ship").position(0, 2, 0);
//! ship.child("Mast").scale(0.5f).add<Spin>(1.0f);
//! if (ship.has<MeshRenderer>()) { ship.get<MeshRenderer>().mesh = id; }
//! \endcode
// ****************************************************************************
class Entity
{
public:

    // ------------------------------------------------------------------------
    //! \brief An empty handle, naming nothing.
    // ------------------------------------------------------------------------
    Entity() = default;

    // ------------------------------------------------------------------------
    //! \brief Bind a live \c EntityId to its \c World.
    // ------------------------------------------------------------------------
    Entity(World& p_world, EntityId p_id) : m_world(&p_world), m_id(p_id) {}

    // ------------------------------------------------------------------------
    //! \brief The id of the entity inside its World.
    // ------------------------------------------------------------------------
    [[nodiscard]] EntityId id() const
    {
        return m_id;
    }

    // ------------------------------------------------------------------------
    //! \brief The id, for the functions that take one.
    // ------------------------------------------------------------------------
    [[nodiscard]] operator EntityId() const
    {
        return m_id;
    }

    // ------------------------------------------------------------------------
    //! \brief Does the handle name an entity still alive?
    // ------------------------------------------------------------------------
    [[nodiscard]] explicit operator bool() const
    {
        return (m_world != nullptr) && m_world->alive(m_id);
    }

    // ------------------------------------------------------------------------
    [[nodiscard]] bool operator==(Entity const& p_other) const
    {
        return (m_world == p_other.m_world) && (m_id == p_other.m_id);
    }

    // ------------------------------------------------------------------------
    //! \brief The World the entity lives in.
    // ------------------------------------------------------------------------
    [[nodiscard]] World& world() const
    {
        assert(m_world != nullptr && "an empty entity handle has no world");
        return *m_world;
    }

    // ------------------------------------------------------------------------
    //! \brief Attach a component, replacing the one of that type.
    //! \code
    //! player.set(Velocity{ 1, 0 });
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename T>
    Entity& set(T p_component)
    {
        world().add(m_id, std::move(p_component));
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Attach a component built from the arguments, or a behavior when
    //! T derives from Behavior.
    //!
    //! \code
    //! enemy.add<Hostile>();           // a tag: a component without data
    //! cube.add<Spin>(2.0f);           // a behavior, built with Spin(2.0f)
    //! \endcode
    // ------------------------------------------------------------------------
    template <typename T, typename... Args>
    Entity& add(Args&&... p_args)
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            world().addBehavior(
                m_id, std::make_unique<T>(std::forward<Args>(p_args)...));
        }
        else if constexpr (std::is_constructible_v<T, Args...>)
        {
            world().add(m_id, T(std::forward<Args>(p_args)...));
        }
        else
        {
            world().add(m_id, T{ std::forward<Args>(p_args)... });
        }
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Detach the component of type T, or the behaviors of type T.
    // ------------------------------------------------------------------------
    template <typename T>
    Entity& remove()
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            world().template removeBehaviors<T>(m_id);
        }
        else
        {
            world().template remove<T>(m_id);
        }
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Has the entity a component, or a behavior, of type T?
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] bool has() const
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            return find<T>() != nullptr;
        }
        else
        {
            return (m_world != nullptr) && m_world->template has<T>(m_id);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief The component, which the entity must have.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] T& get() const
    {
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            T* behavior = find<T>();
            assert((behavior != nullptr) &&
                   "Entity::get of a missing behavior");
            return *behavior;
        }
        else
        {
            return world().template get<T>(m_id);
        }
    }

    // ------------------------------------------------------------------------
    //! \brief The component, or the behavior, of type T; nullptr when the
    //! entity has none.
    // ------------------------------------------------------------------------
    template <typename T>
    [[nodiscard]] T* find() const
    {
        if (m_world == nullptr)
        {
            return nullptr;
        }
        if constexpr (std::is_base_of_v<Behavior, T>)
        {
            return m_world->template behavior<T>(m_id);
        }
        else
        {
            return m_world->template tryGet<T>(m_id);
        }
    }

    // ------------------------------------------------------------------------
    // Identity
    // ------------------------------------------------------------------------

    //! \brief Current entity name (empty if unnamed).
    [[nodiscard]] std::string const& name() const
    {
        return world().name(m_id);
    }

    // ------------------------------------------------------------------------
    //! \brief Rename and keep chaining.
    // ------------------------------------------------------------------------
    Entity& name(std::string p_name)
    {
        world().setName(m_id, std::move(p_name));
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Show and run the entity and its children, or neither.
    // ------------------------------------------------------------------------
    Entity& enable(bool p_enabled = true)
    {
        world().setEnabled(m_id, p_enabled);
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Visible to renderer and behaviors (checks ancestors too)?
    // ------------------------------------------------------------------------
    [[nodiscard]] bool enabled() const
    {
        return world().enabledInHierarchy(m_id);
    }

    // ------------------------------------------------------------------------
    //! \brief Destroy the entity and its children.
    // ------------------------------------------------------------------------
    void destroy() const
    {
        world().destroy(m_id);
    }

    // ------------------------------------------------------------------------
    // Hierarchy
    // ------------------------------------------------------------------------

    //! \brief Create an entity hanging from this one. It moves with it.
    [[nodiscard]] Entity child(std::string p_name = {}) const;

    //! \brief Hang this entity from another one, keeping its local place.
    Entity& parent(Entity const& p_parent)
    {
        [[maybe_unused]] const auto done =
            bool(world().setParent(m_id, p_parent.m_id));
        assert(done && "parent(): a dead entity, or a cycle");
        return *this;
    }

    //! \brief Same as parent(Entity), but reports a
    //! failure(dead entity, or a cycle) instead of only
    //! asserting, so that release builds can react.
    //!
    //! \code
    //! if (!child.setParent(newParent)) { /* refused */ }
    //! \endcode
    [[nodiscard]] Status
    setParent(Entity const& p_parent,
              ReparentPolicy p_policy = ReparentPolicy::KeepLocal) const
    {
        return world().setParent(m_id, p_parent.m_id, p_policy);
    }

    //! \brief The entity this one hangs from, or an empty handle.
    [[nodiscard]] Entity parent() const
    {
        return Entity(world(), world().parent(m_id));
    }

    //! \brief The entity at the end of a path of names under this one, as
    //! "Body/LeftLeg", or an empty handle.
    [[nodiscard]] Entity lookup(std::string_view p_path) const
    {
        return Entity(world(), world().find(m_id, p_path));
    }

    //! \brief Call a function with each child, in order.
    template <typename F>
    void children(F&& p_callback) const
    {
        EntityId child = world().firstChild(m_id);
        while (child.valid())
        {
            const EntityId next = world().nextSibling(child);
            std::invoke(p_callback, Entity(world(), child));
            child = next;
        }
    }

    // ------------------------------------------------------------------------
    // Place, relative to the parent. On a joint, the place of the joint frame
    // when the joint is at zero: the joint then moves the entity from there.
    // ------------------------------------------------------------------------

    Entity& position(float p_x, float p_y, float p_z)
    {
        return position(compages::core::Vector3f(p_x, p_y, p_z));
    }

    Entity& position(compages::core::Vector3f const& p_position)
    {
        place().position = p_position;
        return *this;
    }

    [[nodiscard]] compages::core::Vector3f position() const
    {
        if (LocalTransform const* origin = jointOrigin())
        {
            return origin->position;
        }
        return world().transforms().position(m_id);
    }

    Entity& rotation(compages::core::Quatf const& p_rotation)
    {
        place().rotation = p_rotation;
        return *this;
    }

    //! \brief Set the orientation to an angle around an axis.
    Entity& rotation(Radians p_radians, compages::core::Vector3f const& p_axis)
    {
        return rotation(compages::core::Quatf::fromAngleAxis(
            p_radians, compages::core::vector::normalize(p_axis)));
    }

    [[nodiscard]] compages::core::Quatf rotation() const
    {
        if (LocalTransform const* origin = jointOrigin())
        {
            return origin->rotation;
        }
        return world().transforms().rotation(m_id);
    }

    //! \brief Turn by an angle around an axis of the entity.
    Entity& rotate(Radians p_radians, compages::core::Vector3f const& p_axis)
    {
        place().rotate(p_radians, p_axis);
        return *this;
    }

    Entity& scale(float p_scale)
    {
        return scale(compages::core::Vector3f(p_scale, p_scale, p_scale));
    }

    Entity& scale(float p_x, float p_y, float p_z)
    {
        return scale(compages::core::Vector3f(p_x, p_y, p_z));
    }

    Entity& scale(compages::core::Vector3f const& p_scale)
    {
        place().scale = p_scale;
        return *this;
    }

    [[nodiscard]] compages::core::Vector3f scale() const
    {
        if (LocalTransform const* origin = jointOrigin())
        {
            return origin->scale;
        }
        return world().transforms().scale(m_id);
    }

    //! \brief Turn so as to face a point, given in the parent's axes: the
    //! entity's -z axis points at it, as a camera looks.
    Entity& lookAt(compages::core::Vector3f const& p_target,
                   compages::core::Vector3f const& p_up =
                       compages::core::Vector3f(0.0f, 1.0f, 0.0f))
    {
        compages::world::lookAt(place(), p_target, p_up);
        return *this;
    }

    Entity& lookAt(float p_x, float p_y, float p_z)
    {
        return lookAt(compages::core::Vector3f(p_x, p_y, p_z));
    }

    // ------------------------------------------------------------------------
    // Joints: what moves a link of a robot relative to its parent link
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    //! \brief Make the entity turn around an axis, within an interval of
    //! angles. Its current place becomes the place of the joint at zero.
    //!
    //! \code
    //! arm.revolute({ 0, 0, 1 }, -90.0_deg, 90.0_deg).angle(30.0_deg);
    //! wheel.revolute({ 1, 0, 0 });    // no bounds: a continuous joint
    //! \endcode
    // ------------------------------------------------------------------------
    Entity& revolute(compages::core::Vector3f const& p_axis,
                     units::angle::radian_t p_min = units::angle::radian_t(
                         -std::numeric_limits<double>::infinity()),
                     units::angle::radian_t p_max = units::angle::radian_t(
                         std::numeric_limits<double>::infinity()))
    {
        RevoluteJoint joint{ .origin = takeOrigin(),
                             .axis =
                                 compages::core::vector::normalize(p_axis) };
        joint.state.position.min = p_min;
        joint.state.position.max = p_max;
        world().add(m_id, std::move(joint));
        return *this;
    }

    // ------------------------------------------------------------------------
    //! \brief Make the entity slide along an axis, within an interval of
    //! offsets. Its current place becomes the place of the joint at zero.
    //!
    //! \code
    //! slider.prismatic({ 0, 0, 1 }, 0.0_m, 0.3_m).offset(0.1_m);
    //! \endcode
    // ------------------------------------------------------------------------
    Entity& prismatic(compages::core::Vector3f const& p_axis,
                      units::length::meter_t p_min = units::length::meter_t(
                          -std::numeric_limits<double>::infinity()),
                      units::length::meter_t p_max = units::length::meter_t(
                          std::numeric_limits<double>::infinity()))
    {
        PrismaticJoint joint{ .origin = takeOrigin(),
                              .axis =
                                  compages::core::vector::normalize(p_axis) };
        joint.state.position.min = p_min;
        joint.state.position.max = p_max;
        world().add(m_id, std::move(joint));
        return *this;
    }

    //! \brief Set the angle of the revolute joint. Kept inside its bounds when
    //! the World is updated.
    Entity& angle(units::angle::radian_t p_angle)
    {
        world().template get<RevoluteJoint>(m_id).state.position.value =
            p_angle;
        return *this;
    }

    //! \brief The angle of the revolute joint, as last set.
    [[nodiscard]] units::angle::radian_t angle() const
    {
        return world().template get<RevoluteJoint>(m_id).state.position.value;
    }

    //! \brief Set the offset of the prismatic joint. Kept inside its bounds
    //! when the World is updated.
    Entity& offset(units::length::meter_t p_offset)
    {
        world().template get<PrismaticJoint>(m_id).state.position.value =
            p_offset;
        return *this;
    }

    //! \brief The offset of the prismatic joint, as last set.
    [[nodiscard]] units::length::meter_t offset() const
    {
        return world().template get<PrismaticJoint>(m_id).state.position.value;
    }

    //! \brief The local place, to be changed field by field. On a joint, it is
    //! overwritten by the joint at the next update: change position() or
    //! rotation() instead.
    [[nodiscard]] LocalTransformView transform() const
    {
        return world().transform(m_id);
    }

    //! \brief The place in the world, as of the last update of the World.
    [[nodiscard]] compages::core::Matrix44f const& worldMatrix() const
    {
        return world().worldMatrix(m_id);
    }

    //! \brief Where the entity is in the world, as of the last update.
    [[nodiscard]] compages::core::Vector3f worldPosition() const
    {
        compages::core::Matrix44f const& m = worldMatrix();
        return compages::core::translation(m);
    }

private:

    //! \brief The origin of the joint of the entity, or nullptr without one.
    [[nodiscard]] LocalTransform* jointOrigin() const
    {
        if (m_world == nullptr)
        {
            return nullptr;
        }
        if (auto* joint = m_world->template tryGet<RevoluteJoint>(m_id))
        {
            return &joint->origin;
        }
        if (auto* joint = m_world->template tryGet<PrismaticJoint>(m_id))
        {
            return &joint->origin;
        }
        return nullptr;
    }

    //! \brief What position(), rotation() and scale() change: the joint
    //! origin, or else the local place.
    [[nodiscard]] LocalTransformView place() const
    {
        if (LocalTransform* origin = jointOrigin())
        {
            return LocalTransformView(
                origin->position, origin->rotation, origin->scale);
        }
        return transform();
    }

    //! \brief The origin of a new joint: that of the joint being replaced, or
    //! the local place. Removes the joint being replaced.
    [[nodiscard]] LocalTransform takeOrigin() const
    {
        LocalTransform origin = world().transforms().local(m_id);
        if (LocalTransform const* previous = jointOrigin())
        {
            origin = *previous;
        }
        world().template remove<RevoluteJoint>(m_id);
        world().template remove<PrismaticJoint>(m_id);
        return origin;
    }

    World* m_world = nullptr;
    EntityId m_id{};
};

inline Entity Behavior::entity() const
{
    return Entity(*m_world, m_entity);
}

inline LocalTransformView Behavior::transform() const
{
    return m_world->transform(m_entity);
}

inline Input const& Behavior::input() const
{
    return m_world->input();
}

inline compages::core::Frame const& Behavior::frame() const
{
    return m_world->frame();
}

template <typename... T, typename F>
inline void World::each(F&& p_callback)
{
    m_registry.view<T...>().each(
        [this, &p_callback](entt::entity p_entity, T&... p_components)
        {
            std::invoke(
                p_callback, Entity(*this, EntityId(p_entity)), p_components...);
        });
}

} // namespace compages::world
