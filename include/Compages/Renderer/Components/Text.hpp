// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Text written on the picture: a Label next to an entity (what finds
//! a planet smaller than a pixel, or names the parts of a robot), a Tooltip
//! (a framed box of lines pointing at its entity, like the tooltip of a web
//! page), a Caption at a fixed place of the picture of a camera (speeds,
//! altitudes, a score).
//!
//! Components like the helpers: set on an entity, drawn by the Scene at each
//! render, after everything else. The letters keep
//! their size in pixels whatever the distance; a dot marks the point and the
//! text starts on its right. They are drawn from a distance field (the
//! method of Glumpy): sharp at any size, with a dark halo that keeps them
//! readable on a bright picture. A label that would cover one already written
//! on the picture is not written, and a label is not hidden by the things in
//! front of its point; nor is it written over a Caption or a Tooltip. All
//! need Scene::font().
//! \code
//! scene.font("fonts/DejaVuSans.ttf");
//! mars.set(compages::renderer::Label{ .text = "Mars",
//!                                     .color = { 1.0f, 0.95f, 0.85f },
//!                                     .pixels = 16.0f,
//!                                     .hide_closer_than = 0.0f });
//! island.set(compages::renderer::Tooltip{ .title = "Sciences",
//!                                         .text = "fr.wikipedia.org/wiki/Physique\n...",
//!                                         .color = { 0.92f, 0.95f, 1.0f },
//!                                         .accent = { 1.0f, 0.8f, 0.45f },
//!                                         .background = { 0.04f, 0.08f, 0.14f },
//!                                         .opacity = 0.85f,
//!                                         .pixels = 14.0f });
//! camera.set(compages::renderer::Caption{ .text = "Altitude 0 km",
//!                                         .color = { 1.0f, 0.96f, 0.88f },
//!                                         .pixels = 16.0f,
//!                                         .at = { 12.0f, 10.0f } });
//! \endcode
// ****************************************************************************

#include "Compages/Core/Vector.hpp"

#include <string>

namespace compages::renderer
{

//! \brief A name written next to its entity, on the picture.
struct Label
{
    //! \brief What is written, in UTF-8. The letters of Latin-1 (French
    //! accents included); any other is written as '?'.
    std::string text;
    //! \brief Colour of the letters and of the dot (sRGB).
    compages::Vector3f color;
    //! \brief Height of the letters on the picture, in pixels.
    float pixels;
    //! \brief Not written when the camera is closer than this, in metres:
    //! the thing is then large enough to be seen without its name. Zero:
    //! always written.
    float hide_closer_than;
};

//! \brief A box of text pointing at its entity: a title, lines under it, on
//! a frame with round corners, joined to the point by a stem. Always faces
//! the camera and keeps its size in pixels. It goes above the point, or
//! below when there is no room above, and stays inside the picture. Laid out
//! before the labels, so that no label covers it.
struct Tooltip
{
    //! \brief The first line, a quarter taller than the others (UTF-8).
    std::string title;
    //! \brief The lines under the title, in UTF-8; '\n' starts a new line.
    //! Empty: the title alone.
    std::string text;
    //! \brief Colour of the lines (sRGB).
    compages::Vector3f color;
    //! \brief Colour of the title, of the line under it, of the stem and of
    //! the dot on the point (sRGB).
    compages::Vector3f accent;
    //! \brief Colour of the frame (sRGB).
    compages::Vector3f background;
    //! \brief Opacity of the frame, 1 solid: the picture shows through.
    float opacity;
    //! \brief Height of the lines on the picture, in pixels.
    float pixels;
};

//! \brief Text at a fixed place of the picture of its camera: set on the
//! camera entity, drawn only in what that camera sees (each view of a split
//! screen has its own). Change \c text each frame for values that move.
struct Caption
{
    //! \brief What is written, in UTF-8; '\n' starts a new line.
    std::string text;
    //! \brief Colour of the letters (sRGB).
    compages::Vector3f color;
    //! \brief Height of the letters on the picture, in pixels.
    float pixels;
    //! \brief Top left corner of the text, in pixels from the top left
    //! corner of the picture.
    compages::Vector2f at;
};

} // namespace compages::renderer
