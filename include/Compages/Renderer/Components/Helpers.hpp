// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Lines that help to read a scene: a grid on the ground, the axes of
//! an entity, the box around a mesh.
//!
//! Each is a component of an entity, drawn as lines by the Scene at each
//! render, where the entity is. Made by Scene::grid(), Scene::axes() and
//! Scene::showBounds(); removed like any component.
// ****************************************************************************

#include "Compages/Core/Vector.hpp"

#include <cstdint>

namespace compages::renderer
{

//! \brief A square grid in the XZ plane of its entity, centred on it.
struct Grid
{
    //! \brief Width of the whole grid.
    float size = 10.0f;
    //! \brief Cells along one side.
    std::uint32_t divisions = 10u;
    //! \brief Colour of the lines; the two centre lines are brighter.
    compages::Vector3f color{ 0.35f, 0.35f, 0.38f };
};

//! \brief The three axes of its entity: X red, Y green, Z blue. Hung from
//! another entity, it shows how that one is placed and turned (a gizmo).
struct Axes
{
    //! \brief Length of each axis.
    float length = 1.0f;
};

//! \brief Draw the box around the mesh of this entity, turning with it.
struct ShowBounds
{
    compages::Vector3f color{ 0.2f, 1.0f, 0.4f };
};

} // namespace compages::renderer
