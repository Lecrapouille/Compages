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
## OpenGLCppWrapper is distributed in the hope that it will be useful, but
## WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
## General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with OpenGLCppWrapper.  If not, see <http://www.gnu.org/licenses/>.
##=====================================================================

P := .
M := $(P)/.makefile
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := C++ Wrapper allowing to write OpenGL Core applications in few lines
include $(M)/project/Makefile

###################################################
# SOIL (image loader for OpenGL texturing)
#
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/SOIL/build/libSOIL.a)
INCLUDES += $(THIRD_PARTIES_DIR)/SOIL/include
VPATH += $(THIRD_PARTIES_DIR)/SOIL/src

###################################################
# Dear IM Gui (immediate mode rendering GUI)
#
IMGUI_DIR := $(THIRD_PARTIES_DIR)/imgui
INCLUDES += $(IMGUI_DIR) $(IMGUI_DIR)/misc/cpp $(IMGUI_DIR)/backends
VPATH += $(IMGUI_DIR) $(IMGUI_DIR)/misc/cpp
LIB_FILES += $(IMGUI_DIR)/imgui_widgets.cpp $(IMGUI_DIR)/imgui_draw.cpp
LIB_FILES += $(IMGUI_DIR)/imgui_tables.cpp $(IMGUI_DIR)/imgui.cpp
LIB_FILES += $(IMGUI_DIR)/misc/cpp/imgui_stdlib.cpp $(IMGUI_DIR)/backends/imgui_impl_glfw.cpp
LIB_FILES += $(IMGUI_DIR)/backends/imgui_impl_opengl3.cpp

###################################################
# Bullet3 (3d physics)
#
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/bullet3/usr/local/lib/libBulletSoftBody.a)
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/bullet3/usr/local/lib/libBulletDynamics.a)
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/bullet3/usr/local/lib/libBulletCollision.a)
THIRD_PARTIES_LIBS += $(abspath $(THIRD_PARTIES_DIR)/bullet3/usr/local/lib/libLinearMath.a)
INCLUDES += $(THIRD_PARTIES_DIR)/bullet3/usr/local/include
INCLUDES += $(THIRD_PARTIES_DIR)/bullet3/usr/local/include/bullet

###################################################
# Units library
#
INCLUDES += $(THIRD_PARTIES_DIR)/units/include

###################################################
# JSON parser
#
INCLUDES += $(THIRD_PARTIES_DIR)/json/include

###################################################
# OpenGLCppWrapper defines.
#
# CHECK_OPENGL allows to check if bad parameters are
#   passed to OpenGL routines. Produce an error message
#   in the console but does not abort the program.
# ENABLE_DEBUG activate logs on the console.

DEFINES += -DCHECK_OPENGL -UENABLE_DEBUG

###################################################
# Inform Makefile where to find header files for third parts
#
INCLUDES += $(P)/include $(P)/src $(THIRD_PARTIES_DIR)

###################################################
# Inform Makefile where to find *.cpp and *.o files
#
VPATH += $(P)/src $(THIRD_PARTIES_DIR)

###################################################
# Compiled objects files
#
LIB_FILES += $(call rwildcard,$(P)/src/Common,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/Components,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/Loaders,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/Math,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/OpenGL,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/Scene,*.cpp)
LIB_FILES += $(call rwildcard,$(P)/src/UI,*.cpp)

include $(M)/rules/Makefile
