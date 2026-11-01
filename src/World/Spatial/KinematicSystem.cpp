// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Spatial/KinematicSystem.hpp"
#include "Compages/World/Components/PoseDriver.hpp"
#include "Compages/World/Spatial/Joint.hpp"
#include "Compages/World/Spatial/TransformStore.hpp"

#include "Compages/World/Entt.hpp"

#include <cmath>
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
        if (!p_transforms.has(entity) ||
            !mayWrite(p_registry.try_get<PoseDriver>(native),
                      PoseDriver::Kinematic))
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

compages::core::Vector3f jointAxis(compages::core::Vector3f const& p_axis)
{
    const float length = p_axis.norm();
    if (!std::isfinite(length) || (length < 1.0e-6f))
    {
        return compages::core::Vector3f(0.0f, 0.0f, 0.0f);
    }
    return p_axis / length;
}

LocalTransform jointTransform(RevoluteJoint const& p_joint)
{
    const compages::core::Vector3f axis = jointAxis(p_joint.axis);
    if (axis.norm() < 0.5f) // zero axis: the link does not move
    {
        return p_joint.origin;
    }
    const units::angle::radian_t angle = p_joint.state.position.applied();
    LocalTransform local = p_joint.origin;
    local.rotation = p_joint.origin.rotation *
                     compages::core::Quatf::fromAngleAxis(angle, axis);
    local.rotation.normalize();
    return local;
}

LocalTransform jointTransform(PrismaticJoint const& p_joint)
{
    const float offset = p_joint.state.position.applied().to<float>();
    const compages::core::Vector3f axis = jointAxis(p_joint.axis);
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
