# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. See LICENSING.md for details.
#
# The libraries, for MyMakefile 3. A project embedding Compages reads it with
# its third parties:
#   TARGETS += CompagesProject
#   CompagesProject.GIT := https://github.com/Lecrapouille/Compages.git
#   CompagesProject.MAKEFILES := Compages.mk external/external.mk
#   myapp.DEPENDS := Compages
#
# One library per layer, so that the linker checks what the includes say:
#   Compages-World  Core, Fields and World: no GPU, no OpenGL. For a
#                   simulation, a server or tests with no screen.
#   Compages-GPU    Core, Fields and the GPU layer, with its backend.
#   Compages        the Renderer. It depends on the two others, so that
#                   depending on Compages gives the three layers.
# A program that depends on Compages-World alone fails to link as soon as
# it calls the GPU or the Renderer.

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

### Core, Fields and World: entities, scene graph, joints, routines, URDF.
TARGETS += Compages-World
Compages-World.TYPE := library
Compages-World.DESCRIPTION := Compages World layer: scene graph and ECS, with no GPU
Compages-World.SOURCES := src/World/**/*.cpp
Compages-World.PUBLIC_INCLUDES := include
Compages-World.INCLUDES := src
Compages-World.DEPENDS := units entt pugixml

### Core, Fields and the GPU layer: shaders, buffers, textures, compute.
TARGETS += Compages-GPU
Compages-GPU.TYPE := library
Compages-GPU.DESCRIPTION := Compages GPU layer: shader-named data, drawables and compute
Compages-GPU.SOURCES := src/GPU/*.cpp src/GPU/Internal/**/*.cpp \
    src/GPU/Backends/$(GPU_BACKEND_DIR)/*.cpp $(GPU_BACKEND_GLAD)
Compages-GPU.PUBLIC_INCLUDES := include
Compages-GPU.INCLUDES := src src/GPU/Backends/glad/include
Compages-GPU.DEFINES := $(if $(filter GLES3,$(GPU_BACKEND)),COMPAGES_GLES3)
# Read by the window code of an application, which asks for the context the
# backend needs (examples/Common/Window.cpp).
Compages-GPU.PUBLIC_DEFINES := COMPAGES_GPU_BACKEND_$(GPU_BACKEND)
Compages-GPU.DEPENDS := units stb
ifeq ($(GPU_BACKEND),GLES3)
ifneq ($(TARGET_OS),Emscripten)
    # A native ES context (Mesa, ...): the symbols come from libGLESv2.
    Compages-GPU.LIBS := -lGLESv2
endif
endif

### The Renderer: Scene, assets, snapshot and drawing, over the two others.
TARGETS += Compages
Compages.TYPE := library
Compages.DESCRIPTION := C++ Wrapper allowing to write OpenGL Core applications in few lines
Compages.SOURCES := src/Renderer/**/*.cpp
Compages.PUBLIC_INCLUDES := include
Compages.INCLUDES := src
Compages.DEPENDS := Compages-World Compages-GPU stb cgltf json
