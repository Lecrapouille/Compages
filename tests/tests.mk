# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. See LICENSING.md for details.
#
# Unit tests, with Google Test. Paths are relative to this directory.

TARGETS += Compages-UnitTest
Compages-UnitTest.TYPE := test
Compages-UnitTest.DESCRIPTION := Unit tests for Compages
Compages-UnitTest.SOURCES := **/*.cpp
Compages-UnitTest.INCLUDES := . ../src ../src/GPU/Backends/$(GPU_BACKEND)/glad/include
Compages-UnitTest.DEFINES := GPU_BACKEND_$(GPU_BACKEND)
Compages-UnitTest.CXXFLAGS := -Wno-noexcept -Wno-undef -Wno-float-equal
Compages-UnitTest.DEPENDS := Compages
Compages-UnitTest.PKG_CONFIG := gtest gmock glfw3
