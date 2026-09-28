// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

// ****************************************************************************
//! \file
//! \brief Private header of the OpenGL 4.5 backend.
//!
//! This is the only place in the project where glad, and therefore any gl*
//! symbol, may be included. Nothing outside src/GPU/Backends/GL45 includes it:
//! that rule is what makes the promise of a public API without OpenGL
//! enforceable rather than merely intended.
// ****************************************************************************

#include <glad/gl.h>

#include "GPU/Backends/Backend.hpp"

namespace compages::gpu::backend
{

//! \brief The lowest OpenGL version this backend can work with. 4.5 is where
//! Direct State Access became core, and the whole backend is written with it.
constexpr int MINIMUM_VERSION_MAJOR = 4;
constexpr int MINIMUM_VERSION_MINOR = 5;

namespace detail
{

// ----------------------------------------------------------------------------
//! \brief Drop every vertex array object the backend is holding.
//!
//! The one piece of state the backend keeps of its own accord rather than on
//! behalf of a handle: a cache of ways of reading a vertex, shared between
//! pipelines. Emptied by shutdown(), after the pools have already let go of their
//! shares, so anything left here was held by nobody.
// ----------------------------------------------------------------------------
void clearVertexArrayCache();

} // namespace detail

} // namespace compages::gpu::backend
