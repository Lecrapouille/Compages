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
