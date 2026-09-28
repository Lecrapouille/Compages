##=====================================================================
## Compages: A C++20 OpenGL wrapper.
## Copyright 2018-2026 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of Compages.
##
## Compages is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## Compages is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with Compages.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

###################################################
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
