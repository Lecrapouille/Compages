# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. See LICENSING.md for details.
#
# The gallery of examples. Paths are relative to this directory.

TARGETS += Compages-examples
Compages-examples.DESCRIPTION := Examples for Compages
Compages-examples.SOURCES := **/*.cpp
Compages-examples.INCLUDES := .
Compages-examples.DEPENDS := Compages imgui
Compages-examples.PKG_CONFIG := glfw3
# Where examples/Common/DataPath.hpp looks for the textures and models.
Compages-examples.DEFINES = COMPAGES_DATA_DIR="$(Compages-data.DIR)"
ifeq ($(TARGET_OS),Linux)
    Compages-examples.LIBS := -ldl
endif

# Third parties of the gallery.
TARGETS += imgui Compages-data

# Dear ImGui, with the GLFW and OpenGL 3 backends, for the gallery.
imgui.GIT := https://github.com/ocornut/imgui.git
imgui.VERSION := docking
imgui.TYPE := static
imgui.SOURCES := imgui.cpp imgui_draw.cpp imgui_tables.cpp imgui_widgets.cpp \
    backends/imgui_impl_glfw.cpp backends/imgui_impl_opengl3.cpp
imgui.PUBLIC_INCLUDES := . backends
imgui.PKG_CONFIG := glfw3

# Textures and models of the examples.
Compages-data.GIT := https://github.com/Lecrapouille/Compages-data.git
