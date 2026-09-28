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
//=============================================================================

// ****************************************************************************
//! \file
//! \brief The one place stb_image is compiled.
//!
//! stb_image is a single header holding both its declarations and its
//! definitions, so exactly one translation unit must ask for the definitions.
//! This is it.
//!
//! It replaces SOIL, which this project used to carry: a library last released
//! in 2008, needing a separate build step, and reading fewer formats than this
//! single file does.
//!
//! The warnings are turned off around the include rather than fixed. This is
//! somebody else's code, written to be portable C89, and it is not ours to
//! change: the alternative is either patching it, which makes updating it
//! painful, or lowering the warning level of the whole project, which would cost
//! far more than it saves.
// ****************************************************************************

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wuseless-cast"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#pragma GCC diagnostic ignored "-Wswitch-enum"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wmissing-declarations"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wundef"
#pragma GCC diagnostic ignored "-Wcast-align"
#pragma GCC diagnostic ignored "-Wduplicated-branches"
#pragma GCC diagnostic ignored "-Warith-conversion"
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

//! \brief Ask stb_image for a sentence rather than a code when a file will not
//! load, since that sentence goes straight into the error a caller reads.
#define STBI_FAILURE_USERMSG

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#pragma GCC diagnostic pop
