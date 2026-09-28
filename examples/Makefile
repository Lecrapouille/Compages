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
TARGET_NAME := $(PROJECT_NAME)-examples
TARGET_DESCRIPTION := Examples for $(PROJECT_NAME)
include $(M)/project/Makefile

###################################################
# Dear ImGui, with the GLFW and OpenGL 3 backends.
# Compiled here rather than into the library: nothing in src/ opens a window.
#
IMGUI_DIR := $(THIRD_PARTIES_DIR)/imgui
IMGUI_FILES := $(IMGUI_DIR)/imgui.cpp
IMGUI_FILES += $(IMGUI_DIR)/imgui_draw.cpp
IMGUI_FILES += $(IMGUI_DIR)/imgui_tables.cpp
IMGUI_FILES += $(IMGUI_DIR)/imgui_widgets.cpp
IMGUI_FILES += $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp
IMGUI_FILES += $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp
SRC_FILES += $(IMGUI_FILES)
INCLUDES += $(IMGUI_DIR) $(IMGUI_DIR)/backends

IMGUI_WARNINGS := -Wno-conversion -Wno-old-style-cast -Wno-sign-conversion \
  -Wno-float-equal -Wno-undef -Wno-shadow -Wno-double-promotion -Wno-cast-qual \
  -Wno-switch-default -Wno-useless-cast -Wno-zero-as-null-pointer-constant \
  -Wno-suggest-attribute=noreturn -Wno-format-nonliteral
IMGUI_OBJS := $(patsubst $(abspath $(P))/%,%,$(foreach f, \
  $(IMGUI_FILES) $(P)/examples/Common/Gallery.cpp $(P)/examples/Common/GalleryPanels.cpp,$(abspath $f)))
IMGUI_OBJS := $(addprefix $(OBJS_ROOT_DIR)/,$(addsuffix .o,$(basename $(IMGUI_OBJS))))
$(IMGUI_OBJS): USER_CXXFLAGS += $(IMGUI_WARNINGS)

###################################################
# Compile the standalone gallery
#
# examples/legacy is kept as a reference of the previous API and is not compiled.
#
SRC_FILES += $(filter-out $(P)/examples/legacy/%,$(call rwildcard,$(P)/examples,*.cpp))
INCLUDES += $(P)/examples $(P)/include $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/units/include
INCLUDES += $(THIRD_PARTIES_DIR)/stb
INCLUDES += $(THIRD_PARTIES_DIR)/cgltf
INCLUDES += $(THIRD_PARTIES_DIR)/entt/src
VPATH += $(P)/examples
INTERNAL_LIBS := $(call internal-lib,$(PROJECT_NAME))

ifeq ($(OS),Darwin)
LINKER_FLAGS += -framework OpenGL -framework Cocoa
LINKER_FLAGS += -framework IOKit -framework CoreVideo
LINKER_FLAGS += -L/usr/local/lib -L/opt/local/lib -lglfw
else ifeq ($(OS),Linux)
PKG_LIBS += glfw3
LINKER_FLAGS += -ldl
else
$(error Unknown architecture)
endif

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

# Linked again whenever the library changes, not only when an example does.
$(TARGET_BINARY_NAME): $(INTERNAL_LIBS)
