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
    Vector3f position{ 0.0f, 0.0f, 0.0f };
    Quatf rotation{};
    Vector3f scale{ 1.0f, 1.0f, 1.0f };
};

// ****************************************************************************
//! \brief Build the local TRS matrix: translate, then rotate, then scale.
//!
//! Shared by \c LocalTransform, \c LocalTransformView and \c TransformSystem
//! so a hot update never packs SoA fields back into a struct first.
// ****************************************************************************
[[nodiscard]] inline Matrix44f composeLocalMatrix(Vector3f const& p_position,
                                                  Quatf const& p_rotation,
                                                  Vector3f const& p_scale)
{
    Matrix44f matrix =
        compages::matrix::translate(Matrix44f(compages::matrix::Identity), p_position);
    Quatf turning = p_rotation;
    matrix = compages::matrix::rotate(matrix, turning.angle(), turning.axis());
    matrix = compages::matrix::scale(matrix, p_scale);
    return matrix;
}

// ****************************************************************************
//! \brief Local matrix of a TRS snapshot.
//!
//! \code
//! compages::world::LocalTransform pose{ .position = { 0, 1, 0 }, .scale = { 2, 2, 2 } };
//! Matrix44f M = compages::world::localMatrix(pose);
//! \endcode
// ****************************************************************************
[[nodiscard]] inline Matrix44f localMatrix(LocalTransform const& p_pose)
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
//! Matrix44f M = compages::world::localMatrix(static_cast<compages::world::LocalTransform>(t));
//! \endcode
// ****************************************************************************
class LocalTransformView
{
public:

    //! \brief Alias into the position SoA column.
    Vector3f& position;
    //! \brief Alias into the rotation SoA column.
    Quatf& rotation;
    //! \brief Alias into the scale SoA column.
    Vector3f& scale;

    //! \brief Bind three SoA references for one entity slot.
    LocalTransformView(Vector3f& p_position,
                       Quatf& p_rotation,
                       Vector3f& p_scale)
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
    LocalTransformView& rotate(float p_radians, Vector3f const& p_axis)
    {
        rotation =
            rotation * Quatf::fromAngleAxis(units::angle::radian_t(p_radians),
                                            compages::vector::normalize(p_axis));
        rotation.normalize();
        return *this;
    }

    //! \brief Turn around the entity's own y axis (like a spinning top).
    LocalTransformView& rotateY(float p_radians)
    {
        return rotate(p_radians, Vector3f(0.0f, 1.0f, 0.0f));
    }

    //! \brief Rotate around local +X.
    LocalTransformView& rotateX(float p_radians)
    {
        return rotate(p_radians, Vector3f(1.0f, 0.0f, 0.0f));
    }

    //! \brief Rotate around local +Z.
    LocalTransformView& rotateZ(float p_radians)
    {
        return rotate(p_radians, Vector3f(0.0f, 0.0f, 1.0f));
    }

    //! \brief Move by an offset in the parent's axes.
    LocalTransformView& translate(Vector3f const& p_offset)
    {
        position += p_offset;
        return *this;
    }
};

[[nodiscard]] inline Matrix44f localMatrix(LocalTransformView const& p_view)
{
    return localMatrix(static_cast<LocalTransform>(p_view));
}

} // namespace compages::world
