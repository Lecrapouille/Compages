// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Spatial/KinematicSystem.hpp"
#include "Compages/World/Spatial/Joint.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"

#include "Compages/World/Entt.hpp"

#include <cstring>

namespace compages::world
{

namespace
{

//! \brief Bitwise equality: "nothing was written since", not "close enough".
template <class T>
bool same(T const& p_a, T const& p_b)
{
    return std::memcmp(&p_a, &p_b, sizeof(T)) == 0;
}

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

LocalTransform jointTransform(RevoluteJoint const& p_joint)
{
    const units::angle::radian_t angle = p_joint.state.position.clamped();
    LocalTransform local = p_joint.origin;
    local.rotation = p_joint.origin.rotation *
                     compages::core::Quatf::fromAngleAxis(angle, compages::core::vector::normalize(p_joint.axis));
    local.rotation.normalize();
    return local;
}

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

void KinematicSystem::update(entt::registry& p_registry,
                             TransformStore& p_transforms) const
{
    apply<RevoluteJoint>(p_registry, p_transforms);
    apply<PrismaticJoint>(p_registry, p_transforms);
}

} // namespace compages::world
