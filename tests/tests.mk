# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. See LICENSING.md for details.
#
# Unit tests, with Google Test. Paths are relative to this directory.

# Core, Fields and World, linked with Compages-World only: a call to the GPU
# or to the Renderer from these layers fails to link here.
TARGETS += Compages-WorldTest
Compages-WorldTest.TYPE := test
Compages-WorldTest.DESCRIPTION := Unit tests of Compages that need no GPU
Compages-WorldTest.SOURCES := main.cpp Core/*.cpp Fields/*.cpp World/*.cpp
Compages-WorldTest.INCLUDES := . ../src
Compages-WorldTest.CXXFLAGS := -Wno-noexcept -Wno-undef -Wno-float-equal
Compages-WorldTest.DEPENDS := Compages-World
Compages-WorldTest.PKG_CONFIG := gtest gmock

# The GPU and the Renderer, with an OpenGL context (a hidden GLFW window).
TARGETS += Compages-UnitTest
Compages-UnitTest.TYPE := test
Compages-UnitTest.DESCRIPTION := Unit tests of the GPU and Renderer layers of Compages
Compages-UnitTest.SOURCES := main.cpp GPU/*.cpp Renderer/*.cpp
Compages-UnitTest.INCLUDES := . ../src
Compages-UnitTest.CXXFLAGS := -Wno-noexcept -Wno-undef -Wno-float-equal
Compages-UnitTest.DEPENDS := Compages
Compages-UnitTest.PKG_CONFIG := gtest gmock glfw3
