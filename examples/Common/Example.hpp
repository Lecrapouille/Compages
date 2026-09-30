// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include "Compages/GPU/GPU.hpp"
#include "Compages/World/Controllers/ViewFrame.hpp"

#include <memory>
#include <string>

// ****************************************************************************
//! \file
//! \brief What every example is, seen from the gallery.
//!
//! Four things: a name, a sentence saying what it demonstrates, a way to set
//! itself up, and a way to draw one frame. A fifth, optional, puts a few
//! controls of its own into the "Try it" panel. Everything else, the window,
//! the loop, the timing, the overlay, belongs to the gallery.
//!
//! There is deliberately no tear down. An example holds its resources as
//! members, and destroying it releases them, which is the same rule as
//! everywhere else in the library. That is also what makes the gallery a leak
//! test: if closing an example does not bring the resource counters back to
//! where they were, the example is holding something it should not, and the
//! overlay shows it immediately.
// ****************************************************************************

namespace examples
{

//! \brief What the gallery tells an example about the frame it draws: the
//! size, the time and the input. \c total restarts when the example is chosen.

// ****************************************************************************
//! \brief One demonstration, of one idea.
//!
//! Each example demonstrates a point about the design rather than a function of
//! the graphics API, which is why the description is part of the interface: the
//! gallery shows it, and writing it is how the author of an example checks it
//! has a point.
// ****************************************************************************
class Example
{
public:

    virtual ~Example() = default;

    // ------------------------------------------------------------------------
    //! \brief What this example is called, as the gallery lists it.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual std::string name() const = 0;

    // ------------------------------------------------------------------------
    //! \brief What it demonstrates, in a sentence or two the gallery shows next
    //! to it.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual std::string description() const = 0;

    // ------------------------------------------------------------------------
    //! \brief Build everything this example needs on the device.
    //!
    //! Called once, on a live device, just after the example is chosen.
    //! Anything that can fail belongs here rather than in the constructor, so
    //! that the reason reaches the gallery and appears on the screen instead of
    //! being thrown.
    //!
    //! \return why the example cannot run. A frame error recorded during the
    //! set up (a misspelled attribute, say) counts as a failure too.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual compages::Status setUp() = 0;

    // ------------------------------------------------------------------------
    //! \brief Draw one frame.
    //!
    //! The gallery has already opened a pass over the whole window, so drawing
    //! to the screen needs no pass of its own: compages::gpu::clear() then
    //! draw. Whatever goes wrong is the frame error (see gpu/Errors.hpp), which
    //! the gallery reads after the frame: it stops the example and shows the
    //! reason rather than repeating the same failure sixty times a second.
    //!
    //! \param[in] p_frame the size of the window and how much time has passed.
    // ------------------------------------------------------------------------
    virtual void draw(compages::world::ViewFrame const& p_frame) = 0;

    // ------------------------------------------------------------------------
    //! \brief Draw a few Dear ImGui widgets of its own: a choice of animation,
    //! a slider. Called inside the "Try it" panel, only while the panels are
    //! shown, so that nothing the example needs to run may depend on it.
    // ------------------------------------------------------------------------
    virtual void controls()
    {
        /* do nothing */
    }

    // ------------------------------------------------------------------------
    //! \brief Which physical keys fill \c compages::world::ViewFrame::input. The
    //! gallery asks for this every frame; override to remap from Try it.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual compages::world::KeyMap keyMap() const
    {
        return compages::world::KeyMap::defaults();
    }

    // ------------------------------------------------------------------------
    //! \brief Should a click on the picture hide the pointer and turn every
    //! motion of the mouse into a look around, until Escape? What a first
    //! person game wants. Off by default.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual bool capturesMouse() const
    {
        return false;
    }

    // ------------------------------------------------------------------------
    //! \brief A few lines written over the picture, panels shown or not: the
    //! score of a game, a hint. A line starting with '!' is written large, in
    //! the middle. Empty, the default, writes nothing.
    // ------------------------------------------------------------------------
    [[nodiscard]] virtual std::string hud() const
    {
        return {};
    }
};

//! \brief How an example is handed around.
using ExamplePtr = std::unique_ptr<Example>;

} // namespace examples
