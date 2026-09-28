# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

# Location of the project directory and Makefiles
#
P := .
M := $(P)/.makefile

###################################################
# Project definition
#
include $(P)/Makefile.common
TARGET_NAME := $(PROJECT_NAME)
TARGET_DESCRIPTION := C++ Wrapper allowing to write OpenGL Core applications in few lines
include $(M)/project/Makefile

###################################################
# Compile shared and static libraries
#
# Exactly one GPU backend is compiled. Selecting another one is a matter of
# changing GPU_BACKEND in Makefile.common.
#
LIB_FILES := $(filter-out src/GPU/Backends/%,$(call rwildcard,src,*.cpp))
LIB_FILES += $(call rwildcard,src/GPU/Backends/$(GPU_BACKEND),*.cpp)
DEFINES += -DGPU_BACKEND_$(GPU_BACKEND)
INCLUDES := $(P)/include $(P)/src $(THIRD_PARTIES_DIR)
INCLUDES += $(THIRD_PARTIES_DIR)/units/include
INCLUDES += $(THIRD_PARTIES_DIR)/stb
INCLUDES += $(THIRD_PARTIES_DIR)/cgltf
INCLUDES += $(THIRD_PARTIES_DIR)/json/include
INCLUDES += $(THIRD_PARTIES_DIR)/entt/src
INCLUDES += $(THIRD_PARTIES_DIR)/pugixml/src
INCLUDES += $(P)/src/GPU/Backends/$(GPU_BACKEND)/glad/include
VPATH := $(P)/src

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile

###################################################
# Extra rules
#
post-build:: build-examples

install::
	$(Q)install -d -m 755 $(INSTALL_LIBDIR)
	$(Q)install -d -m 755 $(INSTALL_INCLUDEDIR)/entt
	$(Q)cp -R $(THIRD_PARTIES_DIR)/entt/src/entt/. $(INSTALL_INCLUDEDIR)/entt/
	$(Q)install -m 644 $(THIRD_PARTIES_DIR)/units/include/units.h $(INSTALL_INCLUDEDIR)/

.PHONY: build-examples
build-examples: $(TARGET_STATIC_LIB_NAME)
	$(Q)$(MAKE) --no-print-directory --directory=examples all
