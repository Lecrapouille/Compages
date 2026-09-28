// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/AABB.hpp"
#include "Compages/Core/Ray.hpp"
#include "Compages/Renderer/Components/MeshRenderer.hpp"
#include "Compages/World/World.hpp"

#include <optional>

namespace compages::renderer
{

// ****************************************************************************
//! \brief One MeshRenderer the geometric raycast hit.
//!
//! Distance is along the ray, in world units, because the Ray direction is
//! unit length. The point is \c ray.pointAt(distance).
//!
//! \code
//! if (hit) { compages::world::Entity e = world.entity(hit->entity); }
//! \endcode
// ****************************************************************************
struct RayHit
{
    //! \brief Entity that owns the hit \c MeshRenderer.
    compages::world::EntityId entity{};
    //! \brief Distance along the ray from its origin.
    float distance = 0.0f;
    //! \brief World-space intersection point.
    Vector3f point{ 0.0f, 0.0f, 0.0f };
};

// ****************************************************************************
//! \brief Closest MeshRenderer whose supplied world AABB the ray enters.
//!
//! The World does not own mesh bounds: those live on the asset. The caller
//! provides them, typically by transforming \c MeshAsset::local_bounds with
//! the entity's world matrix. Bounds come in as a callback, so this function
//! does not open the catalogue or the device.
//!
//! Disabled entities and entities that are not alive are skipped. An empty
//! box is a miss.
//!
//! \tparam BoundsFn callable \c AABB(EntityId, MeshRenderer const&).
//!
//! \code
//! auto hit = compages::renderer::raycast(world, ray, [&](compages::world::EntityId e, auto const&) {
//!     return transformedAabb(e);
//! });
//! \endcode
// ****************************************************************************
template <typename BoundsFn>
[[nodiscard]] std::optional<RayHit> raycast(compages::world::World const& p_world,
                                            Ray const& p_ray,
                                            BoundsFn&& p_world_bounds)
{
    std::optional<RayHit> best;
    p_world.each<MeshRenderer>(
        [&](compages::world::EntityId entity, MeshRenderer const& p_renderer) {
        if (!p_world.enabledInHierarchy(entity))
        {
            return;
        }
        const AABB box = p_world_bounds(entity, p_renderer);
        const std::optional<float> t = intersect(p_ray, box);
        if (!t.has_value())
        {
            return;
        }
        if (best.has_value() && (*t >= best->distance))
        {
            return;
        }
        RayHit hit;
        hit.entity = entity;
        hit.distance = *t;
        hit.point = p_ray.pointAt(*t);
        best = hit;
    });
    return best;
}

} // namespace compages::renderer
