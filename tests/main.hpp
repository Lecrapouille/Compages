// SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
// Copyright (c) 2018-2026 Quentin Quadrat
//
// This file is part of Compages. It is available under the GNU GPL v3 or,
// for users who cannot use the GPL, under a commercial license.
// See LICENSING.md for details.

#pragma once

#include <cstddef>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

// size_t is 32 or 64 bits depending on the architecture; this keeps
// literals like 4_z typed as size_t in the Math tests.
constexpr std::size_t operator""_z(unsigned long long const n)
{
    return static_cast<std::size_t>(n);
}
