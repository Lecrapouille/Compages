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
//! \brief The colours of a value, from the low end of a range to the high end.
// ****************************************************************************
enum class ColorMap
{
    //! \brief Dark blue, green, yellow: every step looks as large as the
    //! others, and it stays readable in grey and for colour-blind eyes.
    Viridis,
    //! \brief Black, red, yellow, white: heat.
    Inferno,
    //! \brief Blue, light grey, red: below and above a middle value.
    Coolwarm,
    //! \brief Deep sea, shallows, sand, grass, rock, snow: a relief.
    Terrain,
    //! \brief Black to white.
    Grey,
};

// ****************************************************************************
//! \brief How Scene::show() draws a Field<float>: each value as a colour, and
//! maybe as a height.
//!
//! \code
//! scene.show(temperature, { .colors = compages::renderer::ColorMap::Inferno,
//!                           .low = 20.0f, .high = 100.0f,
//!                           .relief = 0.0f, .lit = false }, "Plate");
//! \endcode
// ****************************************************************************
struct FieldLook
{
    //! \brief The colours from low to high.
    ColorMap colors;
    //! \brief The value drawn with the first colour, and the one drawn with the
    //! last; values outside take the colour of the end. Must differ.
    float low;
    float high;
    //! \brief Metres of height per unit of value: 0 draws a flat picture on
    //! the ground, more raises each point by its value.
    float relief;
    //! \brief True: lit by the sun and the lamps, like a ground. False: the
    //! colours exactly as the map gives them, like a chart.
    bool lit;
};

// ****************************************************************************
//! \brief What Scene::probe() finds under a pixel: where the ray meets the
//! field as it is drawn, and the value there.
// ****************************************************************************
struct FieldProbe
{
    //! \brief The point of the drawn field, in the world.
    compages::Vector3f point;
    //! \brief The value of the field there (bilinear).
    float value;
};

} // namespace compages::renderer
