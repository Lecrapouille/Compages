// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/Core/Units.hpp"
#include "Compages/Core/Vector.hpp"

namespace compages::world
{



// ****************************************************************************
//! \brief Place, attitude and size of an entity relative to its parent.
//!
//! Value type: copy, serialise, store in a prefab. Live entities use
//! \c TransformStore (SoA); \c World::transform() returns a \c
//! LocalTransformView that aliases three slots in that store.
//!
//! \code
//! compages::world::LocalTransform saved = world.transform(entity).operator
//! LocalTransform(); entity.set(saved);
//! \endcode
// ****************************************************************************
struct LocalTransform
{
    compages::core::Vector3f position{ 0.0f, 0.0f, 0.0f };
    compages::core::Quatf rotation{};
    compages::core::Vector3f scale{ 1.0f, 1.0f, 1.0f };
};

// ****************************************************************************
//! \brief Build the local TRS matrix: translate, then rotate, then scale.
//!
//! Shared by \c LocalTransform, \c LocalTransformView and \c TransformSystem
//! so a hot update never packs SoA fields back into a struct first.
// ****************************************************************************
[[nodiscard]] inline compages::core::Matrix44f composeLocalMatrix(compages::core::Vector3f const& p_position,
                                                  compages::core::Quatf const& p_rotation,
                                                  compages::core::Vector3f const& p_scale)
{
    compages::core::Matrix44f matrix =
        compages::core::translate(compages::core::Matrix44f(compages::core::matrix::Identity), p_position);
    compages::core::Quatf turning = p_rotation;
    matrix = compages::core::rotate(matrix, turning.angle(), turning.axis());
    matrix = compages::core::scale(matrix, p_scale);
    return matrix;
}

// ****************************************************************************
//! \brief Local matrix of a TRS snapshot.
//!
//! \code
//! compages::world::LocalTransform pose{ .position = { 0, 1, 0 }, .scale = { 2, 2, 2 } };
//! compages::core::Matrix44f M = compages::world::localMatrix(pose);
//! \endcode
// ****************************************************************************
[[nodiscard]] inline compages::core::Matrix44f localMatrix(LocalTransform const& p_pose)
{
    return composeLocalMatrix(p_pose.position, p_pose.rotation, p_pose.scale);
}

// ****************************************************************************
//! \brief Mutable alias of one entity's TRS inside \c TransformStore.
//!
//! The three references are not a \c LocalTransform& : storage is structure-of-
//! arrays (position[], rotation[], scale[]), so one contiguous struct cannot
//! be referenced. Conceptually this *is* the entity's \c LocalTransform; it
//! reads/writes the same fields, and converts to \c LocalTransform for copies.
//!
//! \code
//! compages::world::LocalTransformView t = world.transform(entity);
//! t.position = { 1, 0, 0 };
//! t.rotateY(world.frame().elapsed);
//! compages::core::Matrix44f M = compages::world::localMatrix(static_cast<compages::world::LocalTransform>(t));
//! \endcode
// ****************************************************************************
class LocalTransformView
{
public:

    //! \brief Alias into the position SoA column.
    compages::core::Vector3f& position;
    //! \brief Alias into the rotation SoA column.
    compages::core::Quatf& rotation;
    //! \brief Alias into the scale SoA column.
    compages::core::Vector3f& scale;

    //! \brief Bind three SoA references for one entity slot.
    LocalTransformView(compages::core::Vector3f& p_position,
                       compages::core::Quatf& p_rotation,
                       compages::core::Vector3f& p_scale)
        : position(p_position), rotation(p_rotation), scale(p_scale)
    {
    }

    LocalTransformView(LocalTransformView const&) = default;

    //! \brief Copy TRS from another view of the same or another slot.
    LocalTransformView& operator=(LocalTransformView const& p_other)
    {
        position = p_other.position;
        rotation = p_other.rotation;
        scale = p_other.scale;
        return *this;
    }

    //! \brief Copy TRS from a value type (prefab, snapshot).
    LocalTransformView& operator=(LocalTransform const& p_value)
    {
        position = p_value.position;
        rotation = p_value.rotation;
        scale = p_value.scale;
        return *this;
    }

    //! \brief Snapshot the current TRS as a \c LocalTransform value.
    [[nodiscard]] operator LocalTransform() const
    {
        return LocalTransform{ position, rotation, scale };
    }

    //! \brief Rotate around an axis in the entity's local frame (three.js style).
    LocalTransformView& rotate(float p_radians, compages::core::Vector3f const& p_axis)
    {
        rotation =
            rotation * compages::core::Quatf::fromAngleAxis(units::angle::radian_t(p_radians),
                                            compages::core::vector::normalize(p_axis));
        rotation.normalize();
        return *this;
    }

    //! \brief Turn around the entity's own y axis (like a spinning top).
    LocalTransformView& rotateY(float p_radians)
    {
        return rotate(p_radians, compages::core::Vector3f(0.0f, 1.0f, 0.0f));
    }

    //! \brief Rotate around local +X.
    LocalTransformView& rotateX(float p_radians)
    {
        return rotate(p_radians, compages::core::Vector3f(1.0f, 0.0f, 0.0f));
    }

    //! \brief Rotate around local +Z.
    LocalTransformView& rotateZ(float p_radians)
    {
        return rotate(p_radians, compages::core::Vector3f(0.0f, 0.0f, 1.0f));
    }

    //! \brief Move by an offset in the parent's axes.
    LocalTransformView& translate(compages::core::Vector3f const& p_offset)
    {
        position += p_offset;
        return *this;
    }
};

[[nodiscard]] inline compages::core::Matrix44f localMatrix(LocalTransformView const& p_view)
{
    return localMatrix(static_cast<LocalTransform>(p_view));
}

} // namespace compages::world
