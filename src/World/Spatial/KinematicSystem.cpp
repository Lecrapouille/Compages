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

#include "Compages/World/Spatial/KinematicSystem.hpp"
#include "Compages/World/Spatial/Joint.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"

#include "Compages/World/Entt.hpp"

#include <cstring>

namespace compages::world
{

namespace
{

//------------------------------------------------------------------------------
//! \brief Bitwise equality: "nothing was written since", not "close enough".
template <class T>
bool same(T const& p_a, T const& p_b)
{
    return std::memcmp(&p_a, &p_b, sizeof(T)) == 0;
}

//------------------------------------------------------------------------------
template <class Joint>
void apply(entt::registry& p_registry, TransformStore& p_transforms)
{
    for (auto [native, joint] : p_registry.view<Joint>().each())
    {
        const EntityId entity(native);
        if (!p_transforms.has(entity))
        {
            continue;
        }
        const LocalTransform local = jointTransform(joint);
        LocalTransformView slot = p_transforms.localMutable(entity);
        if (same(slot.position, local.position) &&
            same(slot.rotation, local.rotation) &&
            same(slot.scale, local.scale))
        {
            continue;
        }
        slot = local;
        p_transforms.markDirty(entity);
    }
}

} // namespace

//------------------------------------------------------------------------------
LocalTransform jointTransform(RevoluteJoint const& p_joint)
{
    const units::angle::radian_t angle = p_joint.state.position.clamped();
    LocalTransform local = p_joint.origin;
    local.rotation = p_joint.origin.rotation *
                     compages::core::Quatf::fromAngleAxis(angle, compages::core::vector::normalize(p_joint.axis));
    local.rotation.normalize();
    return local;
}

//------------------------------------------------------------------------------
LocalTransform jointTransform(PrismaticJoint const& p_joint)
{
    const float offset = p_joint.state.position.clamped().to<float>();
    const compages::core::Vector3f axis = compages::core::vector::normalize(p_joint.axis);
    const compages::core::Vector3f slide(axis.x * offset * p_joint.origin.scale.x,
                         axis.y * offset * p_joint.origin.scale.y,
                         axis.z * offset * p_joint.origin.scale.z);
    LocalTransform local = p_joint.origin;
    local.position += p_joint.origin.rotation * slide;
    return local;
}

//------------------------------------------------------------------------------
void KinematicSystem::update(entt::registry& p_registry,
                             TransformStore& p_transforms) const
{
    apply<RevoluteJoint>(p_registry, p_transforms);
    apply<PrismaticJoint>(p_registry, p_transforms);
}

} // namespace compages::world
