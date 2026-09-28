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

#include "Compages/GPU/Core/Handle.hpp"

#include <cstddef>
#include <string>
#include <utility>

// ****************************************************************************
//! \file
//! \brief The two ways the gpu layer says something went wrong.
//!
//! Errors come in two kinds, and each has its own channel.
//!
//! - **What can fail for a reason outside the program**: compiling a shader,
//!   reading a file, asking the driver for something it does not have. These
//!   return a Status, checked once with COMPAGES_TRY, because the caller has to
//!   decide what to do about them.
//!
//! - **What happens every frame**: setting a uniform, binding a texture,
//!   drawing. A failure there is a programming mistake (a uniform misspelled,
//!   a buffer too short), not something to recover from in the middle of a
//!   frame. These functions return nothing; they record the first failure of
//!   the frame here, and the window shows it and stops the example.
//!
//! \code
//! compages::gpu::Status Demo::setUp()
//! {
//!     COMPAGES_TRY(m_quad.load(VERTEX, FRAGMENT));  // may fail: a Status
//!     return compages::gpu::success();
//! }
//!
//! void Demo::draw(Frame const& p_frame)
//! {
//!     m_quad["time"] = p_frame.total;               // cannot fail loudly
//!     m_quad.draw();                                // recorded, if it fails
//! }
//!
//! // Once per frame, by whoever runs the loop:
//! if (compages::gpu::hasFrameError())
//! {
//!     std::cerr << compages::gpu::takeFrameError() << std::endl;
//! }
//! \endcode
// ****************************************************************************

namespace compages::gpu
{

// ----------------------------------------------------------------------------
//! \brief Record that something failed during this frame.
//!
//! Only the first message is kept, because it is the cause and those after it
//! are usually its consequences. The others are counted.
//!
//! \param[in] p_message what went wrong, in a sentence a user can act on.
// ----------------------------------------------------------------------------
void reportError(std::string p_message);

// ----------------------------------------------------------------------------
//! \brief Record a failed Status, and say whether it succeeded.
//!
//! The bridge from the first channel to the second: code that must keep
//! going, such as a draw loop, turns a Status into a recorded error.
//!
//! \code
//! if (!compages::gpu::check(m_texture.load("missing.png")))
//! {
//!     return;   // the reason is waiting in compages::gpu::takeFrameError()
//! }
//! \endcode
//!
//! \return true when \c p_status was a success.
// ----------------------------------------------------------------------------
bool check(Status const& p_status);

// ----------------------------------------------------------------------------
//! \brief Has anything failed since the error was last taken?
// ----------------------------------------------------------------------------
[[nodiscard]] bool hasFrameError();

// ----------------------------------------------------------------------------
//! \brief How many failures were recorded since the error was last taken.
//! Only the first one's message is kept.
// ----------------------------------------------------------------------------
[[nodiscard]] std::size_t frameErrorCount();

// ----------------------------------------------------------------------------
//! \brief Return the first recorded message and forget everything recorded.
//!
//! \return an empty string when nothing failed.
// ----------------------------------------------------------------------------
[[nodiscard]] std::string takeFrameError();

// ----------------------------------------------------------------------------
//! \brief Stop in the debugger on the first recorded failure.
//!
//! Off by default. Turned on, reportError() asserts, which puts the debugger
//! on the very line that failed instead of at the end of the frame. Has no
//! effect when assertions are compiled out.
// ----------------------------------------------------------------------------
void setBreakOnError(bool p_enabled);

// ----------------------------------------------------------------------------
//! \brief Is setBreakOnError() on?
// ----------------------------------------------------------------------------
[[nodiscard]] bool breakOnError();

// ----------------------------------------------------------------------------
//! \brief Run something that records its failures, and return the first one
//! as a Status.
//!
//! The bridge in the other direction: code that wants a Status from a function
//! returning nothing, typically a test or a setup step drawing once.
//! Whatever was recorded before is kept aside and put back afterwards, so
//! calling this in the middle of a frame does not swallow an earlier error.
//! setBreakOnError() is suspended meanwhile, since a failure here is expected
//! to be looked at rather than stopped on.
//!
//! \code
//! compages::gpu::Status drawn = compages::gpu::attempt([&] { quad.draw(); });
//! EXPECT_FALSE(drawn);
//! EXPECT_THAT(drawn.error(), HasSubstr("no pass is open"));
//! \endcode
// ----------------------------------------------------------------------------
template <typename F>
[[nodiscard]] Status attempt(F&& p_function)
{
    const bool breaking = breakOnError();
    const std::size_t before_count = frameErrorCount();
    std::string before = takeFrameError();
    setBreakOnError(false);
    std::forward<F>(p_function)();
    setBreakOnError(breaking);
    const bool failed = hasFrameError();
    std::string message = takeFrameError();
    if (before_count > 0u)
    {
        reportError(std::move(before));
    }
    if (failed)
    {
        return failure(std::move(message));
    }
    return success();
}

} // namespace compages::gpu
