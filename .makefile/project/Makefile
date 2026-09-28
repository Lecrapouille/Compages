##=============================================================================
## MIT License
##
## Copyright (c) 2019-2025 Quentin Quadrat <lecrapouille@gmail.com>
##
## Permission is hereby granted, free of charge, to any person obtaining a copy
## of this software and associated documentation files (the "Software"), to deal
## in the Software without restriction, including without limitation the rights
## to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
## copies of the Software, and to permit persons to whom the Software is
## furnished to do so, subject to the following conditions:
##
## The above copyright notice and this permission notice shall be included in all
## copies or substantial portions of the Software.
##
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
## AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
## LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
## OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
## SOFTWARE.
##=============================================================================

###############################################################################
# Check if some variables have been defined before including this file.
###############################################################################

#? Auto-detect author from git config or system user
AUTHOR ?= $(shell git config --global user.name 2>/dev/null || echo $(USER))

ifndef PROJECT_NAME
$(error PROJECT_NAME is mandatory for giving a name to your project.)
endif

ifndef PROJECT_VERSION
$(error PROJECT_VERSION is mandatory for giving a version to your binaries and libraries.)
endif

ifndef TARGET_NAME
$(error TARGET_NAME is mandatory for giving a name to your target executable or target library.)
endif

ifndef TARGET_DESCRIPTION
$(error TARGET_DESCRIPTION is mandatory for giving a short description of your target.)
endif

ifndef P
$(error P is mandatory for setting the local path to your project root folder.)
endif

ifndef M
$(error M is mandatory for setting the local path to MyMakefile root folder.)
endif

ifndef COMPILATION_MODE
$(error COMPILATION_MODE is mandatory for knowing if the project shall be compiled for debug or release or test or normal.)
endif

ifdef BUILD_MACOS_APP_BUNDLE
    ifndef APPLE_IDENTIFIER
    $(error APPLE_IDENTIFIER is mandatory for creating a macOS application bundle.)
    endif

    ifndef MACOS_BUNDLE_ICON
    $(error MACOS_BUNDLE_ICON is mandatory for creating a macOS application bundle.)
    endif
endif

###############################################################################
# Cleanup project variables.
###############################################################################

# Unit tests
ifeq ($(COMPILATION_MODE),test)
    COMPILATION_MODE := debug
    USE_COVERAGE := 1
endif

# Force to convert to lower case the project and target name if requested.
ifdef FORCE_LOWER_CASE
    PROJECT_NAME := $(shell echo $(PROJECT_NAME) | tr A-Z a-z)
    TARGET_NAME := $(shell echo $(TARGET_NAME) | tr A-Z a-z)
    INTERNAL_LIBS := $(shell echo $(INTERNAL_LIBS) | tr A-Z a-z)
endif

# Do not accept libxxx as project name.
ifeq "$(filter lib%,$(PROJECT_NAME))" "$(PROJECT_NAME)"
    $(error PROJECT_NAME shall not start by lib.)
endif

# Do not accept libxxx as target name since its automagically managed.
ifeq "$(filter lib%,$(TARGET_NAME))" "$(TARGET_NAME)"
    $(error TARGET_NAME shall not start by lib. Library name is automatically managed by MyMakefile.)
endif

# Set the project version. Can be used for creating shared libraries.
PROJECT_VERSION_MAJOR := $(shell echo $(PROJECT_VERSION) | cut -d"." -f1)

###############################################################################
# Change the verbosity of the makefile. For example
# V=1 make -j8
###############################################################################

Q := @
ifdef V
    VERBOSE := $(V)
endif
ifdef VERBOSE
   Q :=
endif

###############################################################################
# Use bash instead of shell for Unix systems. Activate debug mode for Makefile.
# On MSYS2/MINGW64, /bin/bash may resolve to the WSL stub on GitHub Actions;
# use the MSYS interpreter explicitly.
###############################################################################

ifdef D
    DEBUG := $(D)
endif

###############################################################################
# Detect the operating system: Unix, OSX and Windows and architecture.
###############################################################################

ifeq ($(OS),Windows_NT)
    OS := Windows
    ifdef MSYSTEM
        WIN_ENV := $(patsubst CYGWIN%,Cygwin,$(MSYSTEM))
        WIN_ENV := $(patsubst MSYS%,MSYS,$(WIN_ENV))
        WIN_ENV := $(patsubst MINGW%,MINGW,$(WIN_ENV))
    else
        WIN_ENV := MINGW
    endif
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        ARCH := 64
    endif
    ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        ARCH := 64
    else
        ARCH := 32
    endif
else ifdef MSYSTEM
    # MSYS2/MINGW64 on GitHub Actions: OS may be unset while MSYSTEM is set.
    OS := Windows
    WIN_ENV := $(patsubst CYGWIN%,Cygwin,$(MSYSTEM))
    WIN_ENV := $(patsubst MSYS%,MSYS,$(WIN_ENV))
    WIN_ENV := $(patsubst MINGW%,MINGW,$(WIN_ENV))
    ifeq ($(PROCESSOR_ARCHITEW6432),AMD64)
        ARCH := 64
    else ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
        ARCH := 64
    else
        ARCH := 32
    endif
# Compile the project for HTML5 with Emscripten or Exaequos
# https://github.com/exaequos ? It is simpler to consider as OS rather than
# target architecture.
else ifdef EXAEQUOS
    OS := Emscripten
    DEFINES += -DEXAEQUOS
else ifdef EMSCRIPTEN
    OS := Emscripten
# Linux or Darwin (MacOS)
else
    _UNAME_OS := $(shell sh -c 'uname -s 2>/dev/null || echo Unknown')
    OS := $(_UNAME_OS)
    ARCH := $(shell sh -c 'uname -m 2>/dev/null || echo Unknown')
    ifneq ($(findstring MINGW,$(_UNAME_OS)),)
        OS := Windows
        WIN_ENV := MINGW
    else ifneq ($(findstring MSYS,$(_UNAME_OS)),)
        OS := Windows
        WIN_ENV := MSYS
    else ifneq ($(findstring CYGWIN,$(_UNAME_OS)),)
        OS := Windows
        WIN_ENV := Cygwin
    else ifeq "$(OS)" "Unknown"
        $(error Unknown architecture. Abort!)
    endif
endif

export OS
export WIN_ENV
export ARCH

ifeq ($(OS),Windows)
    ifneq ($(MSYSTEM),)
        SHELL := /usr/bin/bash.exe
        BASH := /usr/bin/bash.exe
    else
        BASH := bash
    endif
else
    SHELL := /bin/bash
    BASH := /bin/bash
endif

ifdef DEBUG
    OLD_SHELL := $(SHELL)
    SHELL = $(warning [$@])$(OLD_SHELL) -x
endif

###############################################################################
# Use macros for wrapping shell commands to make them architecture independent.
###############################################################################

ifeq "$(OS)" "Windows"
    ifneq ($(findstring MINGW,$(MSYSTEM))$(findstring MSYS,$(MSYSTEM)),)
        # MSYS2/MINGW64 shells provide Unix mkdir and paths like D:/a/...
        MKDIR := mkdir -p
        RM := rm -fr
        CP := cp -fr
        CD := cd
        MV := mv
    else
        MKDIR := mkdir
        RM := rmdir /S/Q
        CP := copy /y
        CD := dir
    endif
else
    MKDIR := mkdir -p
    RM := rm -fr
    CP := cp -fr
    CD := cd
    MV := mv
endif

###############################################################################
# Colorful echo
###############################################################################

#? Echo command for colors
ECHO ?= echo
ECHO_COLOR ?= $(ECHO) -e

COLOR_DEFAULT := \033[00m
RED := \033[31m
GREEN := \033[32m
YELLOW := \033[33m
BLUE := \033[34m
PURPLE := \033[35m
CYAN := \033[36m
COLOR_INFO := \033[42m
COLOR_WARNING := \033[43m
COLOR_ERROR := \033[41m

###############################################################################
# Display predefined colorful messages
###############################################################################

print-simple = $(ECHO_COLOR) "$(PURPLE)*** $1: $(CYAN)$2$(COLOR_DEFAULT)$3"
print-from = $(ECHO_COLOR) "$(PURPLE)*** $1: $(CYAN)$2$(COLOR_DEFAULT) <= $(YELLOW)$3$(COLOR_DEFAULT)$4"
print-to = $(ECHO_COLOR) "$(PURPLE)*** $1: $(CYAN)$2$(COLOR_DEFAULT) => $(YELLOW)$3$(COLOR_DEFAULT)$4"
print-install = $(ECHO_COLOR) "$(PURPLE)*** Installing $1: $(CYAN)$2$(COLOR_DEFAULT) => $(YELLOW)$3$(COLOR_DEFAULT)$4"
print-info = $(ECHO_COLOR) "$(COLOR_INFO)!!! $1 !!!$(COLOR_DEFAULT)"
print-warning = $(ECHO_COLOR) "$(COLOR_WARNING)!!! $1 !!!$(COLOR_DEFAULT)"
print-error = $(ECHO_COLOR) "$(COLOR_ERROR)!!! $1 !!!$(COLOR_DEFAULT)"

###############################################################################
# Make does not offer a recursive recursive wildcard
# $1: list of folder to traverse
# $2: list of patterns
###############################################################################

rwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call rwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
list-folders=$(sort $(dir $(call rwildcard,$1,*)))

###############################################################################
# Internal libs are supposed to be compiled previously and are by consequent
# present on the build folder.
###############################################################################
define internal-lib
    $(BUILD_PATH)/lib$(1)$(EXT_STATIC_LIB)
endef

###############################################################################
# Set extensions for applications, static and shared libraries.
# Beware: for MacOS target a bundle application (xxx.app) is a folder not a
# binary which is inside the folder. See macro CREATE_MACOS_APP_BUNDLE.
# For ExaequOS we define .js but it also generates .wasm files.
###############################################################################

ifeq ($(OS),Linux)
    EXT_BINARY :=
    EXT_DYNAMIC_LIB := .so
    EXT_STATIC_LIB := .a
else ifeq ($(OS),Darwin)
    ifdef BUILD_MACOS_APP_BUNDLE
        EXT_BINARY := .app
    else
        EXT_BINARY :=
    endif
    EXT_DYNAMIC_LIB := .dylib
    EXT_STATIC_LIB := .a
else ifeq ($(OS),Windows)
    EXT_BINARY := .exe
    EXT_DYNAMIC_LIB := .dll
    # MinGW ar/ranlib produce Unix static archives (.a), not MSVC import libs.
    ifeq ($(WIN_ENV),MINGW)
        EXT_STATIC_LIB := .a
    else
        EXT_STATIC_LIB := .lib
    endif
else ifeq ($(OS),Emscripten)
    ifdef EXAEQUOS
        EXT_BINARY := .js
    else
        EXT_BINARY := .html
    endif
    EXT_DYNAMIC_LIB := .so
    EXT_STATIC_LIB := .a
else
    $(error Unknown binary file extension for operating system '$(OS)')
endif

###############################################################################
## GNU standard paths.
## See https://www.gnu.org/prep/standards/html_node/Directory-Variables.html
###############################################################################

#? Define where the project will be installed: $(DESTDIR)$(PREFIX)/lib and $(DESTDIR)$(PREFIX)/bin.
DESTDIR ?=

#? Define where the project will be installed: $(DESTDIR)$(PREFIX)/lib and $(DESTDIR)$(PREFIX)/bin
PREFIX ?= /usr/local

#? Define where to install includes.
INCLUDEDIR ?= $(PREFIX)/include

# Workaround initial missing initial space
INCLUDES := " "

#? Define where to install libraries.
LIBDIR ?= $(PREFIX)/lib

#? Define where to install pkgconfig files.
PKGLIBDIR ?= $(LIBDIR)/pkgconfig

#? Define where to install data and documentation.
DATADIR ?= $(PREFIX)/share

#? Define where to install standalone applications.
BINDIR ?= $(PREFIX)/bin

#? Define where to you can store temporary files.
TMPDIR ?= /tmp

# Final paths.
INSTALL_PKGLIBDIR := $(DESTDIR)$(PKGLIBDIR)
INSTALL_INCLUDEDIR := $(DESTDIR)$(INCLUDEDIR)/$(PROJECT_NAME)/$(PROJECT_VERSION)
INSTALL_LIBDIR := $(DESTDIR)$(LIBDIR)
INSTALL_BINDIR := $(DESTDIR)$(BINDIR)
INSTALL_DATADIR := $(DESTDIR)$(DATADIR)

# Override install folder for ExaequOS
ifdef EXAEQUOS
    INSTALL_BINDIR := $(abspath $(P)/media/localhost/$(TARGET_NAME)/exa)
endif

# Project installation paths on the operating system folder
INSTALL_PROJECT_DIR := $(INSTALL_DATADIR)/$(PROJECT_NAME)/$(PROJECT_VERSION)

# Project installation paths on the user folder
ifeq ($(OS),Linux)
    INSTALL_LOCAL_PROJECT_DATA_DIR += $(HOME)/.local/share/$(PROJECT_NAME)/$(PROJECT_VERSION)/$(DATA_FOLDER_NAME)
else ifeq ($(OS),Windows)
    INSTALL_LOCAL_PROJECT_DATA_DIR += C:\\Users\\$(USER)\$(PROJECT_NAME)\\$(PROJECT_VERSION)\\$(DATA_FOLDER_NAME)
else ifeq ($(OS),Darwin)
    ifdef BUILD_MACOS_APP_BUNDLE
        # Nothing to do it should be computed dynamically inside the bundle path
    endif
else ifeq ($(OS),Emscripten)
    INSTALL_LOCAL_PROJECT_DATA_DIR += $(DATA_FOLDER_NAME)
else
    # Nothing to do
endif

###############################################################################
# Set internal project paths.
###############################################################################

SRC_FILES :=
LIB_FILES :=
INTERNAL_LIBS :=
DIRS_WITH_MAKEFILE :=

#? Name of the directory holding the compilation artifacts.
BUILD ?= build
BUILD_PATH := $(abspath $(P)/$(BUILD))

#? Path to the RPM spec file.
RPM_TOPDIR := $(BUILD)/rpm
RPM_TOPDIR_PATH := $(abspath $(P)/$(RPM_TOPDIR))
RPM_SPEC_FILE := $(RPM_TOPDIR)/SPECS/$(PROJECT_NAME).spec

# Root directory holding compiled C/C++ files.
OBJS_ROOT_DIR := $(BUILD_PATH)/$(COMPILATION_MODE)/objs-$(OS)/$(TARGET_NAME)

# Mach-O binary linked before wrapping into a macOS .app bundle directory.
ifdef BUILD_MACOS_APP_BUNDLE
MACOS_APP_BINARY := $(OBJS_ROOT_DIR)/$(TARGET_NAME)
endif

# Directory holding generated files.
GENERATION_DIR := $(BUILD_PATH)/generated/$(TARGET_NAME)
COMPILER_INFO_DONE := $(GENERATION_DIR)/.compiler-info

# Directory holding downloaded stuffs.
DOWNLOAD_DIR := $(BUILD_PATH)/downloaded

#? Name of the directory holding third parties libraries (external libraries).
THIRD_PARTIES_FOLDER_NAME ?= external
THIRD_PARTIES_DIR := $(abspath $(P)/$(THIRD_PARTIES_FOLDER_NAME))

#? Name of the directory holding documentation.
DOC_FOLDER_NAME ?= doc
PROJECT_DOC_DIR := $(abspath $(P)/$(DOC_FOLDER_NAME))

#? Name of the directory holding generated reports.
PROJECT_GENERATED_FOLDER_NAME ?= doc
PROJECT_GENERATED_DOC_DIR := $(abspath $(P)/$(PROJECT_GENERATED_FOLDER_NAME))

#? Directory where Doxygen is generated
GENERATED_DOXYGEN_DIR ?= $(PROJECT_GENERATED_DOC_DIR)/doxygen
DOXYGEN_INDEX_HTML := $(GENERATED_DOXYGEN_DIR)/html/index.html

#? Path of the generated profiling analysis
GPROF_ANALYSIS ?= $(PROJECT_GENERATED_DOC_DIR)/profiling/analysis.txt
GPROF_ANALYSIS_FOLDER := $(dir $(GPROF_ANALYSIS))

#? Path of the generated code coverage report
COVERAGE_DIR ?= $(PROJECT_GENERATED_DOC_DIR)/code-coverage
COVERAGE_HTML_RAPPORT ?= $(COVERAGE_DIR)/coverage.html
COVERAGE_LCOV_RAPPORT ?= $(COVERAGE_DIR)/lcov.info

#? Define the directory holding project data.
DATA_FOLDER_NAME ?= data
PROJECT_DATA_DIR := $(abspath $(P)/$(DATA_FOLDER_NAME))

#? Define the directory holding unit tests.
PROJECT_TESTS ?= tests

# Generated file holding project information used i.e. by MyLogger.
PROJECT_INFO := project_info
PROJECT_INFO_TXT := $(PROJECT_INFO).txt
PROJECT_INFO_HPP := $(PROJECT_INFO).hpp
GENERATED_HEADER_PROJECT_INFO := $(GENERATION_DIR)/$(PROJECT_INFO_HPP)

#? Path to the default icon for MacOS bundle applications.
ifeq ($(OS),Darwin)
    MACOS_BUNDLE_ICON ?= $(M)/assets/$(DATA_FOLDER_NAME)/macos.icns
endif

PATH_PROJECT_LOGO ?= $(abspath $(M)/assets/icons/logo.png)
DOXYGEN_INPUTS ?= $(abspath $(P)/README.md $(P)/src $(P)/include)

#? Path where to store temporary project files.
PROJECT_TEMP_DIR ?= $(TMPDIR)/$(PROJECT_NAME)/$(PROJECT_VERSION)

# Files that may not exist but giving information about the project.
PROJECT_OTHER_FILES ?= AUTHORS LICENSE README.md ChangeLog VERSION

# File extensions matching for C++ header files.
REGEXP_CXX_HEADER_FILES ?= *.hpp *.ipp *.tpp *.hh *.h *.hxx *.incl

# Define dependencies for debug libraries that need downloading
DEPS_DEBUG_LIBS :=

#? Use nodelete flag for shared libraries
PREVENT_SHARED_LIB_UNLOAD ?=

#? Custom runtime library search paths (rpath) - colon separated
CUSTOM_RPATH ?=

#? Do not compile static library
DO_NOT_COMPILE_STATIC_LIB ?=

#? Do not compile shared library
DO_NOT_COMPILE_SHARED_LIB ?=

###############################################################################
# Rebuild the whole project if a Makefile has been modified
###############################################################################

REBUILD_IF_MAKEFILE_MODIFIED ?=

ifndef REBUILD_IF_MAKEFILE_MODIFIED
LIST_MYMAKEFILES := $(M)/project/Makefile $(M)/rules/Makefile
ifneq (,$(wildcard $(P)/Makefile))
    LIST_MYMAKEFILES += $(P)/Makefile
endif
ifneq (,$(wildcard $(P)/Makefile.common))
    LIST_MYMAKEFILES += $(P)/Makefile.common
endif
else
LIST_MYMAKEFILES :=
endif

###############################################################################
# MyMakefile paths
###############################################################################

PATH_MYMAKEFILE_DOXYFILE := $(abspath $(M)/assets/doxygen)
PATH_MYMAKEFILE_SCRIPTS := $(abspath $(M)/assets/scripts)
SCRIPT_CONFIG := $(PATH_MYMAKEFILE_SCRIPTS)/config.sh
SCRIPT_EXTRACT_STATIC_LIB := $(PATH_MYMAKEFILE_SCRIPTS)/alib.sh
SCRIPT_TAR_PROJECT := $(PATH_MYMAKEFILE_SCRIPTS)/targz.sh
SCRIPT_GIT_CLONE := $(PATH_MYMAKEFILE_SCRIPTS)/clone.sh
SCRIPT_DETECT_LICENSE := $(PATH_MYMAKEFILE_SCRIPTS)/detect-license.sh
SCRIPT_DOWNLOAD_THIRD_PARTIES := $(PATH_MYMAKEFILE_SCRIPTS)/download-external-libs.sh
SCRIPT_COMPILE_THIRD_PARTIES := $(PATH_MYMAKEFILE_SCRIPTS)/compile-external-libs.sh
PROJECT_MANIFEST_THIRD_PARTIES := $(THIRD_PARTIES_DIR)/manifest
PROJECT_SCRIPT_COMPILE_THIRD_PARTIES := $(THIRD_PARTIES_DIR)/compilation
SCRIPT_OBS := $(PATH_MYMAKEFILE_SCRIPTS)/obs.sh

###############################################################################
# Auto-detect license from LICENSE file or default to MIT
###############################################################################

DETECTED_LICENSE := $(shell $(BASH) $(SCRIPT_DETECT_LICENSE) $(P) 2>/dev/null || echo "UNKNOWN")
LICENSE ?= $(if $(DETECTED_LICENSE),$(DETECTED_LICENSE),MIT)
