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

// ****************************************************************************
//! \file
//! \brief Compiles the vendored glad loader as part of the library.
//!
//! glad generates src/gl.c, which is left exactly as generated so that it can be
//! regenerated without losing local edits. It is included here instead of being
//! compiled on its own for two reasons.
//!
//! Building it as C++ rather than C means the project uses one compiler and one
//! set of flags; the C compiler was otherwise being handed C++ only warning
//! options and complaining about each of them.
//!
//! And a generated file cannot be expected to satisfy the warning level the
//! project holds its own code to. glad casts every loaded symbol to its precise
//! function pointer type, which for a handful of them is the type it already
//! has, so the project's -Wuseless-cast fires on generated code nobody is going
//! to rewrite. Silencing it here rather than project wide keeps the warning
//! useful everywhere else.
// ****************************************************************************

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuseless-cast"
#include "./src/gl.c"
#pragma GCC diagnostic pop
