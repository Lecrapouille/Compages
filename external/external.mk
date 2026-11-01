# SPDX-License-Identifier: GPL-3.0-or-later OR LicenseRef-Compages-Commercial
# Copyright (c) 2018-2026 Quentin Quadrat
#
# This file is part of Compages. See LICENSING.md for details.
#
# Third parties of the library, fetched on the first build. Those of the
# gallery are in examples/examples.mk. Their commits are locked in
# external/versions.lock: commit it.

TARGETS += units entt stb cgltf json pugixml

# Header only libraries used by the library.
units.GIT := https://github.com/nholthaus/units.git
units.VERSION := v2.3.3
units.PUBLIC_INCLUDES := include

entt.GIT := https://github.com/skypjack/entt.git
entt.VERSION := v4.0.0
entt.PUBLIC_INCLUDES := src

stb.GIT := https://github.com/nothings/stb.git
stb.PUBLIC_INCLUDES := .

cgltf.GIT := https://github.com/jkuhlmann/cgltf.git
cgltf.PUBLIC_INCLUDES := .

json.GIT := https://github.com/nlohmann/json.git
json.PUBLIC_INCLUDES := include

# Compiled by src/World/Import/PugixmlImpl.cpp.
pugixml.GIT := https://github.com/zeux/pugixml.git
pugixml.VERSION := v1.16
pugixml.PUBLIC_INCLUDES := src
