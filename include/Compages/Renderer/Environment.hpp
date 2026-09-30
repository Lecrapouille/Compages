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

#include "Compages/Core/Vector.hpp"

#include "Compages/Core/Quaternion.hpp"
#include "Compages/Core/Transformation.hpp"
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
    compages::core::Vector3f ambient{ 0.15f, 0.15f, 0.18f };

    //! \brief Direction the built-in lit shader receives when no directional
    //! light is present in the compages::world::World. Points toward the light (i.e. the
    //! opposite of what the photons travel).
    compages::core::Vector3f default_light_direction{ 0.4f, 0.8f, 0.6f };

    //! \brief Colour of that light when no directional light is present. Black
    //! leaves the scene to its ambient and its lamps: a cave, a night.
    compages::core::Vector3f default_light_color{ 1.0f, 1.0f, 1.0f };

    //! \brief What the distance fades to, and how fast. A density of zero is
    //! no fog; around 0.1 hides what is twenty units away.
    compages::core::Vector3f fog_color{ 0.0f, 0.0f, 0.0f };
    float fog_density = 0.0f;
};

} // namespace compages::renderer
