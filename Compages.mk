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

#? GPU backend compiled in the library: the folder of src/GPU/Backends.
GPU_BACKEND ?= GL45

TARGETS += Compages
Compages.TYPE := library
Compages.DESCRIPTION := C++ Wrapper allowing to write OpenGL Core applications in few lines
Compages.SOURCES := src/GPU/*.cpp src/GPU/Core/**/*.cpp \
    src/GPU/Internal/**/*.cpp src/Renderer/**/*.cpp src/World/**/*.cpp \
    src/GPU/Backends/$(GPU_BACKEND)/**/*.cpp
Compages.PUBLIC_INCLUDES := include
Compages.INCLUDES := src src/GPU/Backends/$(GPU_BACKEND)/glad/include
Compages.DEFINES := GPU_BACKEND_$(GPU_BACKEND)
Compages.DEPENDS := units entt stb cgltf json pugixml
