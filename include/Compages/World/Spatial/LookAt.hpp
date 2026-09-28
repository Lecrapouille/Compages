// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
#include "Compages/World/Spatial/LocalTransform.hpp"

// ****************************************************************************
//! \file
//! \brief Orient entities toward a world-space target (three.js-style lookAt).
//!
//! Used by \c Entity::lookAt() and demos that aim meshes at a moving point.
//!
//! \code
//! compages::world::LocalTransformView t = world.transform(camera);
//! compages::world::lookAt(t, { 0, 0, 0 });
//! \endcode
// ****************************************************************************

namespace compages::world
{

// ------------------------------------------------------------------------
//! \brief Set \c p_rotation so local \c -Z points from \c p_position at
//! \c p_target.
//!
//! Builds a view matrix with \c compages::matrix::lookAt(), then stores the inverse as
//! a quaternion — the same convention as \c Object3D.lookAt() in three.js.
//! \c p_position is read only; on a degenerate direction the rotation is left
//! unchanged.
//!
//! \param[in] p_position Eye or object origin in the parent space.
//! \param[out] p_rotation Local attitude to update.
//! \param[in] p_target Point to face in the same space as \c p_position.
//! \param[in] p_up Hint axis to resolve roll (usually world or parent up).
// ------------------------------------------------------------------------
inline void applyLookAt(Vector3f const& p_position,
                        Quatf& p_rotation,
                        Vector3f const& p_target,
                        Vector3f const& p_up)
{
    const Vector3f delta = p_target - p_position;
    if (compages::vector::norm(delta) < 1.0e-6f)
    {
        return;
    }

    const Matrix44f view = compages::matrix::lookAt(p_position, p_target, p_up);
    p_rotation = Quatf::fromMatrix(compages::matrix::inverse(view));
}

// ------------------------------------------------------------------------
//! \brief Rotate a \c LocalTransform value in place toward \c p_target.
//! \param[in,out] p_transform Position and rotation; scale is untouched.
//! \param[in] p_target Point to face.
//! \param[in] p_up Up vector for the lookAt basis (default +Y).
// ------------------------------------------------------------------------
inline void lookAt(LocalTransform& p_transform,
                   Vector3f const& p_target,
                   Vector3f const& p_up = Vector3f(0.0f, 1.0f, 0.0f))
{
    applyLookAt(p_transform.position, p_transform.rotation, p_target, p_up);
}

// ------------------------------------------------------------------------
//! \brief Rotate an entity's SoA slot via \c LocalTransformView.
//! \param[in,out] p_transform Mutable view into \c TransformStore.
//! \param[in] p_target Point to face in the entity's parent space.
//! \param[in] p_up Up vector for the lookAt basis (default +Y).
// ------------------------------------------------------------------------
inline void lookAt(LocalTransformView p_transform,
                   Vector3f const& p_target,
                   Vector3f const& p_up = Vector3f(0.0f, 1.0f, 0.0f))
{
    applyLookAt(p_transform.position, p_transform.rotation, p_target, p_up);
}

} // namespace compages::world
