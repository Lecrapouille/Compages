# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. See LICENSING.md for details.
#
# The library, for MyMakefile 3. A project embedding Compages reads it with
# its third parties:
#   TARGETS += CompagesProject
#   CompagesProject.GIT := https://github.com/Lecrapouille/Compages.git
#   CompagesProject.MAKEFILES := Compages.mk external/external.mk
#   myapp.DEPENDS := Compages

#? GPU backend compiled in the library:
#?   GL45  OpenGL 4.5 core, everything (Linux, Windows). Default.
#?   GL41  OpenGL 4.1 core, no compute (macOS). Default on macOS.
#?   GLES3 OpenGL ES 3.0, no compute (WebGL 2 with Emscripten, or a native
#?         ES context). Default with TOOLCHAIN=emscripten.
ifeq ($(TARGET_OS),Emscripten)
    GPU_BACKEND ?= GLES3
else ifeq ($(TARGET_OS),Darwin)
    GPU_BACKEND ?= GL41
else
    GPU_BACKEND ?= GL45
endif

# GL41 and GLES3 are the same code (src/GPU/Backends/GL41), GLES3 compiled
# with COMPAGES_GLES3. Only the desktop backends load their symbols with glad.
ifeq ($(GPU_BACKEND),GLES3)
    GPU_BACKEND_DIR := GL41
    GPU_BACKEND_GLAD :=
else
    GPU_BACKEND_DIR := $(GPU_BACKEND)
    GPU_BACKEND_GLAD := src/GPU/Backends/glad/GladUnit.cpp
endif

TARGETS += Compages
Compages.TYPE := library
Compages.DESCRIPTION := C++ Wrapper allowing to write OpenGL Core applications in few lines
Compages.SOURCES := src/GPU/*.cpp \
    src/GPU/Internal/**/*.cpp src/Renderer/**/*.cpp src/World/**/*.cpp \
    src/GPU/Backends/$(GPU_BACKEND_DIR)/*.cpp $(GPU_BACKEND_GLAD)
Compages.PUBLIC_INCLUDES := include
Compages.INCLUDES := src src/GPU/Backends/glad/include
Compages.DEFINES := $(if $(filter GLES3,$(GPU_BACKEND)),COMPAGES_GLES3)
# Read by the window code of an application, which asks for the context the
# backend needs (examples/Common/Window.cpp).
Compages.PUBLIC_DEFINES := COMPAGES_GPU_BACKEND_$(GPU_BACKEND)
Compages.DEPENDS := units entt stb cgltf json pugixml
ifeq ($(GPU_BACKEND),GLES3)
ifneq ($(TARGET_OS),Emscripten)
    # A native ES context (Mesa, ...): the symbols come from libGLESv2.
    Compages.LIBS := -lGLESv2
endif
endif
