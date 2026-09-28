# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

# Location of the project directory and Makefiles
#
P := ..
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)-UnitTest
TARGET_DESCRIPTION := Unit tests for $(PROJECT_NAME)
COMPILATION_MODE := test
include $(M)/project/Makefile

###################################################
# Compile the unit tests
#
# Library sources are compiled here rather than linked from the static lib:
# the coverage report needs them built with the test compilation mode.
# Exactly one GPU backend is compiled, the same as the library.
#
SRC_FILES := $(filter-out $(P)/src/GPU/Backends/%,$(call rwildcard,$(P)/src,*.cpp)) \
    $(call rwildcard,$(P)/src/GPU/Backends/$(GPU_BACKEND),*.cpp) \
    $(call rwildcard,$(P)/tests,*.cpp)
INCLUDES := $(P)/tests $(P)/include $(P)/src $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/units/include
INCLUDES += $(THIRD_PARTIES_DIR)/stb
INCLUDES += $(THIRD_PARTIES_DIR)/cgltf
INCLUDES += $(THIRD_PARTIES_DIR)/json/include
INCLUDES += $(THIRD_PARTIES_DIR)/entt/src
INCLUDES += $(THIRD_PARTIES_DIR)/pugixml/src
INCLUDES += $(P)/src/GPU/Backends/$(GPU_BACKEND)/glad/include
VPATH := $(P)/tests $(P)/src
DEFINES += -DGPU_BACKEND_$(GPU_BACKEND)
USER_CXXFLAGS += -Wno-noexcept -Wno-undef -Wno-float-equal
PKG_LIBS += gtest gmock

ifeq ($(OS),Darwin)
LINKER_FLAGS += -framework OpenGL -framework Cocoa
LINKER_FLAGS += -framework IOKit -framework CoreVideo
LINKER_FLAGS += -L/usr/local/lib -L/opt/local/lib -lglfw
else
PKG_LIBS += glfw3
endif

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile
