// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

namespace compages::world
{

// ****************************************************************************
//! \brief A light that does not fall off with distance.
//!
//! Where it points is the EntityId's forward axis (the local \c -Z direction of
//! the world matrix), so a directional light parented to a rig turns with the
//! rig. Rotate the entity to aim the light.
//!
//! \code
//! sun.set(compages::world::DirectionalLight{ .intensity = 3.0f });
//! \endcode
// ****************************************************************************
struct DirectionalLight
{
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
};

// ****************************************************************************
//! \brief A light at a point, falling off with distance.
//!
//! The point in space is the EntityId's world position. The renderer reads the
//! entity's world matrix at extraction time and passes the position to the
//! shader; the component itself holds only what does not come from the
//! transform.
//!
//! \code
//! lamp.set(compages::world::PointLight{ .color = { 1, 0.8f, 0.5f }, .range = 8.0f });
//! \endcode
// ****************************************************************************
struct PointLight
{
    Vector3f color{ 1.0f, 1.0f, 1.0f };
    float intensity = 1.0f;
    //! \brief Distance at which the light has faded out: a fifth of it at
    //! half this distance, nothing beyond.
    float range = 100.0f;
};

} // namespace compages::world
