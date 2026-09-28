// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstddef>
#include <string>

// ****************************************************************************
//! \file
//! \brief What the library is doing and what it is holding.
//!
//! Two kinds of number, and the difference between them is the point.
//!
//! The counts of work are reset every frame and say how much a frame cost: how
//! many draw calls, how many vertices. They are what to look at when a frame is
//! slow.
//!
//! The counts of resources are not reset, because they are not about a frame at
//! all. They say how many handles are alive and how much device memory they add
//! up to, and they should come back to where they started when a demo is closed.
//! An example that leaves them higher than it found them leaked, and the examples
//! gallery shows them continuously for exactly that reason: switching through
//! seventeen demos in a loop without the numbers drifting is the demonstration that
//! handle lifetimes are right.
// ****************************************************************************

namespace compages::gpu
{

// ****************************************************************************
//! \brief How much work a frame asked of the device.
// ****************************************************************************
struct FrameStatistics
{
    //! \brief How many times something was drawn. The number a frame budget is
    //! usually spent on: each one costs the driver work whatever it draws, which
    //! is why batching and instancing exist.
    std::size_t draw_calls = 0u;
    //! \brief How many vertices were asked for, indices counted once each.
    std::size_t vertices = 0u;
    //! \brief How many objects were drawn, counting an instanced draw as the
    //! number of instances it asked for.
    std::size_t instances = 0u;
    //! \brief How many times a pass was started.
    std::size_t passes = 0u;
    //! \brief How many compute dispatches were asked for.
    std::size_t dispatches = 0u;
};

// ****************************************************************************
//! \brief What the device is holding on our behalf.
// ****************************************************************************
struct ResourceStatistics
{
    std::size_t buffers = 0u;
    std::size_t textures = 0u;
    std::size_t shaders = 0u;
    std::size_t programs = 0u;
    std::size_t pipelines = 0u;
    std::size_t framebuffers = 0u;
    //! \brief How many distinct ways of reading a vertex are held. Less than the
    //! number of pipelines whenever several of them read a vertex alike.
    std::size_t vertex_readers = 0u;

    //! \brief How many bytes of buffers, as they were asked for.
    std::size_t buffer_bytes = 0u;
    //! \brief How many bytes of textures, every level and face counted. An
    //! estimate: what the driver really set aside may be more, since it is free to
    //! pad and to compress.
    std::size_t texture_bytes = 0u;

    // ------------------------------------------------------------------------
    //! \brief Is the device holding nothing at all? What a closed example should
    //! leave behind.
    // ------------------------------------------------------------------------
    [[nodiscard]] bool empty() const
    {
        return (buffers == 0u) && (textures == 0u) && (shaders == 0u) &&
               (programs == 0u) && (pipelines == 0u) && (framebuffers == 0u) &&
               (vertex_readers == 0u);
    }

    // ------------------------------------------------------------------------
    //! \brief A readable summary, for the overlay of the examples.
    // ------------------------------------------------------------------------
    [[nodiscard]] std::string toString() const;
};

// ----------------------------------------------------------------------------
//! \brief How much work has been asked of the device since the last reset.
// ----------------------------------------------------------------------------
[[nodiscard]] FrameStatistics const& frameStatistics();

// ----------------------------------------------------------------------------
//! \brief Start counting the work of a new frame.
//!
//! Called once per frame by whoever owns the loop.
// ----------------------------------------------------------------------------
void resetFrameStatistics();

// ----------------------------------------------------------------------------
//! \brief What the device is holding right now, counted by asking the pools.
// ----------------------------------------------------------------------------
[[nodiscard]] ResourceStatistics resourceStatistics();

} // namespace compages::gpu
