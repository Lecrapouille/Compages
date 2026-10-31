# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. It is available under the GNU GPL v3 or,
# for users who cannot use the GPL, under a commercial license.
# See LICENSING.md for details.

PROJECT_NAME := Compages
PROJECT_VERSION := 1.0.0
CXX_STANDARD := c++20
COMPILATION_MODE ?= debug

###############################################################################
# The library (Compages.mk, also read by the projects embedding Compages:
# CompagesProject.MAKEFILES := Compages.mk external/external.mk), its third
# parties, the tests and the gallery.
#
SUBMAKEFILES := Compages.mk external/external.mk tests/tests.mk examples/examples.mk

M ?= .makefile
include $(M)/MyMakefile.mk

# The headers of entt and units are included by the public headers of
# Compages: installed beside them.
install::
	$(Q)$(call make-dir,$(DESTDIR)$(INCLUDEDIR)/entt)
	$(Q)cp -R $(entt.DIR)src/entt/. $(DESTDIR)$(INCLUDEDIR)/entt/
	$(Q)$(call copy-files,$(units.DIR)include/units.h,$(DESTDIR)$(INCLUDEDIR)/)
