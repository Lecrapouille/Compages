// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/Core/Vector.hpp"

namespace compages::renderer
{

// ****************************************************************************
//! \brief The world's ambient information: what a Scene tells the renderer
//! about the space around the entities.
//!
//! Kept small on purpose. IBL, skybox and HDR belong here when Stage 9 is
//! reached; the fields below are what the built-in lit material reads today.
// ****************************************************************************
struct Environment
{
    //! \brief A dim colour multiplied by the material colour to fake the light
    //! that comes from every direction.
    Vector3f ambient{ 0.15f, 0.15f, 0.18f };

    //! \brief Direction the built-in lit shader receives when no directional
    //! light is present in the World. Points toward the light (i.e. the
    //! opposite of what the photons travel).
    Vector3f default_light_direction{ 0.4f, 0.8f, 0.6f };

    //! \brief Colour of that light when no directional light is present. Black
    //! leaves the scene to its ambient and its lamps: a cave, a night.
    Vector3f default_light_color{ 1.0f, 1.0f, 1.0f };

    //! \brief What the distance fades to, and how fast. A density of zero is
    //! no fog; around 0.1 hides what is twenty units away.
    Vector3f fog_color{ 0.0f, 0.0f, 0.0f };
    float fog_density = 0.0f;
};

} // namespace compages::renderer
