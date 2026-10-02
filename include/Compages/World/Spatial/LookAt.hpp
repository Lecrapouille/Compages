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

#include "Compages/Core/Matrix.hpp"
#include "Compages/Core/Vector.hpp"

namespace compages::world
{

// ------------------------------------------------------------------------
//! \brief Set \c p_rotation so local \c -Z points from \c p_position at
//! \c p_target.
//!
//! Builds a view matrix with \c compages::core::lookAt(), then stores the
//! inverse as a quaternion — the same convention as \c Object3D.lookAt() in
//! three.js.
//! \c p_position is read only; on a degenerate direction the rotation is left
//! unchanged.
//!
//! \param[in] p_position Eye or object origin in the parent space.
//! \param[out] p_rotation Local attitude to update.
//! \param[in] p_target Point to face in the same space as \c p_position.
//! \param[in] p_up Hint axis to resolve roll (usually world or parent up).
// ------------------------------------------------------------------------
inline void applyLookAt(compages::core::Vector3f const& p_position,
                        compages::core::Quatf& p_rotation,
                        compages::core::Vector3f const& p_target,
                        compages::core::Vector3f const& p_up)
{
    const compages::core::Vector3f delta = p_target - p_position;
    if (compages::core::vector::norm(delta) < 1.0e-6f)
    {
        return;
    }

    const compages::core::Matrix44f view =
        compages::core::lookAt(p_position, p_target, p_up);
    p_rotation =
        compages::core::Quatf::fromMatrix(compages::core::inverse(view));
}

// ------------------------------------------------------------------------
//! \brief Rotate a \c LocalTransform value in place toward \c p_target.
//! \param[in,out] p_transform Position and rotation; scale is untouched.
//! \param[in] p_target Point to face.
//! \param[in] p_up Up vector for the lookAt basis (default +Y).
// ------------------------------------------------------------------------
inline void lookAt(LocalTransform& p_transform,
                   compages::core::Vector3f const& p_target,
                   compages::core::Vector3f const& p_up =
                       compages::core::Vector3f(0.0f, 1.0f, 0.0f))
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
                   compages::core::Vector3f const& p_target,
                   compages::core::Vector3f const& p_up =
                       compages::core::Vector3f(0.0f, 1.0f, 0.0f))
{
    applyLookAt(p_transform.position, p_transform.rotation, p_target, p_up);
}

} // namespace compages::world
