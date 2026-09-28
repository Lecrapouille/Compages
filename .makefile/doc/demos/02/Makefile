###################################################
# Location of the project directory and Makefiles
#
P := .
M := ../../..
BUILD := ../../build

###################################################
# Project definition
#
PROJECT_NAME := TutorialMyMakefile
PROJECT_VERSION := 0.1.0
TARGET_NAME := HelloWorld02
TARGET_DESCRIPTION := Hello Word
COMPILATION_MODE := release

include $(M)/project/Makefile

###################################################
# Internal libs to compile in the correct order
#
LIB_FOO := $(call internal-lib,foo)
LIB_BAR := $(call internal-lib,bar)
INTERNAL_LIBS := $(LIB_FOO) $(LIB_BAR)
DIRS_WITH_MAKEFILE := $(P)/src/bar $(P)/src/foo

###################################################
# Compile the project
#
INCLUDES := include
SRC_FILES := src/main.cpp

###################################################
# Generic Makefile rules
#
include $(M)/rules/Makefile