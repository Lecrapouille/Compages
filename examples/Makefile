##=====================================================================
## OpenGLCppWrapper: A C++11 OpenGL 'Core' wrapper.
## Copyright 2018-2022 Quentin Quadrat <lecrapouille@gmail.com>
##
## This file is part of OpenGLCppWrapper.
##
## OpenGLCppWrapper is free software: you can redistribute it and/or modify it
## under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
##
## OpenGLCppWrapper is distributedin the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

###################################################
# Location of the project directory and Makefiles
#
P=..
M=$(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)-demos
TARGET_DESCRIPTION := Demos for $(PROJECT)
include $(M)/project/Makefile

###################################################
# List of files to compile.
#
SRC_FILES += $(call rwildcard,$(P)/examples/00_Window,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/examples/01_Core,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/examples/02_SceneGraph,*.cpp)
SRC_FILES += $(call rwildcard,$(P)/examples/03_ThreeJs,*.cpp)
SRC_FILES += $(P)/examples/debug.cpp
SRC_FILES += $(P)/examples/main.cpp

###################################################
# Set Libraries compiled in the external/ directory.
# For knowing which libraries is needed please read
# the doc/Install.md file.
#
INTERNAL_LIBS := $(call internal-lib,$(PROJECT_NAME))
INCLUDES += $(P)/include $(P)/src $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/bullet3/usr/local/include
INCLUDES += $(THIRD_PARTIES_DIR)/bullet3/usr/local/include/bullet
INCLUDES += $(THIRD_PARTIES_DIR)/SOIL/include
INCLUDES += $(THIRD_PARTIES_DIR)/units/include
INCLUDES += $(THIRD_PARTIES_DIR)/json/include
IMGUI_DIR := $(THIRD_PARTIES_DIR)/imgui
INCLUDES += $(IMGUI_DIR) $(IMGUI_DIR)/misc/cpp $(IMGUI_DIR)/backends

###################################################
# OpenGL: glfw and glew libraries
#
ifeq ($(OS),Darwin)
INCLUDES += /usr/local/include /opt/local/include
LINKER_FLAGS += -framework OpenGL -framework Cocoa
LINKER_FLAGS += -framework IOKit -framework CoreVideo
LINKER_FLAGS += -L/usr/local/lib -L/opt/local/lib
LINKER_FLAGS += -lGLEW -lglfw
else ifeq ($(OS),Linux)
PKG_LIBS += glew --static glfw3
else
$(error Unknown architecture)
endif

###############################################################################
# Sharable information between all Makefiles
#
include $(M)/rules/Makefile