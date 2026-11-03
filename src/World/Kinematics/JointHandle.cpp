// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#include "Compages/World/Kinematics/JointHandle.hpp"
#include "Compages/World/World.hpp"

namespace compages::world
{

LocalTransform* JointHandle::origin() const
{
    if (auto* joint = m_world->tryGet<RevoluteJoint>(m_id))
    {
        return &joint->origin;
    }
    if (auto* joint = m_world->tryGet<PrismaticJoint>(m_id))
    {
        return &joint->origin;
    }
    return nullptr;
}

LocalTransform JointHandle::takeOrigin() const
{
    LocalTransform origin = m_world->transforms().local(m_id);
    if (LocalTransform const* previous = this->origin())
    {
        origin = *previous;
    }
    m_world->remove<RevoluteJoint>(m_id);
    m_world->remove<PrismaticJoint>(m_id);
    return origin;
}

JointHandle& JointHandle::revolute(compages::Vector3f const& p_axis,
                                   units::angle::radian_t p_min,
                                   units::angle::radian_t p_max)
{
    RevoluteJoint joint{ .origin = takeOrigin(), .axis = jointAxis(p_axis) };
    joint.state.position.min = p_min;
    joint.state.position.max = p_max;
    m_world->add(m_id, std::move(joint));
    return *this;
}

JointHandle& JointHandle::prismatic(compages::Vector3f const& p_axis,
                                    units::length::meter_t p_min,
                                    units::length::meter_t p_max)
{
    PrismaticJoint joint{ .origin = takeOrigin(), .axis = jointAxis(p_axis) };
    joint.state.position.min = p_min;
    joint.state.position.max = p_max;
    m_world->add(m_id, std::move(joint));
    return *this;
}

JointHandle& JointHandle::angle(units::angle::radian_t p_angle)
{
    m_world->get<RevoluteJoint>(m_id).state.position.value = p_angle;
    return *this;
}

units::angle::radian_t JointHandle::angle() const
{
    return m_world->get<RevoluteJoint>(m_id).state.position.applied();
}

units::angle::radian_t JointHandle::commandedAngle() const
{
    return m_world->get<RevoluteJoint>(m_id).state.position.value;
}

JointHandle& JointHandle::offset(units::length::meter_t p_offset)
{
    m_world->get<PrismaticJoint>(m_id).state.position.value = p_offset;
    return *this;
}

units::length::meter_t JointHandle::offset() const
{
    return m_world->get<PrismaticJoint>(m_id).state.position.applied();
}

units::length::meter_t JointHandle::commandedOffset() const
{
    return m_world->get<PrismaticJoint>(m_id).state.position.value;
}

} // namespace compages::world
